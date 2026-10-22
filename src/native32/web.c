/* Direct native-C/WASM execution adapter, explicitly NOT a RISC-V emulator.
 * The same interpreter is also cross-compiled and tested as real RV32 code.
 * Queueing plus statement-sized execution keeps the browser responsive and
 * lets INPUT wait without restarting RUN or replaying prior side effects. */
#include "native.h"
#include <string.h>
__attribute__((import_module("env"),import_name("native_write")))
extern void native_write(const uint8_t *,size_t);
static N32Machine machine;static bool initialized,after_cr;static uint8_t keys[16384];static unsigned key_read,key_write;
static int write_bytes(void *context,const uint8_t *bytes,size_t length){(void)context;native_write(bytes,length);return N32_OK;}
static int read_line(void *context,uint8_t *bytes,size_t capacity,size_t *length){
 (void)context;unsigned end=key_read;while(end!=key_write&&keys[end]!='\n')end=(end+1)%sizeof keys;if(end==key_write)return N32_WAIT;
 size_t used=0;bool overflow=false;
 while(key_read!=end){uint8_t byte=keys[key_read];key_read=(key_read+1)%sizeof keys;if(used<capacity)bytes[used++]=byte;else overflow=true;write_bytes(NULL,&byte,1);}
 key_read=(key_read+1)%sizeof keys;write_bytes(NULL,(const uint8_t *)"\n",1);*length=used;return overflow?N32_LIMIT:N32_OK;
}
void n32_web_reset(void){if(initialized)n32_destroy(&machine);n32_init(&machine,write_bytes,read_line,NULL);initialized=true;key_read=key_write=0;after_cr=false;write_bytes(NULL,(const uint8_t *)"NATIVE32 BASIC\nREADY\n>",22);}
unsigned n32_web_enqueue(unsigned byte){
 if(byte>127)return 0;if(after_cr&&byte=='\n'){after_cr=false;return 1;}
 unsigned next=(key_write+1)%sizeof keys;if(next==key_read)return 0;after_cr=byte=='\r';keys[key_write]=(uint8_t)(byte=='\r'?'\n':byte);key_write=next;return 1;
}
unsigned n32_web_run(unsigned statements){
 for(unsigned i=0;i<statements;i++){
  if(machine.running){int status=n32_step(&machine,1);if(status==N32_WAIT)return 1;
   if(status){write_bytes(NULL,(const uint8_t *)"?",1);const char *error=n32_error(status);write_bytes(NULL,(const uint8_t *)error,strlen(error));write_bytes(NULL,(const uint8_t *)" Error\n",7);}
   if(!machine.running)write_bytes(NULL,(const uint8_t *)"READY\n>",7);
  }else if(key_read!=key_write){uint8_t byte=keys[key_read];key_read=(key_read+1)%sizeof keys;(void)n32_feed(&machine,byte);}
  else return 1;
 }
 return 0;
}
uint64_t n32_web_steps(void){return machine.steps;}
