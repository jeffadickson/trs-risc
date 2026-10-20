/*
 * TRS-RISC: the host environment for native-translated Model I Level II BASIC.
 *
 * This file supplies Z80 register/flag semantics and machine I/O, not a new
 * BASIC interpreter. Original ROM instructions live in generated translated.inc.
 * Comments below pair each helper with the Z80 instructions it implements.
 * Helpers shared by many instructions have no single original ROM address;
 * only verified fixed entry points and copied RAM stubs are given addresses.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error This register layout requires a little-endian host (including RISC-V).
#endif
/* Z80 F register, low to high: C N P/V X H Y Z S. */
enum { C=1,N=2,P=4,X=8,H=16,Y=32,Z=64,S=128 };
/* Z80 AF/BC/DE/HL alias their two eight-bit halves. Both RV64 Linux and
 * the development host are little endian; the guard above enforces this. */
typedef union { uint16_t w; struct {uint8_t lo,hi;}; } Pair;
static Pair reg_af,reg_bc,reg_de,reg_hl;
#define af reg_af.w
#define a reg_af.hi
#define f reg_af.lo
#define bc reg_bc.w
#define b reg_bc.hi
#define c reg_bc.lo
#define de reg_de.w
#define d reg_de.hi
#define e reg_de.lo
#define hl reg_hl.w
#define h reg_hl.hi
#define l reg_hl.lo
/* EX AF,AF' and EXX swap these alternate banks; IX/IY/SP/PC remain
 * independent. I/R and IFF state are modeled without external interrupts. */
static uint16_t af2,bc2,de2,hl2,ix,iy,sp,pc;
static uint8_t ir,refresh,iff1,iff2,im,halted,mem[65536];
static uint64_t steps,hooks;
static const char *input="\r\rPRINT 2+2\r";
static size_t input_pos;
static int terminal_io=1,trace=0,interactive=0;
/* Host-side failure, with no Z80 opcode equivalent. Unsupported RAM code must
 * fail visibly instead of silently falling back to an unverified interpreter. */
static void fail(const char *why,unsigned addr){fprintf(stderr,"%s at %04x after %"PRIu64" steps\n",why,addr,steps);exit(2);}
/* Z80 examples: 7E LD A,(HL); 3A nn nn LD A,(nn).
 * Return memory through the Model I map. Keyboard matrix reads at 3800h..3BFFh
 * return no key; scripted/interactive input is connected at ROM CALL 002Bh. */
static uint8_t rd(uint16_t addr){
    if(addr>=0x3800 && addr<0x3c00)return 0; /* no physical keys pressed */
    return mem[addr];
}
/* Z80 examples: 77 LD (HL),A; 32 nn nn LD (nn),A.
 * Ignore ROM/peripheral writes below 3C00h. Video RAM starts at 3C00h and
 * writable BASIC RAM at 4000h. uint16_t addresses preserve Z80 wraparound. */
static void wr(uint16_t addr,uint8_t value){if(addr>=0x3c00)mem[addr]=value;}
/* Z80 example: 2A lo hi / LD HL,(nn).
 * Low byte is at nn, high byte at nn+1 (wrapping FFFFh to 0000h). Never read
 * a native uint16_t directly: alignment and the emulated memory map matter. */
static uint16_t rd16(uint16_t addr){return rd(addr)|((uint16_t)rd(addr+1)<<8);}
/* Z80 example: 22 lo hi / LD (nn),HL.
 * Store the low byte first and use the memory map for BOTH byte writes. */
static void wr16(uint16_t addr,uint16_t value){wr(addr,value);wr(addr+1,value>>8);}
/* Z80: E5 PUSH HL; CD lo hi CALL nn also pushes the return PC.
 * The stack grows down by two. The low byte ends up at the new SP, matching
 * Z80 memory layout even though the physical CPU writes the high byte first.
 * This model does not expose bus-cycle ordering. */
static void push(uint16_t value){sp-=2;wr16(sp,value);}
/* Z80: E1 POP HL; C9 RET restores PC from this same little-endian word.
 * SP advances by two after the read, with 16-bit wraparound. */
static uint16_t pop(void){uint16_t value=rd16(sp);sp+=2;return value;}
/* Z80 P/V flag for XOR, AND, OR, and CB rotates: P=1 for even parity.
 * Fold the eight operand bits by XOR. ADD/SUB use signed overflow instead;
 * parity() must NOT be used for their P/V result. */
