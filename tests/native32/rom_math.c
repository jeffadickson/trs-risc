#define TESTING
#include "../../src/runtime.c"
#undef a
#undef b
#undef c
#undef d
#undef e
#undef f
#undef h
#undef l
#include <math.h>
/* Test-only adapter. The firmware never links the Z80 reference runtime.
 * Boot once to install the ROM's RAM arithmetic stubs, then restore that
 * snapshot before each call. Feed exact MBF SINGLE values, not decimal text.
 * The returned double exactly represents every MBF SINGLE output, including
 * its exponent range below IEEE binary32's normal range. */
static uint8_t boot_memory[65536];static int booted;
int rom_math(unsigned entry,float value,double *result){
 if(!booted){memcpy(mem,rom_image,sizeof rom_image);pc=0;
  for(unsigned n=0;n<1000000;n++){if(pc==0x33){pc=pop();continue;}if(pc==0x2b){booted=1;break;}translated_step();}
  if(!booted)return 1;memcpy(boot_memory,mem,sizeof mem);
 }else memcpy(mem,boot_memory,sizeof mem);
 uint32_t bits;memcpy(&bits,&value,4);unsigned exponent=(bits>>23)&255;
 if(value&&(!exponent||exponent>253))return 1;
 mem[0x40af]=4;mem[0x4121]=(uint8_t)bits;mem[0x4122]=(uint8_t)(bits>>8);
 mem[0x4123]=(uint8_t)(((bits>>16)&127)|((bits>>24)&128));mem[0x4124]=value?(uint8_t)(exponent+2):0;
 reg_af.w=reg_bc.w=reg_de.w=reg_hl.w=0;sp=0xf000;push(0x5000);pc=(uint16_t)entry;
 for(unsigned n=0;n<1000000;n++){
  if(pc==0x5000){uint32_t mantissa=(mem[0x4121]|mem[0x4122]<<8|(mem[0x4123]&127)<<16)|0x800000;
   *result=mem[0x4124]?ldexp((double)mantissa,(int)mem[0x4124]-152):0;
   if(mem[0x4123]&128)*result=-*result;return 0;
  }
  if(pc==0x1e4a||pc==0x7b2||pc==0x199a)return 2;
  translated_step();
 }return 3;
}
