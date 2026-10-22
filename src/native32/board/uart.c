/* Minimal polled16550 binding. This map matches a common virt UART, but the
 * firmware still requires a machine providing the stated flash/RAM addresses.
 * Board I/O is separate from the interpreter and its numeric representation. */
#include "native.h"
#include <string.h>
static volatile uint8_t *const uart=(volatile uint8_t *)(uintptr_t)0x10000000;
static N32Machine machine;
static int get_byte(void){static bool after_cr;for(;;){__asm__ volatile("fence iorw,iorw":::"memory");if(!(uart[5]&1))return -1;uint8_t byte=uart[0];if(after_cr&&byte=='\n'){after_cr=false;continue;}after_cr=byte=='\r';return byte=='\r'?'\n':byte;}}
static void put_byte(uint8_t byte){while(!(uart[5]&0x20)){}__asm__ volatile("fence iorw,iorw":::"memory");uart[0]=byte;}
static int write_bytes(void *context,const uint8_t *bytes,size_t length){(void)context;for(size_t i=0;i<length;i++){if(bytes[i]=='\n')put_byte('\r');put_byte(bytes[i]);}return N32_OK;}
static int read_line(void *context,uint8_t *bytes,size_t capacity,size_t *length){
 (void)context;static uint8_t line[N32_LINE_LIMIT];static size_t used;static bool overflow;int input;
 while((input=get_byte())>=0){uint8_t byte=(uint8_t)input;if(byte=='\n'){write_bytes(NULL,(const uint8_t *)"\n",1);if(used>capacity){used=0;overflow=false;return N32_LIMIT;}memcpy(bytes,line,used);*length=used;used=0;bool bad=overflow;overflow=false;return bad?N32_LIMIT:N32_OK;}
  if(byte==8||byte==127){if(used){used--;write_bytes(NULL,(const uint8_t *)"\b \b",3);}}
  else if(used<sizeof line){line[used++]=byte;write_bytes(NULL,&byte,1);}else overflow=true;
 }return N32_WAIT;
}
_Noreturn void native32_rom_main(void){
 uart[1]=0;uart[3]=0x80;uart[0]=1;uart[1]=0;uart[3]=3;uart[2]=7;
 n32_init(&machine,write_bytes,read_line,NULL);write_bytes(NULL,(const uint8_t *)"NATIVE32 BASIC\nREADY\n>",22);
 for(;;){if(machine.running){int status=n32_step(&machine,128);if(status&&status!=N32_WAIT){write_bytes(NULL,(const uint8_t *)"?",1);const char *error=n32_error(status);write_bytes(NULL,(const uint8_t *)error,strlen(error));write_bytes(NULL,(const uint8_t *)" Error\n",7);}if(!machine.running)write_bytes(NULL,(const uint8_t *)"READY\n>",7);}
  else{int input=get_byte();if(input>=0)(void)n32_feed(&machine,(uint8_t)input);}}
}
