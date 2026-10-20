/*
 * trs-risc RV64IMA user-mode emulator, usable as C or WebAssembly.
 *
 * This CPU executes the ACTUAL Linux/RISC-V ELF produced by the project. It is
 * independent of the Z80 translator: the guest may be any supported static
 * RV64IMA soft-float ELF, not only BASIC. It does not emulate a privileged board,
 * MMU, hardware devices, threads, compressed instructions or floating point.
 *
 * All guest addresses are checked offsets into guest_ram. The Linux syscall
 * layer is deliberately small, deterministic and sandboxed: no host file access.
 * For full compatibility and performance, use QEMU; this is an inspectable toy.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define RAM_SIZE (96u*1024u*1024u)
#define ELF_LIMIT (8u*1024u*1024u)
#define STACK_TOP (RAM_SIZE-4096u)
#define PAGE 4096u
static unsigned char guest_ram[RAM_SIZE],elf_image[ELF_LIMIT],keyboard[16384];
static uint64_t registers[32],guest_pc,instructions,heap_end,map_end,reservation;
static unsigned key_read,key_write,state,exit_status,reservation_valid;
static char fault_message[256];
enum { RUNNING=0,WAITING=1,EXITED=2,FAULTED=3 };
#ifdef __wasm__
__attribute__((import_module("env"),import_name("write_bytes")))
extern void rv_output(unsigned fd,const unsigned char *bytes,unsigned length);
#else
static void rv_output(unsigned fd,const unsigned char *bytes,unsigned length){fwrite(bytes,1,length,fd==2?stderr:stdout);fflush(fd==2?stderr:stdout);}
#endif

/* Trap before touching an out-of-range host address. No guest fault can turn a
   guest offset into an unchecked host pointer. Diagnostics preserve the PC. */
static void fault(const char *message){
    if(state!=FAULTED)snprintf(fault_message,sizeof fault_message,"%s at RV64 PC=%llx after %llu instructions",message,(unsigned long long)guest_pc,(unsigned long long)instructions);
    state=FAULTED;
}
static int range(uint64_t address,uint64_t length){if(address>RAM_SIZE||length>RAM_SIZE-address){fault("guest memory out of bounds");return 0;}return 1;}
/* Little-endian guest loads support unaligned DATA accesses; instruction PCs
   must be four-byte aligned because the compressed extension is not supported. */
static uint64_t load(uint64_t address,unsigned width){
    if(!range(address,width))return 0;
    uint64_t value=0;
    for(unsigned index=0;index<width;index++)value|=(uint64_t)guest_ram[address+index]<<(index*8);
    return value;
}
static void store(uint64_t address,uint64_t value,unsigned width){
    if(!range(address,width))return;
    for(unsigned index=0;index<width;index++)guest_ram[address+index]=(unsigned char)(value>>(index*8));
    reservation_valid=0;
}
/* RV64 sign extension is explicit; never rely on a 32-bit host long. */
static uint64_t sext(uint64_t value,unsigned bits){uint64_t sign=UINT64_C(1)<<(bits-1);return (value^sign)-sign;}
static uint64_t word(uint64_t value){return sext((uint32_t)value,32);}
static uint64_t align_page(uint64_t value){return (value+PAGE-1)&~(uint64_t)(PAGE-1);}
static unsigned available(void){return (key_write+sizeof keyboard-key_read)%sizeof keyboard;}

/* Linux read/readv: no data suspends the ECALL without consuming it. Browser
   keyboard input resumes this exact syscall; it never blocks the JS event loop. */
static uint64_t read_input(uint64_t address,uint64_t length){
    if(!range(address,length))return (uint64_t)-14;
    if(!length)return 0;
    unsigned count=available();if(!count){state=WAITING;return 0;}
    if(length<count)count=(unsigned)length;
    for(unsigned index=0;index<count;index++){guest_ram[address+index]=keyboard[key_read];key_read=(key_read+1)%sizeof keyboard;}
    return count;
}
static uint64_t write_output(uint64_t fd,uint64_t address,uint64_t length){
    if(fd!=1&&fd!=2)return (uint64_t)-9;
    if(!range(address,length))return (uint64_t)-14;
    rv_output((unsigned)fd,guest_ram+address,(unsigned)length);return length;
}

