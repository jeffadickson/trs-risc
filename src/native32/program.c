/* Sorted native line vector. Source line numbers use the full nonnegative
 * signed32 range; legacy16-bit .basram links are not part of this structure.
 * Mutations preflight allocation before releasing existing program text. */
#include "native.h"
#include <stdlib.h>
#include <string.h>
static size_t find(const N32Program *p,uint32_t number){size_t a=0,b=p->count;while(a<b){size_t c=a+(b-a)/2;if(p->lines[c].number<number)a=c+1;else b=c;}return a;}
static void clear_program(N32Program *p){for(size_t i=0;i<p->count;i++)free(p->lines[i].tokens);free(p->lines);*p=(N32Program){0};}
int n32_program_set(N32Program *p,uint32_t number,const uint8_t *text,size_t length){
 if(number>INT32_MAX||(!text&&length))return N32_DOMAIN;
 size_t at=find(p,number);bool exists=at<p->count&&p->lines[at].number==number;
 if(!length){if(exists){free(p->lines[at].tokens);memmove(p->lines+at,p->lines+at+1,(p->count-at-1)*sizeof *p->lines);p->count--;}return N32_OK;}
 uint8_t *copy=malloc(length);if(!copy)return N32_MEMORY;memcpy(copy,text,length);
 if(exists){free(p->lines[at].tokens);p->lines[at].tokens=copy;p->lines[at].length=length;return N32_OK;}
 if(p->count==p->capacity){size_t capacity=p->capacity?p->capacity*2:8;if(capacity<p->capacity||capacity>SIZE_MAX/sizeof *p->lines){free(copy);return N32_MEMORY;}
  N32Line *lines=realloc(p->lines,capacity*sizeof *lines);if(!lines){free(copy);return N32_MEMORY;}p->lines=lines;p->capacity=capacity;}
 memmove(p->lines+at+1,p->lines+at,(p->count-at)*sizeof *p->lines);p->lines[at]=(N32Line){number,copy,length};p->count++;return N32_OK;
}
void n32_init(N32Machine *m,N32Write write,N32Read read,void *context){*m=(N32Machine){.write=write,.read=read,.context=context,.random=0x19781975};memset(m->defaults,N32_SINGLE,sizeof m->defaults);}
void n32_destroy(N32Machine *m){if(m){n32_variables_clear(m);clear_program(&m->program);*m=(N32Machine){0};}}
static void spaces(const uint8_t *text,size_t length,size_t *at){while(*at<length&&(text[*at]==' '||text[*at]=='\t'))(*at)++;}
static int trailing(const uint8_t *text,size_t length,size_t at){spaces(text,length,&at);return at==length?N32_OK:N32_SYNTAX;}
static int output(N32Machine *m,const char *text,size_t length){return length&&m->write?m->write(m->context,(const uint8_t *)text,length):N32_OK;}
static int say(N32Machine *m,const char *text){return output(m,text,strlen(text));}
static int print_value(N32Machine *m,const N32Value *v){if(v->type==N32_STRING)return output(m,(const char *)v->as.string.bytes,v->as.string.length);
 char text[48];size_t length;int status=n32_format(v,text,sizeof text,&length);if(status)return status;
 if(text[0]!='-'){status=say(m," ");if(status)return status;}status=output(m,text,length);return status?status:say(m," ");}
