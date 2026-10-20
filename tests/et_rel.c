/* Real assembler-produced ET_REL fixture, then independently corrupted tables.
   These tests inspect guest results and failure atomicity, not just stdout. */
#define RV_NO_MAIN
#include "../src/rv/rv64.c"
#include <assert.h>
static unsigned char original[ELF_LIMIT];
static unsigned file_length,fixture_length,headers,sections,text,rodata,data,bss,symbols,strings,rela_text,rela_data,note;
static unsigned entry_symbol,message_symbol,absolute_symbol,local_symbol;
static uint64_t value(unsigned at,unsigned width){return elf_value(at,width,file_length);}
static unsigned header(unsigned section){return headers+section*64;}
static void patch(unsigned at,uint64_t v,unsigned width){for(unsigned i=0;i<width;i++)elf_image[at+i]=(unsigned char)(v>>(i*8));}
static void restore(void){file_length=fixture_length;memcpy(elf_image,original,file_length);state=RUNNING;}
static int boot(uint64_t base){return boot_elf_at(file_length,2,(const char *const[]){"rel-test","argument"},base);}
static void rejected(uint64_t base,const char *message){
    if(boot(base)||state!=FAULTED||!strstr(fault_message,message)){fprintf(stderr,"expected %s, got %s\n",message,fault_message);assert(0);}
    /* Every rejection, including a bad final relocation, leaves code/data and
       early valid patches unpublished. Reset clears a previous successful run. */
    assert(!extent_count&&!guest_pc&&!heap_end&&!load_bias);
    for(unsigned i=0;i<80;i++)assert(!guest_ram[0x400000+i]);
    for(unsigned i=0;i<24;i++)assert(!guest_ram[0x402000+i]);
}
static unsigned named_symbol(const char *name){
    unsigned start=(unsigned)value(header(symbols)+24,8),size=(unsigned)value(header(symbols)+32,8);
    for(unsigned i=1;i<size/24;i++)if(!strcmp((char *)elf_image+strings+value(start+i*24,4),name))return i;
    assert(!"symbol missing from real fixture");return 0;
}
int main(void){
    FILE *input=fopen("build/rel-guest.o","rb");assert(input);fixture_length=(unsigned)fread(original,1,sizeof original,input);assert(!ferror(input));fclose(input);restore();
    headers=(unsigned)value(40,8);sections=(unsigned)value(60,2);
    unsigned name_table=(unsigned)value(header((unsigned)value(62,2))+24,8);
    for(unsigned i=1;i<sections;i++){
        const char *name=(char *)elf_image+name_table+value(header(i),4);
        if(!strcmp(name,".text"))text=i;if(!strcmp(name,".rodata"))rodata=i;
        if(!strcmp(name,".data"))data=i;if(!strcmp(name,".bss"))bss=i;
        if(!strcmp(name,".symtab"))symbols=i;if(!strcmp(name,".rela.text"))rela_text=i;
        if(!strcmp(name,".rela.data"))rela_data=i;
        if(!strcmp(name,".note.GNU-stack"))note=i;
    }
    assert(text&&rodata&&data&&bss&&symbols&&rela_text&&rela_data);
    unsigned symbol_file=(unsigned)value(header(symbols)+24,8);
    strings=(unsigned)value(header((unsigned)value(header(symbols)+40,4))+24,8);
    entry_symbol=named_symbol("_start");message_symbol=named_symbol("message");
    absolute_symbol=named_symbol("absolute_value");local_symbol=named_symbol("local_message");
    unsigned rt=(unsigned)value(header(rela_text)+24,8),rd=(unsigned)value(header(rela_data)+24,8);
    for(unsigned which=0;which<2;which++){
        uint64_t base=which?0xc00000:0x400000;restore();assert(boot(base));
        assert(guest_pc==base&&load_bias==base&&heap_end==base+0x5000&&extent_count==4);
        assert(load(base+64,8)==base+0x1000&&load(base+72,8)==base+0x4000);
        assert(load(base+0x2000,8)==base+0x1000-1&&load(base+0x2008,8)==0x1234&&load(base+0x2010,8)==base+0x1000);
        assert(mapped(base,80,1)&&!mapped(base,80,2)&&mapped(base+0x1000,18,4)&&!mapped(base+0x1000,18,2));
        assert(mapped(base+0x2000,24,2)&&mapped(base+0x4000,16,2));
        for(unsigned i=0;i<16;i++)assert(!guest_ram[base+0x4000+i]);
        uint64_t stack=registers[2];assert(!(stack&15)&&load(stack,8)==2);
        assert(!strcmp((char *)guest_ram+load(stack+8,8),"rel-test"));
        assert(!strcmp((char *)guest_ram+load(stack+16,8),"argument"));
        assert(!load(stack+24,8)&&!load(stack+32,8));
        assert(load(stack+40,8)==6&&load(stack+48,8)==PAGE&&load(stack+56,8)==9&&load(stack+64,8)==base&&!load(stack+72,8));
        for(unsigned tick=0;tick<100&&state==RUNNING;tick++)step();
        assert(state==EXITED&&!exit_status&&load(base+0x4000,8)==85);
    }
    puts("PASS: real ET_REL resolves globals/locals/constants, BSS, permissions and freestanding stack at two bases");
    /* A changed section alignment changes the actual relocation values. */
    restore();patch(header(rodata)+48,16384,8);assert(boot(0x400000));assert(load(0x400040,8)==0x404000&&load(0x405000,8)==0x403fff);
    restore();assert(boot(0));assert(load_bias==0x400000);
    restore();patch(header(bss)+32,0,8);patch(symbol_file+named_symbol("scratch")*24+16,0,8);
    assert(boot(0x400000));assert(load(0x400048,8)==0x404000&&extent_count==3); /* empty section anchor */
    /* Exercise a real SHN_ABS symbol even though the assembler folded its use
       into symbol-zero + addend in the unmodified fixture. */
    restore();patch(rd+24+8,((uint64_t)absolute_symbol<<32)|2,8);patch(rd+24+16,0,8);assert(boot(0x400000));assert(load(0x402008,8)==0x1234);
    restore();patch(rt+8,0,8);patch(rt+16,0,8);assert(boot(0x400000));assert(!load(0x400040,8)); /* NONE does not write */
    /* Signed addend boundaries are checked in wide arithmetic before writes. */
    restore();patch(rd+24+16,INT64_MAX,8);assert(boot(0x400000));assert(load(0x402008,8)==INT64_MAX);
    restore();patch(symbol_file+absolute_symbol*24+8,UINT64_MAX,8);patch(rd+24+8,((uint64_t)absolute_symbol<<32)|2,8);patch(rd+24+16,0,8);assert(boot(0x400000));assert(load(0x402008,8)==UINT64_MAX);
    puts("PASS: changed placement, default base, NONE, SHN_ABS and full-width data relocation values");

    restore();rejected(0x400001,"load base");restore();rejected(0x2000000,"load base");
    restore();patch(6,0,1);rejected(0x400000,"header version");
    restore();patch(24,4,8);rejected(0x400000,"entry/program");
    restore();patch(48,1,4);rejected(0x400000,"ISA/ABI");
    restore();patch(60,0,2);rejected(0x400000,"section table");
    restore();patch(60,129,2);rejected(0x400000,"section table");
    restore();patch(40,UINT64_MAX,8);rejected(0x400000,"section table");
    restore();patch(58,63,2);rejected(0x400000,"section table");
    restore();patch(headers+32,1,8);rejected(0x400000,"null section");
    restore();patch(header(text)+24,file_length,8);rejected(0x400000,"outside file");
    restore();patch(header(text)+24,0,8);rejected(0x400000,"overlaps ELF metadata");
    restore();patch(header(data)+24,value(header(text)+24,8),8);rejected(0x400000,"overlapping ET_REL file");
    restore();patch(header(bss)+48,3,8);rejected(0x400000,"section alignment");
    restore();patch(header(bss)+48,UINT64_C(1)<<63,8);rejected(0x400000,"section alignment");
    restore();patch(header(bss)+32,UINT64_MAX,8);rejected(0x400000,"image arena");
    restore();patch(header(text)+8,7,8);rejected(0x400000,"allocatable section");
    restore();patch(header(bss)+8,0x403,8);rejected(0x400000,"allocatable section"); /* TLS */
    restore();patch(header(rela_text)+4,9,4);rejected(0x400000,"metadata section"); /* REL */
    restore();patch(header(rodata),UINT32_MAX,4);rejected(0x400000,"string offset");
    restore();patch(header(symbols)+56,23,8);rejected(0x400000,"symbol table");
    restore();patch(header(symbols)+40,sections,4);rejected(0x400000,"symbol table");
    restore();patch(symbol_file+entry_symbol*24,UINT32_MAX,4);rejected(0x400000,"string offset");
    restore();unsigned name=(unsigned)value(symbol_file+entry_symbol*24,4);
    memset(elf_image+strings+name,'X',(size_t)value(header((unsigned)value(header(symbols)+40,4))+32,8)-name);
    rejected(0x400000,"unterminated/overlong");
    restore();patch(symbol_file+message_symbol*24+6,0,2);rejected(0x400000,"unresolved");
    restore();patch(symbol_file+message_symbol*24+6,0xfff2,2);rejected(0x400000,"COMMON");
    restore();patch(symbol_file+message_symbol*24+6,0xffff,2);rejected(0x400000,"symbol section");
    restore();patch(symbol_file+message_symbol*24+16,UINT64_MAX,8);rejected(0x400000,"symbol outside");
    restore();patch(symbol_file+message_symbol*24+4,0x21,1);rejected(0x400000,"binding/type"); /* weak */
    restore();patch(symbol_file+message_symbol*24,value(symbol_file+entry_symbol*24,4),4);rejected(0x400000,"duplicate");
    restore();patch(symbol_file+entry_symbol*24+8,2,8);patch(symbol_file+entry_symbol*24+16,4,8);rejected(0x400000,"_start");
    restore();patch(symbol_file+entry_symbol*24+4,0x11,1);rejected(0x400000,"_start");
    restore();elf_image[strings+value(symbol_file+entry_symbol*24,4)]='X';rejected(0x400000,"_start missing");
    restore();patch(header(rela_data)+40,0,4);rejected(0x400000,"relocation table");
    restore();patch(header(rela_data)+44,symbols,4);rejected(0x400000,"relocation table");
    restore();patch(header(rela_data)+32,25,8);rejected(0x400000,"relocation table");
    restore();patch(rd+8,((uint64_t)REL_SYMBOLS<<32)|2,8);rejected(0x400000,"symbol out of bounds");
    restore();patch(rd+8,51,8);rejected(0x400000,"only NONE/64");
    restore();patch(rd,UINT64_MAX,8);rejected(0x400000,"relocation destination");
    restore();patch(rd,1,8);rejected(0x400000,"relocation destination");
    restore();patch(rd,24,8);rejected(0x400000,"relocation destination");
    restore();patch(rd+8,0,8); /* NONE with nonzero addend */ rejected(0x400000,"NONE");
    restore();patch(rd+24,0,8);rejected(0x400000,"overlapping ET_REL relocation writes");
    restore();patch(rd+16,INT64_MIN,8);rejected(0x400000,"value overflow");
    restore();patch(symbol_file+absolute_symbol*24+8,UINT64_MAX,8);patch(rd+24+8,((uint64_t)absolute_symbol<<32)|2,8);patch(rd+24+16,1,8);rejected(0x400000,"value overflow");
    /* Malformed final relocation proves that earlier valid code/data and
       literals were not installed. Do this after a successful populated boot. */
    restore();assert(boot(0x400000));restore();patch(rd+48+8,3,8);rejected(0x400000,"only NONE/64");
    /* Resource caps must fail before scanning a hostile table. Append storage
       after the real fixture's section table, preserving every original extent. */
    restore();patch(header(symbols)+24,fixture_length,8);patch(header(symbols)+32,(REL_SYMBOLS+1)*24,8);
    file_length=fixture_length+(REL_SYMBOLS+1)*24;memset(elf_image+fixture_length,0,file_length-fixture_length);
    rejected(0x400000,"symbol table");
    restore();assert(note);patch(header(note)+4,4,4);patch(header(note)+8,0x40,8);patch(header(note)+24,fixture_length,8);
    patch(header(note)+32,(REL_PATCHES+1)*24,8);patch(header(note)+40,symbols,4);patch(header(note)+44,text,4);patch(header(note)+56,24,8);
    file_length=fixture_length+(REL_PATCHES+1)*24;memset(elf_image+fixture_length,0,file_length-fixture_length);
    rejected(0x400000,"too many ET_REL relocations");
    puts("PASS: malformed ET_REL headers/sections/symbols/relocations fail before section or patch publication");
    return 0;
}
