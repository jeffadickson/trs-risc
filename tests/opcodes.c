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
static uint32_t seed=0x80c0ffee;
static unsigned random_value(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
int main(void){
    unsigned count=0;
    for(unsigned entry=0;entry<7*256;entry++)for(unsigned trial=0;trial<16;trial++){
        unsigned family=entry/256,opcode=entry%256;
        // Reference does not implement full block-I/O flags or undefined EDs.
        if((family==0||family==3||family==4)&&opcode==0xed)continue;
        if(family==2 && (!(opcode>=0x40&&opcode<=0x7e&&opcode!=0x77&&opcode!=0x4e&&opcode!=0x6e) &&
           !((opcode&0xe4)==0xa0 && (opcode&3)<2)))continue;
        z80 cpu;memset(&cpu,0,sizeof cpu);cpu.read_byte=read_reference;cpu.write_byte=write_reference;cpu.port_in=port_reference;cpu.port_out=out_reference;
        for(unsigned addr=0;addr<65536;addr++)mem[addr]=(uint8_t)(addr*17+trial);
        memcpy(mem,rom_image,sizeof rom_image);memcpy(reference_mem,mem,sizeof mem);
        reg_af.w=random_value();reg_bc.w=random_value();reg_de.w=random_value();reg_hl.w=random_value();
        ix=random_value();iy=random_value();sp=0xf000;pc=entry*8;halted=0;
        af2=random_value();bc2=random_value();de2=random_value();hl2=random_value();ir=trial;refresh=0;im=0;iff1=iff2=trial&1;
        cpu.pc=pc;cpu.sp=sp;cpu.a=reg_af.hi;unsigned fl=reg_af.lo;
        cpu.sf=!!(fl&S);cpu.zf=!!(fl&Z);cpu.yf=!!(fl&Y);cpu.hf=!!(fl&H);cpu.xf=!!(fl&X);cpu.pf=!!(fl&P);cpu.nf=!!(fl&N);cpu.cf=!!(fl&C);
        cpu.b=reg_bc.hi;cpu.c=reg_bc.lo;cpu.d=reg_de.hi;cpu.e=reg_de.lo;cpu.h=reg_hl.hi;cpu.l=reg_hl.lo;
        cpu.ix=ix;cpu.iy=iy;cpu.i=ir;cpu.iff1=iff1;cpu.iff2=iff2;
        cpu.a_=af2>>8;cpu.f_=af2;cpu.b_=bc2>>8;cpu.c_=bc2;cpu.d_=de2>>8;cpu.e_=de2;cpu.h_=hl2>>8;cpu.l_=hl2;
        steps=count++;unsigned previous=pc;
        translated_step();z80_step(&cpu);
        check("pc",pc,cpu.pc,previous);
        // Compare documented flags: the reference omits X/Y for several EDs
        // and BIT (HL) depends on the unmodeled hidden WZ latch.
        unsigned mask=255&~(X|Y);
        check("af",reg_af.w&((mask)|0xff00),((cpu.a<<8)|flags_reference(&cpu))&((mask)|0xff00),previous);
        check("bc",reg_bc.w,(cpu.b<<8)|cpu.c,previous);check("de",reg_de.w,(cpu.d<<8)|cpu.e,previous);check("hl",reg_hl.w,(cpu.h<<8)|cpu.l,previous);
        check("r",refresh,cpu.r,previous);check("i",ir,cpu.i,previous);check("im",im,cpu.interrupt_mode,previous);
        check("ix",ix,cpu.ix,previous);check("iy",iy,cpu.iy,previous);check("sp",sp,cpu.sp,previous);
        check("af'",af2,(cpu.a_<<8)|cpu.f_,previous);check("bc'",bc2,(cpu.b_<<8)|cpu.c_,previous);
        check("de'",de2,(cpu.d_<<8)|cpu.e_,previous);check("hl'",hl2,(cpu.h_<<8)|cpu.l_,previous);
        if(memcmp(mem,reference_mem,sizeof mem)){fprintf(stderr,"memory mismatch at %04x\n",previous);return 1;}
    }
    printf("PASS: %u differential single-instruction cases (7 opcode families, 16 register states; documented flags)\n",count);
    return 0;
}