static int line_number(const uint8_t *text,size_t length,size_t *at,uint32_t *number){
 spaces(text,length,at);size_t start=*at;uint32_t value=0;
 while(*at<length&&text[*at]>='0'&&text[*at]<='9'){unsigned digit=text[(*at)++]-'0';if(value>((uint32_t)INT32_MAX-digit)/10)return N32_OVERFLOW;value=value*10+digit;}
 if(start==*at)return N32_SYNTAX;*number=value;return N32_OK;
}
static size_t statement_end(const uint8_t *text,size_t length,size_t at){bool quote=false;for(;at<length;at++){uint8_t c=text[at];if(c=='"')quote=!quote;if(!quote&&(c==':'||c==T_ELSE))break;}return at;}
static int numeric(N32Machine *m,const uint8_t *text,size_t length,size_t *used,N32Value *value){int status=n32_expression(m,text,length,used,value);if(!status&&value->type==N32_STRING){n32_clear(value);return N32_TYPE;}return status;}
static int target(N32Machine *m,const uint8_t *text,size_t length,N32Cursor *next){
 size_t used=0;uint32_t number;int status=line_number(text,length,&used,&number);if(status)return status;if(trailing(text,length,used))return N32_SYNTAX;
 size_t at=find(&m->program,number);if(at==m->program.count||m->program.lines[at].number!=number)return N32_UNDEFINED;*next=(N32Cursor){at,0};return N32_OK;
}
static int defaults(N32Machine *m,uint8_t token,const uint8_t *text,size_t length){
 N32Type updated[26];memcpy(updated,m->defaults,sizeof updated);N32Type type=token==T_DEFINT?N32_INTEGER:token==T_DEFSNG?N32_SINGLE:token==T_DEFDBL?N32_DOUBLE:N32_STRING;size_t at=0;
 for(;;){spaces(text,length,&at);if(at==length||text[at]<'A'||text[at]>'Z')return N32_SYNTAX;unsigned first=text[at++]-'A',last=first;spaces(text,length,&at);
  if(at<length&&text[at]=='-'){at++;spaces(text,length,&at);if(at==length||text[at]<'A'||text[at]>'Z')return N32_SYNTAX;last=text[at++]-'A';}
  if(last<first)return N32_SYNTAX;for(unsigned i=first;i<=last;i++)updated[i]=type;spaces(text,length,&at);if(at==length)break;if(text[at++]!=',')return N32_SYNTAX;
 }
 memcpy(m->defaults,updated,sizeof updated);return N32_OK;
}
/* Typed input is parsed at DOUBLE precision before destination conversion,
 * avoiding an intermediate binary32 rounding of a32-bit INTEGER response. */
