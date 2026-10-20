/*
 * Browser execution adapter for the SAME native-translated ROM.
 * No new BASIC interpreter or JavaScript Z80 emulator is introduced here.
 * WASI libc is linked for existing diagnostics; the browser supplies its small
 * fd_write/proc_exit import surface. Normal terminal traffic uses console_write.
 */
#define TESTING
#include "runtime.c"

/* Drop register macros before public WebAssembly APIs, so normal identifier
   names cannot accidentally expand into the Z80 register-pair union. */
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

__attribute__((import_module("env"),import_name("console_write")))
extern void browser_character(unsigned value);
static unsigned char keys[4096];
static unsigned read_key,write_key,web_status;

/* Host reset corresponds to Z80 reset PC=0000h. Preserve immutable rom_image,
   clear all writable state, and let original ROM code perform BASIC startup. */
__attribute__((visibility("default")))
void web_reset(void){
    memset(mem,0,sizeof mem);memcpy(mem,rom_image,sizeof rom_image);
    memset(mem+0x3c00,' ',1024);
    reg_af.w=reg_bc.w=reg_de.w=reg_hl.w=0;
    af2=bc2=de2=hl2=ix=iy=sp=pc=0;
    ir=refresh=iff1=iff2=im=halted=0;steps=hooks=0;
    read_key=write_key=web_status=0;
}

/* Queue one ASCII key; the ring buffer leaves one slot free to distinguish
   full/empty. A=key is returned only to the ROM's blocking keyboard wrapper. */
__attribute__((visibility("default")))
unsigned web_enqueue(unsigned value){
    unsigned next=(write_key+1)%sizeof keys;
    if(next==read_key)return 0;
    keys[write_key]=(unsigned char)(value==10?13:value);write_key=next;
    web_status=0;return 1;
}

/* Execute a bounded native-code slice, then yield to the browser event loop.
   Z80 CALL 0033h writes A; CALL 002Bh returns queued input only for the CALL
   at 0049h (return address 004Ch). Polls elsewhere get A=0. When input is absent
   leave PC and SP untouched, so the next browser key resumes the same call.
   Return 0=more runnable work, 1=waiting for a key, 2=HALT. */
__attribute__((visibility("default")))
unsigned web_run(unsigned budget){
    for(unsigned tick=0;tick<budget&&!halted;tick++,steps++){
        if(pc==0x33){browser_character(reg_af.hi);pc=pop();continue;}
        if(pc==0x2b){
            if(rd16(sp)!=0x004c){reg_af.hi=0;pc=pop();continue;}
            if(read_key==write_key)return web_status=1;
            reg_af.hi=keys[read_key];read_key=(read_key+1)%sizeof keys;pc=pop();continue;
        }
        translated_step();
    }
    return web_status=halted?2:0;
}

/* Browser download/upload views the original BASIC program bytes directly.
   Z80 LD HL,(40A4h) gives the start; LD DE,(40F9h) gives the variable boundary.
   The returned pointer is a WASM linear-memory address, NOT a Z80 address. */
__attribute__((visibility("default")))
unsigned web_program_pointer(void){return (unsigned)(uintptr_t)(mem+0x42e9);}
__attribute__((visibility("default")))
unsigned web_program_length(void){return rd16(0x40f9)>=0x42e9?(unsigned)(rd16(0x40f9)-0x42e9):0;}
__attribute__((visibility("default")))
unsigned web_step_count(void){return (unsigned)steps;}

/* Validate a raw 42E9h-based linked list before installing it. The caller first
   copies bytes into a separate staging buffer; on error, program RAM is intact.
   The JS host invokes this only while BASIC is waiting at its command prompt. */
static unsigned char staging[0xad17];
__attribute__((visibility("default")))
unsigned web_upload_pointer(void){return (unsigned)(uintptr_t)staging;}
__attribute__((visibility("default")))
unsigned web_upload(unsigned length){
    if(length<2||length>sizeof staging||web_status!=1)return 0;
    unsigned offset=0;int previous=-1;
    for(;;){
        if(offset+2>length)return 0;
        unsigned next=staging[offset]|(staging[offset+1]<<8);
        if(!next){if(offset+2!=length)return 0;break;}
        if(offset+5>length||next<0x42e9+offset+5||next>=0x42e9+length)return 0;
        unsigned line=staging[offset+2]|(staging[offset+3]<<8);
        if((int)line<=previous||line>65529)return 0;previous=(int)line;
        unsigned end=next-0x42e9;if(staging[end-1])return 0;
        for(unsigned pos=offset+4;pos+1<end;pos++)if(!staging[pos])return 0;
        offset=end;
    }
    memcpy(mem+0x42e9,staging,length);
    uint16_t end=(uint16_t)(0x42e9+length);
    wr16(0x40f9,end);wr16(0x40fb,end);wr16(0x40fd,end);return 1;
}