/* RV64 Linux ABI: a7=x17 is syscall number; a0..a5=x10..15 are arguments.
   Return a negative errno in a0 for unsupported services, not a host syscall.
   These shims are for one statically linked process; they are not a Linux VM. */
static uint64_t linux_call(void){
    uint64_t number=registers[17],first=registers[10],second=registers[11],third=registers[12],fourth=registers[13];
    switch(number){
    case 63:if(first)return (uint64_t)-9;return read_input(second,third); /* read */
    case 64:return write_output(first,second,third); /* write */
    case 65:case 66:{ /* readv / writev; iovec = {uint64_t base, length}. */
        if(third>1024||!range(second,third*16))return (uint64_t)-22;
        if(number==65&&first)return (uint64_t)-9;
        uint64_t total=0;
        for(uint64_t index=0;index<third;index++){
            uint64_t pointer=load(second+index*16,8),length=load(second+index*16+8,8);
            if(!length)continue;
            if(number==65&&!available()){if(total)return total;state=WAITING;return 0;}
            uint64_t amount=number==65?read_input(pointer,length):write_output(first,pointer,length);
            if((int64_t)amount<0)return total?total:amount;
            total+=amount;if(amount<length)break;
        }return total;
    }
    case 93:case 94:exit_status=(unsigned)first;state=EXITED;return 0; /* exit[_group] */
    case 214: /* brk, bounded below the mmap arena and stack. */
        if(first>=heap_end&&first<32u*1024u*1024u){memset(guest_ram+heap_end,0,(size_t)(first-heap_end));heap_end=first;}
        return heap_end;
    case 222:{ /* mmap: anonymous private allocations only. */
        if(!(fourth&0x20)||!second)return (uint64_t)-22;
        uint64_t address=(fourth&0x10)?first:map_end,length=align_page(second);
        if(length<second||address<32u*1024u*1024u||address>RAM_SIZE-2*1024*1024||length>RAM_SIZE-2*1024*1024-address)return (uint64_t)-12;
        memset(guest_ram+address,0,(size_t)length);if(!(fourth&0x10))map_end=address+length;return address;
    }
    case 215:case 226:return range(first,second)?0:(uint64_t)-22; /* munmap / mprotect, no page/MMU model */
    case 96:return 1; /* set_tid_address */
    case 99:return 0; /* set_robust_list, single-thread process */
    case 134:if(third&&range(third,32))memset(guest_ram+third,0,32);return 0; /* rt_sigaction */
    case 135:if(third&&fourth<=128&&range(third,fourth))memset(guest_ram+third,0,(size_t)fourth);return 0;
    case 261:if(fourth){store(fourth,8*1024*1024,8);store(fourth+8,8*1024*1024,8);}return 0; /* prlimit64 */
    case 29: /* ioctl: minimal terminal inquiries, no physical device. */
        if(second==0x5401&&range(third,36)){memset(guest_ram+third,0,36);return 0;}
        if(second==0x5413){store(third,24,2);store(third+2,80,2);store(third+4,0,4);return 0;}
        return (uint64_t)-25;
    case 80:if(!range(second,128))return (uint64_t)-14;memset(guest_ram+second,0,128);store(second+16,0020600,4);store(second+56,PAGE,4);return 0;
    case 57:return 0; /* close: no real host descriptors to close. */
    case 62:return (uint64_t)-29; /* lseek: terminal is not seekable. */
    case 113:store(second,instructions/1000000,8);store(second+8,(instructions%1000000)*1000,8);return 0;
    case 172:case 178:return 1; /* getpid / gettid */
    case 174:case 175:case 176:case 177:return 1000; /* uid/gid family */
    case 278: /* Deterministic demo entropy, NOT cryptographic randomness. */
        if(!range(first,second))return (uint64_t)-14;
        for(uint64_t index=0;index<second;index++)guest_ram[first+index]=(unsigned char)(index*73+instructions);
        return second;
    default:return (uint64_t)-38; /* ENOSYS */
    }
}