static int parity(uint8_t value){value^=value>>4;value^=value>>2;value^=value>>1;return !(value&1);}
/* Shared flag portion for Z80 INC r, DEC r, ADD A,r and other arithmetic.
 * S follows bit 7; Z means the eight-bit result is zero; undocumented X/Y
 * follow result bits 3/5. This helper intentionally does not set P/V. */
static uint8_t sz(uint8_t value){return (value&(S|X|Y))|(!value?Z:0);}
/* Z80: A8 XOR B; A0 AND B; B0 OR B; CB 00 RLC B.
 * Combine result sign/zero/X/Y with EVEN PARITY. Carry, H and N are supplied
 * by each instruction family so preserved flags are not accidentally lost. */
static uint8_t szp(uint8_t value){return sz(value)|(parity(value)?P:0);}
/* Z80: 04 INC B / 05 DEC B, generalized to any r or mapped memory byte.
 * CARRY IS PRESERVED. H detects a bit-3 carry/borrow; P/V detects 7Fh->80h
 * for INC or 80h->7Fh for DEC; N distinguishes subtraction. */
static uint8_t incdec(uint8_t value,int sub){
    uint8_t result=value+(sub?-1:1);
    f=(f&C)|sz(result)|((value^result)&16?H:0)|(sub?N:0)|((value==(sub?0x80:0x7f))?P:0);
    return result;
}
/* Z80 y-field order: ADD A,r / ADC A,r / SUB r / SBC A,r /
 * AND r / XOR r / OR r / CP r (op 0..7). Immediates share these semantics.
 * Keep a ninth result bit for carry/borrow, then narrow the accumulator.
 * ((old ^ value ^ result) & 10h) gives half carry; sign relationships give
 * signed overflow. CP updates flags but does not change A; its X/Y bits come
 * from the compared operand, unlike ordinary arithmetic. */
static void alu(unsigned op,uint8_t value){
    uint8_t old=a,carry=(op==1||op==3)?(f&C):0,result;
    uint16_t wide;
    if(op<4||op==7){
        int sub=op==2||op==3||op==7;
        wide=sub?(uint16_t)(old-value-carry):(uint16_t)(old+value+carry);result=wide;
        f=sz(result)|((old^value^result)&16?H:0)|(wide&256?C:0)|(sub?N:0)
          |((sub?((old^value)&(old^result)):((~(old^value))&(old^result)))&128?P:0);
        if(op==7)f=(f&~(X|Y))|(value&(X|Y));else a=result;
    }else{
        a=op==4?(a&value):op==5?(a^value):(a|value);
        f=szp(a)|(op==4?H:0);
    }
}
/* Z80: 09 ADD HL,BC; DD 09 ADD IX,BC; FD 09 ADD IY,BC.
 * Preserve S/Z/PV, clear N, derive H from bit 11, C from bit 15, and X/Y
 * from the high result byte. Return the low 16 bits to the generated case. */
static uint16_t add16(uint16_t lhs,uint16_t rhs){
    uint32_t value=(uint32_t)lhs+rhs;
    f=(f&(S|Z|P))|((value>>8)&(X|Y))|((lhs^rhs^value)&0x1000?H:0)|(value&0x10000?C:0);
    return value;
}
/* Z80: ED 4A ADC HL,BC / ED 42 SBC HL,BC (sub selects subtraction).
 * Include incoming C and recompute all arithmetic flags across 16 bits.
 * This differs from ADD HL,rr, which preserves S/Z/PV. */
static uint16_t adc16(uint16_t lhs,uint16_t rhs,int sub){
    uint32_t value=sub?(uint32_t)lhs-rhs-(f&C):(uint32_t)lhs+rhs+(f&C);
    uint16_t result=value;
    f=((result>>8)&(S|X|Y))|(!result?Z:0)|((lhs^rhs^result)&0x1000?H:0)|(value&0x10000?C:0)|(sub?N:0)
      |((sub?((lhs^rhs)&(lhs^result)):((~(lhs^rhs))&(lhs^result)))&0x8000?P:0);
    return result;
}
/* Z80 CB y-field order: RLC / RRC / RL / RR / SLA / SRA / SLL / SRL.
 * RL/RR shift THROUGH incoming C; RLC/RRC recirculate the outgoing bit.
 * SRA preserves the sign bit; undocumented SLL inserts a 1 in bit zero.
 * CB forms set S/Z/PV. RLCA/RRCA/RLA/RRA call this helper but restore their
 * preserved S/Z/PV in generated code; do not conflate the two flag rules. */
