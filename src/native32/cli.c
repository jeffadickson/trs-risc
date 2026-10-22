/* Hosted file/terminal adapter. Interpreter code is independent of stdio. */
#include "native.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int write_bytes(void *unused,const uint8_t *bytes,size_t length){(void)unused;return fwrite(bytes,1,length,stdout)==length?N32_OK:N32_IO;}
/* One CR/LF normalizer serves both command input and INPUT responses. */
static int next_byte(void){static bool after_cr;for(;;){int ch=getchar();if(ch==EOF)return EOF;if(after_cr&&ch=='\n'){after_cr=false;continue;}after_cr=ch=='\r';return ch=='\r'?'\n':ch;}}
static int read_line(void *unused,uint8_t *bytes,size_t capacity,size_t *length){(void)unused;size_t used=0;int ch;bool overflow=false;
 while((ch=next_byte())!=EOF&&ch!='\n'){if(used<capacity)bytes[used++]=(uint8_t)ch;else overflow=true;}
 if(ch==EOF&&!used)return N32_IO;*length=used;return overflow?N32_LIMIT:N32_OK;
}

int main(int argc,char **argv){
 N32Machine m;n32_init(&m,write_bytes,read_line,NULL);int status=N32_OK;
 if(argc==3&&!strcmp(argv[1],"eval")){uint8_t tokens[N32_LINE_LIMIT];size_t length,used;N32Value value={0};status=n32_tokenize(argv[2],tokens,sizeof tokens,&length);if(!status)status=n32_expression(&m,tokens,length,&used,&value);if(!status&&used!=length)status=N32_SYNTAX;
  if(!status){if(value.type==N32_STRING)write_bytes(NULL,value.as.string.bytes,value.as.string.length);else{char text[48];status=n32_format(&value,text,sizeof text,&used);if(!status)write_bytes(NULL,(const uint8_t *)text,used);}putchar('\n');}n32_clear(&value);
 }else if(argc==3&&!strcmp(argv[1],"run")){
  FILE *file=fopen(argv[2],"rb");if(!file)status=N32_IO;else{if(fseek(file,0,SEEK_END))status=N32_IO;long size=ftell(file);if(size<0||size>16*1024*1024)status=N32_LIMIT;rewind(file);
   if(!status){char *source=malloc((size_t)size+1);if(!source)status=N32_MEMORY;else{if(fread(source,1,(size_t)size,file)!=(size_t)size)status=N32_IO;else status=n32_source(&m,source,(size_t)size);free(source);}}fclose(file);}
  if(!status)status=n32_start(&m,1000000);while(!status&&m.running)status=n32_step(&m,1000);
 }else if(argc==2&&!strcmp(argv[1],"console")){
  write_bytes(NULL,(const uint8_t *)"NATIVE32 BASIC\nREADY\n>",22);int ch;
  while((ch=next_byte())!=EOF){status=n32_feed(&m,(uint8_t)ch);if(status)status=N32_OK;bool was_running=m.running;while(m.running){status=n32_step(&m,1000);if(status&&status!=N32_WAIT)break;}
   if(status){fprintf(stderr,"?%s Error in %u\n",n32_error(status),m.error_line);status=N32_OK;}if(was_running&&!m.running)write_bytes(NULL,(const uint8_t *)"READY\n>",7);if(!m.running&&(ch=='\r'||ch=='\n'))fflush(stdout);
  }
 }else{fprintf(stderr,"usage: native32 eval EXPR | run FILE.bas | console\n");status=N32_DOMAIN;}
 if(status)fprintf(stderr,"?%s Error in %u\n",n32_error(status),m.error_line);n32_destroy(&m);return status?1:0;
}