static int input_value(const uint8_t *text,size_t length,N32Type type,N32Value *result){
 size_t at=0;spaces(text,length,&at);while(length>at&&(text[length-1]==' '||text[length-1]=='\t'))length--;
 if(type==N32_STRING){if(at<length&&text[at]=='"'){at++;if(length>at&&text[length-1]=='"')length--;}return n32_string(text+at,length-at,result);}
 bool negative=false;if(at<length&&(text[at]=='-'||text[at]=='+'))negative=text[at++]=='-';
 if(at==length){*result=n32_integer(0);return N32_OK;}
 uint8_t number[N32_LINE_LIMIT+2];size_t n=length-at;if(n>N32_LINE_LIMIT)return N32_LIMIT;memcpy(number,text+at,n);
 if(number[n-1]!='%'&&number[n-1]!='!'&&number[n-1]!='#')number[n++]='#';
 size_t used;N32Value value={0};int status=n32_parse_number(number,n,&used,&value);if(status)return status;if(used!=n)return N32_SYNTAX;
 if(negative){status=n32_unary('-',&value,&value);if(status)return status;}return n32_convert(&value,type,result);
}
static int pending_input(N32Machine *m){
 if(!m->read)return N32_UNSUPPORTED;
 if(!m->input_prompted){int status=say(m,"? ");if(status)return status;m->input_prompted=true;}
 uint8_t bytes[N32_LINE_LIMIT];size_t length=0;int status=m->read(m->context,bytes,sizeof bytes,&length);if(status)return status;if(length>sizeof bytes)return N32_IO;
 N32Value value={0};status=input_value(bytes,length,m->input_target.type,&value);
 if(status==N32_SYNTAX||status==N32_TYPE){int sent=say(m,"?REDO\n");m->input_prompted=false;return sent?sent:N32_WAIT;}
 if(!status)status=n32_set(m,&m->input_target,&value);n32_clear(&value);if(status)return status;
 m->input_pending=m->input_prompted=false;m->cursor=m->input_resume;return N32_OK;
}
static int execute(N32Machine *m,const uint8_t *text,size_t length,size_t absolute,N32Cursor *next){
 size_t at=0,used=0;spaces(text,length,&at);if(at==length)return N32_OK;uint8_t token=text[at++];int status=N32_OK;
 if(token==T_REM||token==T_DATA)return N32_OK;
 if(token==T_END||token==T_STOP){status=trailing(text,length,at);if(!status)m->running=false;return status;}
 if(token==T_CLEAR){if(trailing(text,length,at))return N32_UNSUPPORTED;n32_variables_clear(m);return N32_OK;}
 if(token==T_TRON||token==T_TROFF){status=trailing(text,length,at);if(!status)m->trace=token==T_TRON;return status;}
 if(token>=T_DEFINT&&token<=T_DEFSTR)return defaults(m,token,text+at,length-at);
 if(token==T_PRINT){bool separator=false;spaces(text,length,&at);
  while(at<length){if(text[at]==';'||text[at]==','){separator=true;if(text[at]==',')status=say(m," ");at++;}
   else{N32Value value={0};status=n32_expression(m,text+at,length-at,&used,&value);if(!status)status=print_value(m,&value);n32_clear(&value);if(status)return status;at+=used;separator=false;}
   spaces(text,length,&at);if(status)return status;if(!separator&&at<length&&text[at]!=';'&&text[at]!=',')return N32_SYNTAX;
  }return separator?N32_OK:say(m,"\n");
 }
 if(token==T_IF){N32Value condition={0};status=numeric(m,text+at,length-at,&used,&condition);if(status)return status;at+=used;spaces(text,length,&at);if(at==length||text[at++]!=T_THEN)return N32_SYNTAX;
  size_t otherwise=length;bool quote=false;for(size_t i=at;i<length;i++){if(text[i]=='"')quote=!quote;if(!quote&&text[i]==T_IF)return N32_UNSUPPORTED;if(!quote&&text[i]==T_ELSE){otherwise=i;break;}}
  bool truth=n32_real(&condition)!=0;size_t selected=truth?at:otherwise==length?length:otherwise+1,end=truth?otherwise:length;spaces(text,end,&selected);
  if(selected==end){*next=(N32Cursor){m->cursor.line+1,0};return N32_OK;}
  if(text[selected]>='0'&&text[selected]<='9')return target(m,text+selected,end-selected,next);
  *next=(N32Cursor){m->cursor.line,absolute+selected};return N32_OK;
 }
 if(token==T_GOTO||token==T_GOSUB){if(!m->running)return N32_UNSUPPORTED;if(token==T_GOSUB&&m->return_count==N32_CONTROL_LIMIT)return N32_LIMIT;
  N32Cursor destination;status=target(m,text+at,length-at,&destination);if(status)return status;if(token==T_GOSUB)m->returns[m->return_count++]=*next;*next=destination;return N32_OK;
 }
 if(token==T_RETURN){if(trailing(text,length,at))return N32_SYNTAX;if(!m->return_count)return N32_DOMAIN;*next=m->returns[--m->return_count];return N32_OK;}
 if(token==T_FOR){if(!m->running)return N32_UNSUPPORTED;if(m->loop_count==N32_CONTROL_LIMIT)return N32_LIMIT;
  N32Reference r;status=n32_reference(m,text+at,length-at,&used,&r);if(status)return status;at+=used;if(r.rank||r.type==N32_STRING)return N32_TYPE;
  if(at==length||text[at++]!='=')return N32_SYNTAX;N32Value initial={0},limit={0},step=n32_integer(1);
  status=numeric(m,text+at,length-at,&used,&initial);if(status)return status;at+=used;if(at==length||text[at++]!=T_TO)return N32_SYNTAX;
  status=numeric(m,text+at,length-at,&used,&limit);if(status)return status;at+=used;
  if(at<length&&text[at]==T_STEP){at++;status=numeric(m,text+at,length-at,&used,&step);if(status)return status;at+=used;}
  if(trailing(text,length,at))return N32_SYNTAX;
  status=n32_convert(&limit,r.type,&limit);if(status)return status;status=n32_convert(&step,r.type,&step);if(status)return status;status=n32_set(m,&r,&initial);if(status)return status;
  /* Restarting the same control variable discards its nested frames. */
  size_t frame=m->loop_count;for(size_t i=0;i<m->loop_count;i++)if(m->loops[i].variable.type==r.type&&!strcmp(m->loops[i].variable.name,r.name)){frame=i;break;}
  m->loops[frame]=(N32Loop){r,limit,step,*next};m->loop_count=frame+1;return N32_OK;
 }
 if(token==T_NEXT){if(!m->loop_count)return N32_DOMAIN;size_t frame=m->loop_count-1;spaces(text,length,&at);
  if(at<length){N32Reference r;status=n32_reference(m,text+at,length-at,&used,&r);if(status)return status;at+=used;if(trailing(text,length,at))return N32_UNSUPPORTED;
   while(r.type!=m->loops[frame].variable.type||strcmp(r.name,m->loops[frame].variable.name)){if(!frame)return N32_DOMAIN;frame--;}}
  N32Loop *loop=&m->loops[frame];N32Value current={0},incremented={0};status=n32_get(m,&loop->variable,&current);if(status)return status;
  status=n32_binary('+',&current,&loop->step,&incremented);if(status)return status;status=n32_set(m,&loop->variable,&incremented);if(status)return status;
  status=n32_get(m,&loop->variable,&current);if(status)return status;int order;status=n32_compare(&current,&loop->limit,&order);if(status)return status;
  bool again=n32_real(&loop->step)<0?order>=0:order<=0;m->loop_count=again?frame+1:frame;if(again)*next=loop->resume;return N32_OK;
 }
 if(token==T_INPUT){if(!m->running)return N32_UNSUPPORTED;spaces(text,length,&at);
  if(at<length&&text[at]=='"'){size_t start=++at;while(at<length&&text[at]!='"')at++;if(at==length)return N32_SYNTAX;status=output(m,(const char *)text+start,at-start);if(status)return status;at++;spaces(text,length,&at);if(at==length||text[at++]!=';')return N32_SYNTAX;}
  status=n32_reference(m,text+at,length-at,&used,&m->input_target);if(status)return status;at+=used;if(trailing(text,length,at))return N32_UNSUPPORTED;
  m->input_pending=true;m->input_prompted=false;m->input_resume=*next;return N32_WAIT;
 }
 if(token==T_DIM){do{N32Reference r;status=n32_reference(m,text+at,length-at,&used,&r);if(status)return status;if(!r.rank)return N32_SYNTAX;at+=used;status=n32_dimension(m,&r);if(status)return status;if(at==length)return N32_OK;if(text[at++]!=',')return N32_SYNTAX;}while(true);}
 if(token==T_RESTORE){status=trailing(text,length,at);if(!status)m->data=(N32Cursor){0};return status;}
 if(token==T_READ){do{
   N32Reference r;status=n32_reference(m,text+at,length-at,&used,&r);if(status)return status;at+=used;
   N32Cursor cursor=m->data;bool found=false;
   while(cursor.line<m->program.count){N32Line *line=&m->program.lines[cursor.line];
    if(cursor.offset&&cursor.offset<=line->length){found=true;break;}
    size_t i=0;while(i<line->length&&line->tokens[i]!=T_DATA&&line->tokens[i]!=T_REM)i++;
    if(i<line->length&&line->tokens[i]==T_DATA){cursor.offset=i+1;found=true;break;}cursor.line++;cursor.offset=0;
   }
   if(!found)return N32_DOMAIN;N32Line *line=&m->program.lines[cursor.line];size_t start=cursor.offset,end=start;bool quote=false;
   while(end<line->length){uint8_t ch=line->tokens[end];if(ch=='"')quote=!quote;if(ch==','&&!quote)break;end++;}
   N32Value value={0};status=input_value(line->tokens+start,end-start,r.type,&value);if(!status)status=n32_set(m,&r,&value);n32_clear(&value);if(status)return status;
   m->data=end<line->length?(N32Cursor){cursor.line,end+1}:(N32Cursor){cursor.line+1,0};
   if(at==length)return N32_OK;if(text[at++]!=',')return N32_SYNTAX;
  }while(true);
 }
 if(token!=T_LET){if(token<'A'||token>'Z')return N32_UNSUPPORTED;at--;}
 N32Reference r;status=n32_reference(m,text+at,length-at,&used,&r);if(status)return status;at+=used;if(at==length||text[at++]!='=')return N32_SYNTAX;
 N32Value value={0};status=n32_expression(m,text+at,length-at,&used,&value);if(!status)status=trailing(text,length,at+used);if(!status)status=n32_set(m,&r,&value);n32_clear(&value);return status;
}
int n32_start(N32Machine *m,size_t budget){n32_variables_clear(m);m->running=true;m->input_pending=m->input_prompted=false;m->cursor=m->data=(N32Cursor){0};m->steps=0;m->budget=budget;m->error_line=0;return N32_OK;}
int n32_step(N32Machine *m,size_t statements){
 while(m->running&&statements--){int status;
  if(m->input_pending){status=pending_input(m);if(status==N32_WAIT)return status;if(status)goto failed;continue;}
  if(m->cursor.line>=m->program.count){m->running=false;return N32_OK;}N32Line *line=&m->program.lines[m->cursor.line];size_t at=m->cursor.offset;spaces(line->tokens,line->length,&at);
  if(at==line->length||line->tokens[at]==T_ELSE){m->cursor=(N32Cursor){m->cursor.line+1,0};continue;}
  if(line->tokens[at]==':'){m->cursor.offset=at+1;continue;}
  m->error_line=line->number;if(m->steps==m->budget){status=N32_LIMIT;goto failed;}m->steps++;
  if(!m->cursor.offset&&m->trace){char text[48];size_t length;N32Value number=n32_integer((int32_t)line->number);status=n32_format(&number,text,sizeof text,&length);if(status)goto failed;status=say(m,"<");if(status)goto failed;status=output(m,text,length);if(status)goto failed;status=say(m,">");if(status)goto failed;}
  size_t end=line->tokens[at]==T_IF||line->tokens[at]==T_REM||line->tokens[at]==T_DATA?line->length:statement_end(line->tokens,line->length,at);
  N32Cursor next={m->cursor.line,end};status=execute(m,line->tokens+at,end-at,at,&next);
  if(status==N32_WAIT)return status;if(status)goto failed;m->cursor=next;continue;
failed:m->running=false;m->input_pending=false;return status;
 }
 return N32_OK;
}
int n32_command(N32Machine *m,const char *source){
 size_t length=strlen(source),at=0;while(at<length&&(source[at]==' '||source[at]=='\t'))at++;if(at==length)return N32_OK;
 if(source[at]>='0'&&source[at]<='9'){
  uint32_t number;int status=line_number((const uint8_t *)source,length,&at,&number);if(status)return status;spaces((const uint8_t *)source,length,&at);
  uint8_t tokens[N32_LINE_LIMIT];size_t used;status=n32_tokenize(source+at,tokens,sizeof tokens,&used);if(status)return status;status=n32_program_set(&m->program,number,tokens,used);if(!status)n32_variables_clear(m);return status;
 }
 uint8_t tokens[N32_LINE_LIMIT];size_t used;int status=n32_tokenize(source+at,tokens,sizeof tokens,&used);if(status)return status;if(!used)return N32_OK;
 if(tokens[0]==T_RUN){if(trailing(tokens,used,1))return N32_UNSUPPORTED;return n32_start(m,1000000);}
 if(tokens[0]==T_NEW){if(trailing(tokens,used,1))return N32_SYNTAX;clear_program(&m->program);n32_variables_clear(m);m->trace=false;return N32_OK;}
 if(tokens[0]==T_LIST){if(trailing(tokens,used,1))return N32_UNSUPPORTED;for(size_t i=0;i<m->program.count;i++){char text[N32_LINE_LIMIT*6],number[48];size_t n;N32Value v=n32_integer((int32_t)m->program.lines[i].number);status=n32_format(&v,number,sizeof number,&n);if(status)return status;status=output(m,number,n);if(status)return status;status=say(m," ");if(status)return status;status=n32_list_tokens(m->program.lines[i].tokens,m->program.lines[i].length,text,sizeof text,&n);if(status)return status;status=output(m,text,n);if(status)return status;status=say(m,"\n");if(status)return status;}return N32_OK;}
 size_t offset=0;while(offset<used){spaces(tokens,used,&offset);size_t end=statement_end(tokens,used,offset);N32Cursor next={0,end};status=execute(m,tokens+offset,end-offset,offset,&next);if(status)return status;if(end==used)break;offset=end+1;}return N32_OK;
}
int n32_source(N32Machine *m,const char *text,size_t length){
 if(!m||(!text&&length))return N32_DOMAIN;for(size_t i=0;i<length;i++)if(!text[i])return N32_SYNTAX;
 N32Machine parsed;n32_init(&parsed,NULL,NULL,NULL);size_t at=0;int status=N32_OK;
 while(at<length){size_t start=at;while(at<length&&text[at]!='\n'&&text[at]!='\r')at++;size_t n=at-start;char line[N32_LINE_LIMIT+1];if(n>N32_LINE_LIMIT){status=N32_LIMIT;break;}memcpy(line,text+start,n);line[n]=0;
  size_t first=0;spaces((const uint8_t *)line,n,&first);if(first<n){if(line[first]<'0'||line[first]>'9'){status=N32_SYNTAX;break;}status=n32_command(&parsed,line);if(status)break;}
  if(at<length&&text[at]=='\r')at++;if(at<length&&text[at]=='\n')at++;
 }
 if(!status){clear_program(&m->program);n32_variables_clear(m);m->program=parsed.program;parsed.program=(N32Program){0};}n32_destroy(&parsed);return status;
}

int n32_feed(N32Machine *m,uint8_t byte){
 if(m->running)return N32_WAIT;
 if(m->after_cr){m->after_cr=false;if(byte=='\n')return N32_OK;}
 if(byte==3||byte==27){m->input_length=0;m->input_invalid=false;return say(m,"\nREADY\n>");}
 if(byte=='\r'||byte=='\n'){m->after_cr=byte=='\r';m->input_line[m->input_length]=0;int status=say(m,"\n");if(status)return status;status=m->input_invalid?N32_LIMIT:n32_command(m,m->input_line);m->input_length=0;m->input_invalid=false;
  if(status){say(m,"?");say(m,n32_error(status));say(m," Error\n");}if(!m->running)say(m,"READY\n>");return status;
 }
 if(byte==8||byte==127){if(m->input_length){m->input_length--;return say(m,"\b \b");}return N32_OK;}
 if(byte<32&&byte!='\t')return N32_SYNTAX;if(byte>126)return N32_SYNTAX;
 if(m->input_invalid)return N32_LIMIT;if(m->input_length==N32_LINE_LIMIT){m->input_invalid=true;return N32_LIMIT;}int status=output(m,(const char *)&byte,1);if(!status)m->input_line[m->input_length++]=(char)byte;return status;
}
