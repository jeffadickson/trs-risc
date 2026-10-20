/* Targeted ISA boundary tests, separate from the full ELF/QEMU comparison. */
#define RV_NO_MAIN
#include "../src/rv/rv64.c"
#include <assert.h>
static uint32_t r_type(unsigned top,unsigned rs2,unsigned rs1,unsigned kind,unsigned rd,unsigned op){return top<<25|rs2<<20|rs1<<15|kind<<12|rd<<7|op;}
static uint32_t i_type(int imm,unsigned rs1,unsigned kind,unsigned rd,unsigned op){return ((uint32_t)imm&4095)<<20|rs1<<15|kind<<12|rd<<7|op;}
static void fresh(void){memset(registers,0,sizeof registers);state=RUNNING;guest_pc=4096;instructions=0;reservation_valid=0;}
static void instruction(uint32_t value){
    /* Host writes of test instructions must not break the guest LR reservation. */
    for(unsigned index=0;index<4;index++)guest_ram[guest_pc+index]=(unsigned char)(value>>(index*8));
    step();assert(state==RUNNING);assert(registers[0]==0);
}
int main(void){
    fresh();instruction(i_type(-1,0,0,1,0x13));assert(registers[1]==UINT64_MAX);
    instruction(i_type(7,0,0,0,0x13));assert(registers[0]==0);
    instruction(i_type(63,1,1,2,0x13));assert(registers[2]==UINT64_C(0x8000000000000000));
    instruction(i_type(0x43f,2,5,3,0x13));assert(registers[3]==UINT64_MAX); /* SRAI 63 */
    instruction(i_type(63,2,5,4,0x13));assert(registers[4]==1);
    instruction(i_type(0,1,0,5,0x1b));assert(registers[5]==UINT64_MAX); /* ADDIW */
    fresh();registers[1]=0x80000000;registers[2]=0;
    instruction(r_type(0,2,1,0,3,0x3b));assert(registers[3]==UINT64_C(0xffffffff80000000));
    fresh();registers[1]=UINT64_C(0x8000000000000000);registers[2]=UINT64_MAX;
    instruction(r_type(1,2,1,4,3,0x33));assert(registers[3]==registers[1]); /* DIV overflow */
    instruction(r_type(1,2,1,6,3,0x33));assert(registers[3]==0); /* REM overflow */
    registers[2]=0;instruction(r_type(1,2,1,4,3,0x33));assert(registers[3]==UINT64_MAX);
    instruction(r_type(1,2,1,6,3,0x33));assert(registers[3]==registers[1]);
    registers[1]=UINT64_MAX;registers[2]=3;
    instruction(r_type(1,2,1,1,3,0x33));assert(registers[3]==UINT64_MAX); /* MULH -1*3 */
    registers[2]=UINT64_MAX;
    instruction(r_type(1,2,1,2,3,0x33));assert(registers[3]==UINT64_MAX); /* MULHSU: -(2^64-1), high half is all ones */
    instruction(r_type(1,2,1,3,3,0x33));assert(registers[3]==UINT64_C(0xfffffffffffffffe)); /* MULHU */
    fresh();registers[1]=0x80000000;registers[2]=UINT32_MAX;
    instruction(r_type(1,2,1,4,3,0x3b));assert(registers[3]==UINT64_C(0xffffffff80000000));
    registers[2]=1;instruction(r_type(1,2,1,5,3,0x3b));assert(registers[3]==UINT64_C(0xffffffff80000000)); /* DIVUW sign extension */
    fresh();registers[1]=8192;store(8192,0x80000000,4);
    instruction(i_type(0,1,2,2,3));assert(registers[2]==UINT64_C(0xffffffff80000000));
    instruction(i_type(0,1,6,2,3));assert(registers[2]==0x80000000); /* LWU */
    registers[3]=99;
    instruction(r_type(2<<2,0,1,2,2,0x2f));assert(registers[2]==UINT64_C(0xffffffff80000000)); /* LR.W */
    instruction(r_type(3<<2,3,1,2,2,0x2f));assert(registers[2]==0&&load(8192,4)==99); /* SC.W */
    instruction(r_type(3<<2,3,1,2,2,0x2f));assert(registers[2]==1); /* reservation consumed */
    registers[3]=1;instruction(r_type(0,3,1,2,2,0x2f));assert(registers[2]==99&&load(8192,4)==100); /* AMOADD.W */
    fresh();registers[10]=0;registers[11]=8192;registers[12]=1;registers[17]=63;
    for(unsigned index=0;index<4;index++)guest_ram[4096+index]=(0x00000073u>>(index*8))&255;
    step();assert(state==WAITING&&guest_pc==4096);rv_enqueue('X');step();assert(state==RUNNING&&guest_pc==4100&&guest_ram[8192]=='X');
    fresh();load(RAM_SIZE-1,8);assert(state==FAULTED); /* no host OOB */
    fresh();guest_ram[4096]=0;step();assert(state==FAULTED); /* illegal/C opcode */
    fresh();memset(elf_image,0,64);assert(!boot_elf(64,1,(const char *const[]){"test"}));assert(state==FAULTED);
    puts("PASS: RV64 register/shift/word/M/atomic/ECALL/bounds/invalid-ELF edge cases");return 0;
}