/* Execute one RV64 instruction. The guest instruction itself is decoded here;
   this is distinct from the guest's AOT Z80 code, which executes as RV64 code. */
static void step(void){
    if(guest_pc&3){fault("unaligned instruction PC (C extension not supported)");return;}
    uint32_t instruction=(uint32_t)load(guest_pc,4);if(state==FAULTED)return;
    unsigned opcode=instruction&127,rd=(instruction>>7)&31,kind=(instruction>>12)&7,rs1=(instruction>>15)&31,rs2=(instruction>>20)&31,top=instruction>>25;
    uint64_t lhs=registers[rs1],rhs=registers[rs2],result=0,next=guest_pc+4,immediate=sext(instruction>>20,12);int writes=1;
    switch(opcode){
    case 0x37:result=word(instruction&0xfffff000u);break; /* LUI */
    case 0x17:result=guest_pc+word(instruction&0xfffff000u);break; /* AUIPC */
    case 0x6f:{ /* JAL rd,J-immediate */
        uint64_t delta=((instruction>>31)<<20)|(((instruction>>12)&255)<<12)|(((instruction>>20)&1)<<11)|(((instruction>>21)&1023)<<1);
        result=next;next=guest_pc+sext(delta,21);break;
    }
    case 0x67:if(kind){fault("invalid JALR");return;}result=next;next=(lhs+immediate)&~UINT64_C(1);break;
    case 0x63:{ /* BEQ/BNE/BLT/BGE/BLTU/BGEU */
        uint64_t delta=((instruction>>31)<<12)|(((instruction>>7)&1)<<11)|(((instruction>>25)&63)<<5)|(((instruction>>8)&15)<<1);
        int taken=0;writes=0;
        switch(kind){case 0:taken=lhs==rhs;break;case 1:taken=lhs!=rhs;break;case 4:taken=(int64_t)lhs<(int64_t)rhs;break;case 5:taken=(int64_t)lhs>=(int64_t)rhs;break;case 6:taken=lhs<rhs;break;case 7:taken=lhs>=rhs;break;default:fault("invalid branch");return;}
        if(taken)next=guest_pc+sext(delta,13);
        break;
    }
    case 3:{ /* LB/LH/LW/LD/LBU/LHU/LWU */
        uint64_t address=lhs+immediate;
        if(kind<=3){unsigned bits=8u<<kind;result=load(address,bits/8);if(bits<64)result=sext(result,bits);}
        else if(kind<=6)result=load(address,1u<<(kind-4));else{fault("invalid load");return;}
        break;
    }
    case 0x23:{ /* SB/SH/SW/SD */
        uint64_t delta=sext(((instruction>>25)<<5)|((instruction>>7)&31),12);writes=0;
        if(kind>3){fault("invalid store");return;}store(lhs+delta,rhs,1u<<kind);break;
    }
    case 0x13: /* Integer immediate operations; shifts use six-bit counts. */
        switch(kind){
        case 0:result=lhs+immediate;break;case 2:result=(int64_t)lhs<(int64_t)immediate;break;case 3:result=lhs<immediate;break;
        case 4:result=lhs^immediate;break;case 6:result=lhs|immediate;break;case 7:result=lhs&immediate;break;
        case 1:if(instruction>>26){fault("invalid SLLI");return;}result=lhs<<((instruction>>20)&63);break;
        case 5:if(instruction>>26==0)result=lhs>>((instruction>>20)&63);else if(instruction>>26==16)result=(uint64_t)((int64_t)lhs>>((instruction>>20)&63));else{fault("invalid right shift immediate");return;}break;
        }break;
    case 0x1b: /* ADDIW/SLLIW/SRLIW/SRAIW always sign-extend bit 31. */
        if(kind==0)result=word(lhs+immediate);
        else if(kind==1&&top==0)result=word((uint32_t)lhs<<rs2);
        else if(kind==5&&top==0)result=word((uint32_t)lhs>>rs2);
        else if(kind==5&&top==32)result=word((uint32_t)((int32_t)lhs>>rs2));
        else{fault("invalid word immediate");return;}break;
    case 0x33:case 0x3b:{
        int narrow=opcode==0x3b;unsigned shift=(unsigned)rhs&(narrow?31:63);
        if(top==1){ /* M extension: divide-by-zero and MIN/-1 are defined. */
            if(narrow){uint32_t left=(uint32_t)lhs,right=(uint32_t)rhs;
                switch(kind){
                case 0:result=(uint64_t)left*right;break;
                case 4:result=!right?UINT32_MAX:(left==0x80000000u&&right==UINT32_MAX?left:(uint32_t)((int32_t)left/(int32_t)right));break;
                case 5:result=!right?UINT32_MAX:left/right;break;
                case 6:result=!right?left:(left==0x80000000u&&right==UINT32_MAX?0:(uint32_t)((int32_t)left%(int32_t)right));break;
                case 7:result=!right?left:left%right;break;
                default:fault("invalid M word operation");return;
                }
            }else switch(kind){
                case 0:result=lhs*rhs;break;
                case 1:result=(uint64_t)(((__int128)(int64_t)lhs*(__int128)(int64_t)rhs)>>64);break;
                case 2:result=(uint64_t)(((unsigned __int128)lhs*rhs)>>64)-((int64_t)lhs<0?rhs:0);break;
                case 3:result=(uint64_t)(((unsigned __int128)lhs*rhs)>>64);break;
                case 4:result=!rhs?UINT64_MAX:(lhs==UINT64_C(0x8000000000000000)&&rhs==UINT64_MAX?lhs:(uint64_t)((int64_t)lhs/(int64_t)rhs));break;
                case 5:result=!rhs?UINT64_MAX:lhs/rhs;break;
                case 6:result=!rhs?lhs:(lhs==UINT64_C(0x8000000000000000)&&rhs==UINT64_MAX?0:(uint64_t)((int64_t)lhs%(int64_t)rhs));break;
                case 7:result=!rhs?lhs:lhs%rhs;break;
            }
        }else{
            if(top!=0&&!(top==32&&(kind==0||kind==5))){fault("invalid integer function");return;}
            switch(kind){
            case 0:result=top==32?lhs-rhs:lhs+rhs;break;
            case 1:result=narrow?(uint32_t)lhs<<shift:lhs<<shift;break;
            case 2:if(narrow){fault("invalid word comparison");return;}result=(int64_t)lhs<(int64_t)rhs;break;
            case 3:if(narrow){fault("invalid word comparison");return;}result=lhs<rhs;break;
            case 4:if(narrow){fault("invalid word XOR");return;}result=lhs^rhs;break;
            case 5:result=narrow?(top==32?(uint32_t)((int32_t)lhs>>shift):(uint32_t)lhs>>shift):(top==32?(uint64_t)((int64_t)lhs>>shift):lhs>>shift);break;
            case 6:if(narrow){fault("invalid word OR");return;}result=lhs|rhs;break;
            case 7:if(narrow){fault("invalid word AND");return;}result=lhs&rhs;break;
            }
        }
        if(narrow)result=word(result);
        break;
    }
    case 0x2f:{ /* A extension, single hart. aq/rl have no reordering to constrain. */
        if((kind!=2&&kind!=3)||(lhs&((kind==2?4:8)-1))){fault("invalid/misaligned atomic");return;}
        unsigned width=kind==2?4:8,operation=instruction>>27;uint64_t old=load(lhs,width),value=rhs;
        if(operation==2){if(rs2){fault("invalid LR");return;}reservation=lhs;reservation_valid=1;}
        else if(operation==3){int success=reservation_valid&&reservation==lhs;if(success)store(lhs,rhs,width);reservation_valid=0;result=success?0:1;break;}
        else{
            if(width==4){old=(uint32_t)old;value=(uint32_t)value;}
            switch(operation){
            case 0:value=old+value;break;case 1:break;case 4:value=old^value;break;case 8:value=old|value;break;case 12:value=old&value;break;
            case 16:value=width==4?((int32_t)old<(int32_t)value?old:value):((int64_t)old<(int64_t)value?old:value);break;
            case 20:value=width==4?((int32_t)old>(int32_t)value?old:value):((int64_t)old>(int64_t)value?old:value);break;
            case 24:value=old<value?old:value;break;case 28:value=old>value?old:value;break;
            default:fault("unsupported atomic operation");return;
            }store(lhs,value,width);
        }result=width==4?word(old):old;break;
    }
    case 0x0f:if(kind>1){fault("unsupported fence");return;}writes=0;break;
    case 0x73:
        if(instruction==0x00000073){uint64_t value=linux_call();if(state==WAITING)return;registers[10]=value;writes=0;}
        else if(kind==2&&rs1==0){unsigned csr=instruction>>20;if(csr==0xc00||csr==0xc01||csr==0xc02)result=instructions;else{fault("unsupported CSR");return;}}
        else{fault("unsupported SYSTEM/privileged instruction");return;}break;
    default:fault("unsupported RV64 opcode (no C/F/D extensions)");return;
    }
    if(state==FAULTED)return;
    if(writes&&rd)registers[rd]=result;
    registers[0]=0;guest_pc=next;instructions++;
}