static uint8_t rotate(unsigned op,uint8_t value){
    unsigned carry=0,oldcarry=f&C;uint8_t result=0;
    switch(op){
    case 0:carry=value>>7;result=(value<<1)|carry;break;
    case 1:carry=value&1;result=(value>>1)|(carry<<7);break;
    case 2:carry=value>>7;result=(value<<1)|oldcarry;break;
    case 3:carry=value&1;result=(value>>1)|(oldcarry<<7);break;
    case 4:carry=value>>7;result=value<<1;break;
    case 5:carry=value&1;result=(value>>1)|(value&128);break;
    case 6:carry=value>>7;result=(value<<1)|1;break;
    case 7:carry=value&1;result=value>>1;break;
    }
    f=szp(result)|carry;return result;
}
/* Z80: 27 DAA (decimal-adjust accumulator after packed-BCD arithmetic).
 * Build the 06h/60h correction from A, H and C; N decides add vs subtract.
 * Recompute S/Z/PV/X/Y and the changed bit-4 half carry while preserving N.
 * The original ROM software floating point is separate from this BCD opcode. */
static void daa(void){
    uint8_t old=a,adjust=0,carry=f&C;
    if((f&H)||((a&15)>9))adjust|=6;
    if(carry||(a>0x99)){adjust|=0x60;carry=C;}
    a+=(f&N)?-adjust:adjust;
    f=(f&N)|carry|szp(a)|((old^a)&16?H:0);
}
/* Z80: DB n IN A,(n); ED 40 IN B,(C), etc.
 * Console prototype: all port reads return zero. Disk/cassette timing and
 * device behavior are intentionally absent; this is not hardware emulation. */
static uint8_t inport(uint16_t port){(void)port;return 0;}
/* Z80: D3 n OUT (n),A; ED 41 OUT (C),B, etc.
 * Console prototype: consume the operation without a host device side effect.
 * Register and flag behavior is still generated at the original instruction. */
static void outport(uint16_t port,uint8_t value){(void)port;(void)value;}
/* Known executable RAM, not a general RAM opcode interpreter.
 * JP nn (C3 lo hi) and RET (C9) implement initialized ROM hooks. The fixed
 * copied/self-modified routines below have explicit native handlers. Only the
 * operand bytes may change; fixed opcodes are checked before execution. */
static void ram_hook(void){
    /* Handle known executable RAM sites without a general RAM interpreter. */
    hooks++;
    refresh=(refresh&128)|((refresh+1)&127);
    /* ROM 18f7..1904 is copied to 4080..408d. BASIC patches the
       four immediate operands for its floating-point subtraction routine.
       Translate these known sites explicitly, reading only their operands. */
    uint8_t expected=0;
    switch(pc){
    case 0x4080:expected=0xd6;break;case 0x4082:expected=0x6f;break;
    case 0x4083:expected=0x7c;break;case 0x4084:expected=0xde;break;
    case 0x4086:expected=0x67;break;case 0x4087:expected=0x78;break;
    case 0x4088:expected=0xde;break;case 0x408a:expected=0x47;break;
    case 0x408b:expected=0x3e;break;case 0x4093:expected=0xdb;break;
    case 0x4096:expected=0xd3;break;case 0x40a2:expected=0xfe;break;
    case 0x40a4:expected=0xe9;break;
    }
    if(expected && mem[pc]!=expected)fail("modified translated RAM opcode",pc);
    switch(pc){
    /* Z80 4080: D6 nn SUB nn ; immediate at 4081h is patched by BASIC */
    case 0x4080:alu(2,mem[0x4081]);pc=0x4082;return;
    /* Z80 4082: 6F LD L,A */
    case 0x4082:l=a;pc++;return;
    /* Z80 4083: 7C LD A,H */
    case 0x4083:a=h;pc++;return;
    /* Z80 4084: DE nn SBC A,nn ; patched immediate at 4085h */
    case 0x4084:alu(3,mem[0x4085]);pc=0x4086;return;
    /* Z80 4086: 67 LD H,A */
    case 0x4086:h=a;pc++;return;
    /* Z80 4087: 78 LD A,B */
    case 0x4087:a=b;pc++;return;
    /* Z80 4088: DE nn SBC A,nn ; patched immediate at 4089h */
    case 0x4088:alu(3,mem[0x4089]);pc=0x408a;return;
    /* Z80 408A: 47 LD B,A */
    case 0x408a:b=a;pc++;return;
    /* Z80 408B: 3E nn LD A,nn ; patched immediate at 408Ch */
    case 0x408b:a=mem[0x408c];pc=0x408d;return;
    /* Z80 4093: DB nn IN A,(nn) */
    case 0x4093:a=inport((a<<8)|mem[0x4094]);pc=0x4095;return;
    /* Z80 4096: D3 nn OUT (nn),A */
    case 0x4096:outport((a<<8)|mem[0x4097],a);pc=0x4098;return;
    /* Z80 40A2: FE nn CP nn */
    case 0x40a2:alu(7,mem[0x40a3]);pc=0x40a4;return;
    /* Z80 40A4: E9 JP (HL) */
    case 0x40a4:pc=hl;return;
    }
    if(mem[pc]==0xc3){pc=rd16(pc+1);return;}
    if(mem[pc]==0xc9){pc=pop();return;}
    fail("unsupported RAM code",pc);
}
#include "translated.inc"

