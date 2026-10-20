/* Differential execution against superzazu/z80 (MIT), not used in the port. */
#include "reference/z80.h"
#define TESTING
#include "../src/runtime.c"
#undef a
#undef f
#undef b
#undef c
#undef d
#undef e
#undef h
#undef l
#undef af
#undef bc
#undef de
#undef hl
static uint8_t reference_mem[65536];
static uint8_t read_reference(void *ctx,uint16_t addr){(void)ctx;if(addr>=0x3800&&addr<0x3c00)return 0;return reference_mem[addr];}
static void write_reference(void *ctx,uint16_t addr,uint8_t value){(void)ctx;if(addr>=0x3c00)reference_mem[addr]=value;}
static uint8_t port_reference(z80 *cpu,uint8_t port){(void)cpu;(void)port;return 0;}
static void out_reference(z80 *cpu,uint8_t port,uint8_t value){(void)cpu;(void)port;(void)value;}
static unsigned flags_reference(z80 *cpu){return cpu->sf*128+cpu->zf*64+cpu->yf*32+cpu->hf*16+cpu->xf*8+cpu->pf*4+cpu->nf*2+cpu->cf;}
static void check(const char *name,unsigned actual,unsigned expected,unsigned previous){
    if(actual!=expected){fprintf(stderr,"step=%"PRIu64" instruction=%04x %s: translated=%04x reference=%04x\n",steps,previous,name,actual,expected);exit(1);}
}
int main(void){
    z80 cpu;memset(&cpu,0,sizeof cpu);cpu.read_byte=read_reference;cpu.write_byte=write_reference;cpu.port_in=port_reference;cpu.port_out=out_reference;
    memcpy(mem,rom_image,sizeof rom_image);memset(mem+0x3c00,' ',1024);memcpy(reference_mem,mem,sizeof mem);
    input="\r\rPRINT 2+2\r10 FOR I=1 TO 5\r20 PRINT I*I\r30 NEXT I\rRUN\rPRINT SQR(81),SIN(0),2^10\r";
    for(steps=0;steps<10000000;steps++){
        unsigned previous=pc;
        if(pc==0x33){pc=pop();cpu.pc=reference_mem[cpu.sp]|(reference_mem[(uint16_t)(cpu.sp+1)]<<8);cpu.sp+=2;continue;}
        if(pc==0x2b){
            if(rd16(sp)!=0x004c){reg_af.hi=cpu.a=0;pc=pop();cpu.pc=reference_mem[cpu.sp]|(reference_mem[(uint16_t)(cpu.sp+1)]<<8);cpu.sp+=2;continue;}
            if(!input[input_pos])break;
            reg_af.hi=cpu.a=(uint8_t)input[input_pos++];pc=pop();cpu.pc=reference_mem[cpu.sp]|(reference_mem[(uint16_t)(cpu.sp+1)]<<8);cpu.sp+=2;continue;
        }
        translated_step();z80_step(&cpu);
        check("pc",pc,cpu.pc,previous);check("af",reg_af.w,(cpu.a<<8)|flags_reference(&cpu),previous);
        check("bc",reg_bc.w,(cpu.b<<8)|cpu.c,previous);check("de",reg_de.w,(cpu.d<<8)|cpu.e,previous);check("hl",reg_hl.w,(cpu.h<<8)|cpu.l,previous);
        check("r",refresh,cpu.r,previous);check("i",ir,cpu.i,previous);check("im",im,cpu.interrupt_mode,previous);
        check("ix",ix,cpu.ix,previous);check("iy",iy,cpu.iy,previous);check("sp",sp,cpu.sp,previous);
        check("af'",af2,(cpu.a_<<8)|cpu.f_,previous);check("bc'",bc2,(cpu.b_<<8)|cpu.c_,previous);
        check("de'",de2,(cpu.d_<<8)|cpu.e_,previous);check("hl'",hl2,(cpu.h_<<8)|cpu.l_,previous);
        if(steps%1000==0&&memcmp(mem,reference_mem,sizeof mem)){fprintf(stderr,"memory mismatch after %04x\n",previous);return 1;}
    }
    if(steps==10000000||memcmp(mem,reference_mem,sizeof mem))return 1;
    printf("PASS: %"PRIu64" translated instructions/events in lockstep; %zu input bytes; complete memory matches\n",steps,input_pos);
    return 0;
}