/* ELF64 little-endian reads are separate from guest loads: file offsets are
   bounded by file length, not RAM_SIZE. Loader accepts static ET_EXEC only. */
static uint64_t elf_value(unsigned offset,unsigned width,unsigned length){
    if(offset>length||width>length-offset){fault("truncated ELF");return 0;}
    uint64_t value=0;for(unsigned index=0;index<width;index++)value|=(uint64_t)elf_image[offset+index]<<(index*8);return value;
}
static int boot_elf(unsigned length,int argc,const char *const *argv){
    state=RUNNING;fault_message[0]=0;memset(registers,0,sizeof registers);memset(guest_ram,0,sizeof guest_ram);
    guest_pc=instructions=heap_end=0;map_end=32u*1024u*1024u;reservation_valid=key_read=key_write=exit_status=0;
    if(length<64||length>ELF_LIMIT||memcmp(elf_image,"\177ELF",4)||elf_image[4]!=2||elf_image[5]!=1||elf_value(16,2,length)!=2||elf_value(18,2,length)!=243){fault("expected static little-endian RV64 ELF64 ET_EXEC");return 0;}
    if(elf_value(48,4,length)&7){fault("ELF requires unsupported compressed or hard-float ABI");return 0;}
    uint64_t phoff=elf_value(32,8,length),phsize=elf_value(54,2,length),phcount=elf_value(56,2,length),phdr=0;
    guest_pc=elf_value(24,8,length);
    if(phsize<56||phcount>128||phoff>length||phcount*phsize>length-phoff){fault("invalid ELF program headers");return 0;}
    int entry_valid=0;
    for(unsigned index=0;index<phcount;index++){
        unsigned header=(unsigned)(phoff+index*phsize);uint64_t type=elf_value(header,4,length),flags=elf_value(header+4,4,length),offset=elf_value(header+8,8,length),address=elf_value(header+16,8,length),file_size=elf_value(header+32,8,length),memory_size=elf_value(header+40,8,length);
        if(type==3){fault("dynamic interpreter not supported");return 0;}if(type!=1)continue;
        if(file_size>memory_size||offset>length||file_size>length-offset||address<4096||address>32u*1024u*1024u||memory_size>32u*1024u*1024u-address){fault("invalid ELF load segment");return 0;}
        memcpy(guest_ram+address,elf_image+offset,(size_t)file_size);
        if(address+memory_size>heap_end)heap_end=address+memory_size;
        if(offset<=phoff&&phoff-offset<memory_size)phdr=address+phoff-offset;
        if((flags&1)&&guest_pc>=address&&guest_pc<address+memory_size)entry_valid=1;
    }
    if(!entry_valid||(guest_pc&3)||!phdr){fault("ELF entry/PHDR is not mapped");return 0;}
    heap_end=align_page(heap_end);
    /* Linux entry stack: argc, argv[], NULL, envp NULL, auxv key/value pairs.
       AT_RANDOM is fixed demo data; it is not cryptographic entropy. */
    uint64_t stack=STACK_TOP,pointers[32];if(argc<1||argc>32){fault("invalid guest argc");return 0;}
    for(int index=argc-1;index>=0;index--){size_t amount=strlen(argv[index])+1;if(amount>65536){fault("guest argument too large");return 0;}stack-=amount;memcpy(guest_ram+stack,argv[index],amount);pointers[index]=stack;}
    stack-=16;uint64_t random=stack;for(unsigned index=0;index<16;index++)guest_ram[stack+index]=(unsigned char)(index*17+3);
    uint64_t aux[]={3,phdr,4,phsize,5,phcount,6,PAGE,7,0,9,guest_pc,11,1000,12,1000,13,1000,14,1000,16,(1u<<8)|(1u<<12)|1u,23,0,25,random,0,0};
    unsigned words=(unsigned)argc+3+(unsigned)(sizeof aux/sizeof aux[0]);stack=(stack-words*8)&~UINT64_C(15);registers[2]=stack;
    store(stack,argc,8);stack+=8;for(int index=0;index<argc;index++,stack+=8)store(stack,pointers[index],8);
    store(stack,0,8);stack+=8;store(stack,0,8);stack+=8;for(unsigned index=0;index<sizeof aux/sizeof aux[0];index++,stack+=8)store(stack,aux[index],8);
    return state!=FAULTED;
}

