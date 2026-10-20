/* Loader tests use a real linker-produced PIE, then corrupt individual fields.
   This is independent of the instruction-boundary tests in rv_cpu.c. */
#define RV_NO_MAIN
#include "../src/rv/rv64.c"
#include <assert.h>
static unsigned char original[ELF_LIMIT];
static unsigned file_length,dynamic_file,rela_file,rela_tag,rela_size_tag,rela_entry_tag;
static void patch(unsigned offset,uint64_t value,unsigned width){for(unsigned index=0;index<width;index++)elf_image[offset+index]=(unsigned char)(value>>(index*8));}
static void restore(void){memcpy(elf_image,original,file_length);}
static int boot(uint64_t base){return boot_elf_at(file_length,1,(const char *const[]){"pie-test"},base);}
static void rejected(uint64_t base,const char *message){assert(!boot(base));assert(state==FAULTED);assert(strstr(fault_message,message));}
static unsigned file_offset(uint64_t virtual){
    uint64_t headers=elf_value(32,8,file_length),size=elf_value(54,2,file_length),count=elf_value(56,2,file_length);
    for(unsigned index=0;index<count;index++){
        unsigned head=(unsigned)(headers+index*size);uint64_t address=elf_value(head+16,8,file_length),length=elf_value(head+32,8,file_length);
        if(elf_value(head,4,file_length)==1&&virtual>=address&&virtual-address<length)return (unsigned)(elf_value(head+8,8,file_length)+virtual-address);
    }assert(!"fixture address not in file");return 0;
}
int main(int argc,char **argv){
    FILE *input=fopen(argc>1?argv[1]:"build/pie-guest","rb");assert(input);file_length=(unsigned)fread(original,1,sizeof original,input);fclose(input);restore();
    uint64_t headers=elf_value(32,8,file_length),size=elf_value(54,2,file_length),count=elf_value(56,2,file_length);
    for(unsigned index=0;index<count;index++){unsigned head=(unsigned)(headers+index*size);if(elf_value(head,4,file_length)==2)dynamic_file=(unsigned)elf_value(head+8,8,file_length);}
    assert(dynamic_file);
    for(unsigned pos=dynamic_file;elf_value(pos,8,file_length);pos+=16){uint64_t tag=elf_value(pos,8,file_length);if(tag==7){rela_tag=pos;rela_file=file_offset(elf_value(pos+8,8,file_length));}if(tag==8)rela_size_tag=pos;if(tag==9)rela_entry_tag=pos;}
    assert(rela_file&&rela_tag&&rela_size_tag&&rela_entry_tag);
    for(unsigned index=0;index<2;index++){
        restore();uint64_t bias=index?0xc00000:0x400000;assert(boot(bias));assert(load_bias==bias);
        uint64_t relocation=elf_value(rela_file,8,file_length),addend=elf_value(rela_file+16,8,file_length);
        assert(load(bias+relocation,8)==bias+addend);
        for(unsigned tick=0;tick<10000&&state==RUNNING;tick++)step();
        assert(state==EXITED&&exit_status==0);
    }
    puts("PASS: real static PIE relocates function/data pointers and startup state at two bases");
    restore();rejected(0x400001,"load bias");restore();rejected(0x2000000,"load bias");
    restore();patch(rela_file+8,2,8);rejected(0x400000,"unsupported PIE relocation");
    restore();patch(rela_file+8,(UINT64_C(1)<<32)|3,8);rejected(0x400000,"unsupported PIE relocation");
    restore();patch(rela_file,UINT64_MAX-3,8);rejected(0x400000,"destination overflow");
    restore();patch(rela_file,elf_value(24,8,file_length),8);rejected(0x400000,"destination/addend");
    restore();patch(rela_file+16,UINT64_MAX-0x500000,8);rejected(0x400000,"destination/addend");
    restore();patch(rela_size_tag+8,25,8);rejected(0x400000,"RELA table");
    restore();patch(rela_entry_tag+8,16,8);rejected(0x400000,"RELA table");
    restore();patch(rela_tag+8,RAM_SIZE,8);rejected(0x400000,"RELA table");
    restore();patch(dynamic_file,1,8);rejected(0x400000,"dependencies");
    restore();patch(dynamic_file,7,8);patch(dynamic_file+8,0,8);rejected(0x400000,"duplicate DT_RELA");
    puts("PASS: invalid bias, symbol relocations, text/OOB/negative writes, malformed tables and DT_NEEDED rejected");
    /* Failed validation must not partially install earlier valid relocations. */
    restore();uint64_t first=elf_value(rela_file,8,file_length);patch(rela_file+24+8,2,8);
    rejected(0x400000,"unsupported PIE relocation");assert(load(0x400000+first,8)==elf_value(file_offset(first),8,file_length));
    puts("PASS: validate-all-before-write prevents partially applied relocation tables");
    return 0;
}
