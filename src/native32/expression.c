/* Bounded recursive descent on byte tokens. Native values and ordinary calls
 * replace the ROM's register/flag/stack machinery. Power retains Level II's
 * left associativity; unary minus binds below power, NOT below comparisons. */
#include "native.h"
#include <string.h>
typedef struct {N32Machine *machine;const uint8_t *text;size_t length,at;unsigned depth;} Parser;
static uint8_t peek(Parser *p){while(p->at<p->length&&(p->text[p->at]==' '||p->text[p->at]=='\t'))p->at++;return p->at<p->length?p->text[p->at]:0;}
static bool accept(Parser *p,uint8_t token){if(peek(p)!=token||p->at==p->length)return false;p->at++;return true;}
static bool alpha(uint8_t c){return c>='A'&&c<='Z';}
static int expression(Parser *,unsigned,N32Value *);
static int reference(Parser *p,N32Reference *result){
 N32Reference r={0};uint8_t ch=peek(p);if(!alpha(ch))return N32_SYNTAX;r.type=p->machine->defaults[ch-'A'];unsigned significant=0;
 while(p->at<p->length&&(alpha(p->text[p->at])||(p->text[p->at]>='0'&&p->text[p->at]<='9'))){if(significant<2)r.name[significant++]=(char)p->text[p->at];p->at++;}
 ch=peek(p);if(ch=='%'||ch=='!'||ch=='#'||ch=='$'){p->at++;r.type=ch=='%'?N32_INTEGER:ch=='!'?N32_SINGLE:ch=='#'?N32_DOUBLE:N32_STRING;}
 if(accept(p,'(')){do{
   if(r.rank==N32_RANK)return N32_SUBSCRIPT;N32Value index={0};int status=expression(p,1,&index);if(status)return status;
   int32_t integer;status=n32_to_integer(&index,&integer);n32_clear(&index);if(status||integer<0)return N32_SUBSCRIPT;r.indices[r.rank++]=(uint32_t)integer;
   if(accept(p,')'))break;if(!accept(p,','))return N32_SYNTAX;
  }while(true);}
 *result=r;return N32_OK;
}
static int count(const N32Value *value,int32_t low,int32_t high,int32_t *result){int status=n32_to_integer(value,result);return status?status:*result<low||*result>high?N32_DOMAIN:N32_OK;}
static int function(Parser *p,uint8_t token,N32Value *result){
 if(!accept(p,'('))return N32_SYNTAX;N32Value args[3]={0};unsigned n=0;int status=N32_OK;
 do{if(n==3){status=N32_SYNTAX;goto done;}status=expression(p,1,&args[n]);if(status)goto done;n++;if(accept(p,')'))break;if(!accept(p,',')){status=N32_SYNTAX;goto done;}}while(true);
 if(token==T_LEFT||token==T_RIGHT||token==T_MID){
  if((token!=T_MID&&n!=2)||(token==T_MID&&(n<2||n>3))){status=N32_SYNTAX;goto done;}
  if(args[0].type!=N32_STRING){status=N32_TYPE;goto done;}int32_t amount,start;size_t length=args[0].as.string.length;
  status=count(&args[1],token==T_MID?1:0,255,&amount);if(status)goto done;
  if(token==T_MID){start=amount-1;amount=(int32_t)length;if(n==3)status=count(&args[2],0,255,&amount);}
  else{if((size_t)amount>length)amount=(int32_t)length;start=token==T_LEFT?0:(int32_t)length-amount;}
  if(status)goto done;if((size_t)start>length)start=(int32_t)length;if((size_t)amount>length-(size_t)start)amount=(int32_t)length-start;
  status=n32_string(amount?args[0].as.string.bytes+start:NULL,(size_t)amount,result);goto done;
 }
 if(n!=1){status=N32_SYNTAX;goto done;}N32Value *v=&args[0];
 if(token==T_LEN||token==T_ASC||token==T_VAL){
  if(v->type!=N32_STRING){status=N32_TYPE;goto done;}
  if(token==T_LEN)*result=n32_integer(v->as.string.length);
  else if(token==T_ASC){if(!v->as.string.length){status=N32_DOMAIN;goto done;}*result=n32_integer(v->as.string.bytes[0]);}
  else{size_t start=0;while(start<v->as.string.length&&v->as.string.bytes[start]==' ')start++;bool negative=false;
   if(start<v->as.string.length&&(v->as.string.bytes[start]=='+'||v->as.string.bytes[start]=='-'))negative=v->as.string.bytes[start++]=='-';
   size_t used;status=n32_parse_number(v->as.string.bytes+start,v->as.string.length-start,&used,result);
   if(status==N32_SYNTAX){*result=n32_integer(0);status=N32_OK;}else if(!status&&negative)status=n32_unary('-',result,result);
  }goto done;
 }
 if(token==T_CHR){int32_t byte;status=count(v,0,255,&byte);if(!status){uint8_t ch=(uint8_t)byte;status=n32_string(&ch,1,result);}goto done;}
 if(v->type==N32_STRING){status=N32_TYPE;goto done;}
 if(token==T_STR){char text[48];size_t length;status=n32_format(v,text,sizeof text,&length);if(!status){uint8_t bytes[49];size_t pad=text[0]=='-'?0:1;if(pad)bytes[0]=' ';memcpy(bytes+pad,text,length);status=n32_string(bytes,length+pad,result);}goto done;}
 if(token==T_CINT||token==T_CSNG||token==T_CDBL){status=n32_convert(v,token==T_CINT?N32_INTEGER:token==T_CSNG?N32_SINGLE:N32_DOUBLE,result);goto done;}
 double real=n32_real(v);
 if(token>=T_SQR&&token<=T_LOG){status=n32_math(token,v,result);}
 else if(token==T_ABS){if(real<0)status=n32_unary('-',v,result);else *result=*v;}
 else if(token==T_SGN)*result=n32_integer(real>0?1:real<0?-1:0);
 else if(token==T_INT||token==T_FIX){
  if(v->type==N32_INTEGER){*result=*v;goto done;}
  N32Value whole=n32_double(real);uint64_t bits=((uint64_t)whole.as.wide.high<<32)|whole.as.wide.low;unsigned exponent=(unsigned)((bits>>52)&2047);
  if(exponent<1023)bits&=UINT64_C(0x8000000000000000);
  else if(exponent<1075)bits&=~((UINT64_C(1)<<(1075-exponent))-1);
  whole.as.wide.low=(uint32_t)bits;whole.as.wide.high=(uint32_t)(bits>>32);double truncated=n32_real(&whole);
  if(token==T_INT&&real<truncated)whole=n32_double(truncated-1);
  int32_t integer;status=n32_to_integer(&whole,&integer);
  if(!status)*result=n32_integer(integer);
  else if(status==N32_OVERFLOW)status=n32_convert(&whole,v->type,result);
 }else if(token==T_RND){int32_t bound;status=count(v,0,INT32_MAX,&bound);if(status)goto done;
  /* Native32 deterministic xorshift, not the original24-bit recurrence. A
   * SINGLE fraction uses its24 significant bits; positive bounds use integer
   * multiply-high and cannot overflow a32-bit signed result. */
  uint32_t state=p->machine->random;state^=state<<13;state^=state>>17;state^=state<<5;p->machine->random=state;
  if(!bound)*result=n32_single((float)(state>>8)*(1.0f/16777216.0f));
  else *result=n32_integer((int32_t)(((uint64_t)state*(uint32_t)bound)>>32)+1);
 }else status=N32_UNSUPPORTED;
done:for(unsigned i=0;i<n;i++)n32_clear(&args[i]);return status;
}
static int primary(Parser *p,N32Value *result){
 uint8_t ch=peek(p);int status;
 if(ch=='+'||ch=='-'||ch==T_NOT){p->at++;N32Value value={0};status=expression(p,ch==T_NOT?3:6,&value);if(!status)status=n32_unary(ch,&value,result);n32_clear(&value);return status;}
 if(accept(p,'(')){status=expression(p,1,result);if(status)return status;if(!accept(p,')')){n32_clear(result);return N32_SYNTAX;}return N32_OK;}
 if(accept(p,'"')){size_t start=p->at;while(p->at<p->length&&p->text[p->at]!='"')p->at++;if(p->at==p->length)return N32_SYNTAX;size_t length=p->at-start;p->at++;return n32_string(p->text+start,length,result);}
 if((ch>='0'&&ch<='9')||ch=='.'){size_t used;status=n32_parse_number(p->text+p->at,p->length-p->at,&used,result);if(!status)p->at+=used;return status;}
 if(alpha(ch)){N32Reference r;status=reference(p,&r);return status?status:n32_get(p->machine,&r,result);}
 if((ch>=T_ABS&&ch<=T_RND)||(ch>=T_SQR&&ch<=T_LOG)){p->at++;return function(p,ch,result);}
 return N32_SYNTAX;
}
static unsigned precedence(uint8_t token){return token==T_OR?1:token==T_AND?2:token=='<'||token=='>'||token=='='?3:token=='+'||token=='-'?4:token=='*'||token=='/'?5:token=='^'||token=='['?7:0;}
static int power(const N32Value *a,const N32Value *b,N32Value *result){
 int32_t exponent;int status=n32_to_integer(b,&exponent);if(status==N32_OVERFLOW)return n32_math_power(a,b,result);if(status)return status;if(n32_real(b)!=exponent)return n32_math_power(a,b,result);
 bool negative=exponent<0;uint32_t bits=negative?0u-(uint32_t)exponent:(uint32_t)exponent;
 N32Value answer=n32_integer(1),factor=*a;
 while(bits){if(bits&1){status=n32_binary('*',&answer,&factor,&answer);if(status)return status;}bits>>=1;if(bits){status=n32_binary('*',&factor,&factor,&factor);if(status)return status;}}
 if(negative){N32Value one=n32_integer(1);return n32_binary('/',&one,&answer,result);}*result=answer;return N32_OK;
}
static int expression(Parser *p,unsigned minimum,N32Value *result){
 if(p->depth==N32_DEPTH)return N32_LIMIT;p->depth++;N32Value left={0};int status=primary(p,&left);if(status)goto done;
 for(;;){uint8_t op=peek(p);unsigned rank=precedence(op);if(rank<minimum||!rank)break;p->at++;unsigned relation=0;
  if(rank==3){relation=op=='<'?1:op=='='?2:4;uint8_t next=peek(p);if(next=='<'||next=='>'||next=='='){unsigned bit=next=='<'?1:next=='='?2:4;if(relation&bit){status=N32_SYNTAX;break;}relation|=bit;p->at++;}}
  N32Value right={0},answer={0};status=expression(p,rank+1,&right);if(status)break;
  if(relation){int order;status=n32_compare(&left,&right,&order);if(!status)answer=n32_integer(relation&(order<0?1:order==0?2:4)?-1:0);}
  else if(op=='^'||op=='['){if(left.type==N32_STRING||right.type==N32_STRING)status=N32_TYPE;else status=power(&left,&right,&answer);}
  else status=n32_binary(op,&left,&right,&answer);
  n32_clear(&right);if(status)break;n32_clear(&left);left=answer;
 }
 if(!status){*result=left;left=n32_integer(0);}
done:n32_clear(&left);p->depth--;return status;
}
int n32_expression(N32Machine *m,const uint8_t *text,size_t length,size_t *used,N32Value *result){
 Parser p={.machine=m,.text=text,.length=length};N32Value value={0};int status=expression(&p,1,&value);if(!status){peek(&p);*used=p.at;*result=value;}return status;
}
int n32_reference(N32Machine *m,const uint8_t *text,size_t length,size_t *used,N32Reference *result){Parser p={.machine=m,.text=text,.length=length};int status=reference(&p,result);if(!status){peek(&p);*used=p.at;}return status;}