/* Public WASM ABI: upload ELF, boot, enqueue stdin, run bounded CPU batches.
   Return state 0=running, 1=stdin wait, 2=process exit, 3=emulator fault. */
unsigned rv_image_pointer(void){return (unsigned)(uintptr_t)elf_image;}
unsigned rv_image_limit(void){return ELF_LIMIT;}
unsigned rv_boot(unsigned length){const char *args[]={"trs80-rv64im","--interactive"};return boot_elf(length,2,args);}
unsigned rv_enqueue(unsigned value){unsigned next=(key_write+1)%sizeof keyboard;if(next==key_read)return 0;keyboard[key_write]=(unsigned char)value;key_write=next;if(state==WAITING)state=RUNNING;return 1;}
unsigned rv_run(unsigned budget){for(unsigned tick=0;tick<budget&&state==RUNNING;tick++)step();return state;}
unsigned rv_steps(void){return (unsigned)instructions;}
unsigned rv_exit_status(void){return exit_status;}
unsigned rv_fault_pointer(void){return (unsigned)(uintptr_t)fault_message;}
#ifndef RV_NO_MAIN
/* Native driver uses the same CPU/loader as WASM. Arguments after the ELF path
   become guest argv, enabling direct comparison with QEMU on the same ELF. */
int main(int argc,char **argv){
    if(argc<2){fprintf(stderr,"usage: rv-run PROGRAM.elf [GUEST ARGS...]\n");return 1;}
    FILE *file=fopen(argv[1],"rb");if(!file){perror(argv[1]);return 1;}
    size_t length=fread(elf_image,1,sizeof elf_image,file);if(fgetc(file)!=EOF){fprintf(stderr,"ELF exceeds limit\n");return 1;}fclose(file);
    if(!boot_elf((unsigned)length,argc-1,(const char *const *)(argv+1))){fprintf(stderr,"%s\n",fault_message);return 1;}
    uint64_t limit=UINT64_C(5000000000);
    while(state==RUNNING||state==WAITING){
        if(state==WAITING){int ch=getchar();if(ch==EOF){fprintf(stderr,"stdin EOF while guest waits\n");return 1;}rv_enqueue((unsigned)ch);}
        rv_run(100000);
        if(instructions>=limit){fprintf(stderr,"guest instruction budget exceeded\n");return 1;}
    }
    if(state==FAULTED){fprintf(stderr,"%s\n",fault_message);return 1;}
    fprintf(stderr,"RV64 instructions=%llu\n",(unsigned long long)instructions);return (int)exit_status;
}
#endif
/* Generic debugger/memory access. The browser BASIC adapter locates the guest's
   Z80 memory symbol in its ELF, keeping BASIC knowledge out of this CPU core. */
unsigned rv_memory_pointer(void){return (unsigned)(uintptr_t)guest_ram;}
unsigned rv_memory_size(void){return RAM_SIZE;}
uint64_t rv_program_counter(void){return guest_pc;}
uint64_t rv_register(unsigned index){return index<32?registers[index]:0;}