/* Host presentation helper, with no Z80 equivalent. The Z80 display path
 * writes characters through LD (nn),A into 3C00h..3FFFh, a 64x16 buffer.
 * Dump printable characters, convert display codes 00h..1Fh to @.._, and
 * trim row-end spaces. This is a diagnostic, not a graphics renderer. */
static void dump_screen(void){
    fprintf(stderr,"\n--- Model I screen ---\n");
    for(unsigned row=0;row<16;row++){
        char line[65];for(unsigned col=0;col<64;col++){
            unsigned ch=mem[0x3c00+row*64+col]&127;
            if(ch<32)ch+=64;
            line[col]=(ch>=32&&ch<127)?ch:'.';
        }line[64]=0;for(int col=63;col>=0&&line[col]==' ';col--)line[col]=0;
        fprintf(stderr,"%s\n",line);
    }
}
#ifndef TESTING
/* Host entry: load the original ROM bytes, start at Z80 PC=0000h, and execute
 * native cases until EOF, HALT, or the instruction budget. Z80 reset begins:
 *   0000 F3       DI
 *   0001 AF       XOR A
 *   0002 C3 74 06 JP 0674h
 * Terminal traps simulate CALL/RET by popping the real Z80 return address. */
int main(int argc,char **argv){
    uint64_t limit=20000000;int screen=0;
    for(int arg=1;arg<argc;arg++){
        if(!strcmp(argv[arg],"--input")&&arg+1<argc)input=argv[++arg];
        else if(!strcmp(argv[arg],"--steps")&&arg+1<argc)limit=strtoull(argv[++arg],0,0);
        else if(!strcmp(argv[arg],"--screen"))screen=1;
        else if(!strcmp(argv[arg],"--trace"))trace=1;
        else if(!strcmp(argv[arg],"--raw"))terminal_io=0;
        else if(!strcmp(argv[arg],"--interactive"))interactive=1;
        else {fprintf(stderr,"usage: %s [--input TEXT] [--steps N] [--screen] [--raw] [--trace] [--interactive]\n",argv[0]);return 2;}
    }
    memcpy(mem,rom_image,sizeof rom_image);memset(mem+0x3c00,' ',1024);
    for(steps=0;steps<limit&&!halted;steps++){
        if(trace && (steps<200 || steps%1000000==0))fprintf(stderr,"pc=%04x af=%04x bc=%04x de=%04x hl=%04x sp=%04x\n",pc,af,bc,de,hl,sp);
        /* Z80 CALL 0033h: print A, then RET. Replacing this ABI entry avoids
         * tying the terminal to Model I video timing or cursor rendering. */
        if(terminal_io && pc==0x33){
            if(a==13)putchar('\n');else if(a>=32&&a<127)putchar(a);
            fflush(stdout);pc=pop();continue;
        }
        /* ROM blocking wrapper, actual Z80 bytes:
         *   0049 CD 2B 00 CALL 002Bh
         *   004C B7       OR A
         *   004D C0       RET NZ
         *   004E 18 F9    JR 0049h
         * Only a return address of 004Ch consumes queued input. BASIC also
         * calls 002Bh to poll BREAK while a program runs; return A=0 there. */
        if(terminal_io && pc==0x2b){
            if(trace)fprintf(stderr,"KEY caller=%04x next=%u\n",rd16(sp),(unsigned char)input[input_pos]);
            if(rd16(sp)!=0x004c){a=0;pc=pop();continue;}
            if(interactive){int ch=getchar();if(ch==EOF)break;a=(uint8_t)ch;}
            else {if(!input[input_pos])break;a=(uint8_t)input[input_pos++];}
            if(a=='\n')a=13;
            pc=pop();continue;
        }
        translated_step();
    }
    if(screen)dump_screen();
    fprintf(stderr,"steps=%"PRIu64" pc=%04x ram_hooks=%"PRIu64" input=%zu/%zu\n",steps,pc,hooks,input_pos,strlen(input));
    return steps>=limit?3:0;
}
#endif
