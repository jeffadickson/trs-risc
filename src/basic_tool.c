/*
 * TRS-RISC BASIC file workbench. The tokenizer and detokenizer are the ORIGINAL
 * Level II ROM, translated ahead of time into this executable. This driver
 * supplies virtual keystrokes and saves/restores the linked-list program area.
 *
 * Z80 ABI used below:
 *   CALL 0033h  ; A = output character
 *   CALL 002Bh  ; A = key or 0; CALL 0049h is the blocking wrapper
 *   40A4h: DEFW 42E9h  ; start of stored BASIC text
 *   40F9h: DEFW nn     ; start of variables (one past program's 0000 sentinel)
 *   40FBh / 40FDh      ; start of arrays / end of variables
 *
 * A .basram file is exactly the memory bytes at 42E9h, not a cassette, .CMD,
 * executable, or invented token format. Its pointers are absolute Z80 addresses.
 */
#define TESTING
#include "runtime.c"
#include <ctype.h>
#include <errno.h>

enum { BASIC_BASE=0x42e9, TEXT_PTR=0x40a4, VAR_PTR=0x40f9,
       ARRAY_PTR=0x40fb, END_PTR=0x40fd, IMAGE_LIMIT=0xf000 };
static char output[262144];
static size_t output_length;
static int live_output;

/* Host error reporting has no Z80 equivalent: do not confuse a file failure
   with the ROM's own ?SN, ?OM, or arithmetic errors. */
static void tool_error(const char *message){fprintf(stderr,"basic-tool: %s\n",message);exit(1);}

/* Z80 code performs file-less memory reads; host files supply the same bytes.
   Cap inputs so neither a corrupt file nor a large listing exhausts host RAM. */
static unsigned char *read_file(const char *path,size_t *length){
    FILE *fp=fopen(path,"rb");if(!fp)tool_error(strerror(errno));
    if(fseek(fp,0,SEEK_END)||ftell(fp)<0)tool_error("cannot measure input");
    long amount=ftell(fp);if(amount>65536)tool_error("input exceeds 64 KB");
    rewind(fp);unsigned char *buffer=calloc((size_t)amount+1,1);
    if(!buffer)tool_error("out of memory");
    if(fread(buffer,1,(size_t)amount,fp)!=(size_t)amount)tool_error("cannot read input");
    fclose(fp);*length=(size_t)amount;return buffer;
}

/* Host counterpart of saving [HL,DE) memory; unlike CSAVE, no cassette header
   or baud-rate encoding is added. Always write binary mode, including on Windows. */
static void write_file(const char *path,const void *buffer,size_t length){
    FILE *fp=fopen(path,"wb");if(!fp)tool_error(strerror(errno));
    if(fwrite(buffer,1,length,fp)!=length)tool_error("cannot write output");
    if(fclose(fp))tool_error("cannot finish output");
}

/* Z80: CALL 0033h, then RET. Printable characters and CR are the same terminal
   adapter as runtime.c. Output is captured for LIST, not fabricated by the tool. */
static void console_character(unsigned char value){
    int ch=value==13?'\n':value;
    if(ch!='\n'&&(ch<32||ch>=127))return;
    if(output_length+1>=sizeof output)tool_error("ROM output exceeds capture limit");
    output[output_length++]=(char)ch;output[output_length]=0;
    if(live_output)putchar(ch);
}

/* Resume the native-translated ROM until a blocking keyboard read has no input.
   Z80: 0049 CD 2B 00 / B7 / C0 / 18 F9 = CALL 002Bh; OR A; RET NZ; JR 0049h.
   A nonblocking poll's return address differs from 004Ch, so it gets A=0.
   EOF means "suspended at keyboard", not HALT, preserving PC and the Z80 stack. */
static void execute(const char *keys){
    input=keys;input_pos=0;output_length=0;output[0]=0;
    for(steps=0;steps<20000000&&!halted;steps++){
        if(pc==0x33){console_character(a);pc=pop();continue;}
        if(pc==0x2b){
            if(rd16(sp)!=0x004c){a=0;pc=pop();continue;}
            if(!input[input_pos])return;
            a=(uint8_t)input[input_pos++];if(a=='\n')a=13;pc=pop();continue;
        }
        translated_step();
    }
    tool_error("instruction budget exceeded (possible infinite loop)");
}

/* Power-on path: PC=0000h -> DI; XOR A; JP 0674h. Two carriage returns accept
   the default memory size and leave BASIC at its command prompt. No BASIC
   parser is implemented here: the ROM initializes and tokenizes its own RAM. */
static void boot(void){
    memcpy(mem,rom_image,sizeof rom_image);memset(mem+0x3c00,' ',1024);
    execute("\r\r");
    if(rd16(TEXT_PTR)!=BASIC_BASE)tool_error("ROM has unexpected BASIC text base");
}

/* Validate the image BEFORE loading it. Z80 layout for each nonfinal node:
       DEFW next_address, line_number
       DEFB tokenized_body..., 0
   The final node contains DEFW 0 only. Requiring contiguous links prevents
   malformed files from making LIST chase arbitrary memory. */
static void validate_image(const unsigned char *bytes,size_t length){
    if(length<2||length>(size_t)(IMAGE_LIMIT-BASIC_BASE))tool_error("invalid program image length");
    size_t offset=0;int previous=-1;
    for(;;){
        if(offset+2>length)tool_error("truncated line link");
        unsigned next=bytes[offset]|(bytes[offset+1]<<8);
        if(!next){if(offset+2!=length)tool_error("bytes after final program sentinel");return;}
        if(offset+5>length||next<BASIC_BASE+offset+5||next>=BASIC_BASE+length)tool_error("invalid next-line address");
        unsigned line=bytes[offset+2]|(bytes[offset+3]<<8);
        if((int)line<=previous||line>65529)tool_error("invalid or unordered BASIC line number");
        previous=(int)line;size_t end=next-BASIC_BASE;
        if(bytes[end-1]!=0)tool_error("line has no NUL terminator");
        for(size_t pos=offset+4;pos+1<end;pos++)if(!bytes[pos])tool_error("embedded NUL in line body");
        offset=end;
    }
}

/* Restore the exact text bytes, then reset variable/array boundaries as NEW's
   empty variable state expects. Z80 equivalents are LDIR and LD (40F9h),HL /
   LD (40FBh),HL / LD (40FDh),HL. RUN subsequently clears execution state. */
static void install_image(const unsigned char *bytes,size_t length){
    validate_image(bytes,length);memcpy(mem+BASIC_BASE,bytes,length);
    uint16_t end=(uint16_t)(BASIC_BASE+length);
    wr16(VAR_PTR,end);wr16(ARRAY_PTR,end);wr16(END_PTR,end);
}

/* Convert source newlines to virtual Enter keys. Preflight line numbers and
   lengths before sending anything: unnumbered text could otherwise execute an
   immediate RUN/NEW rather than become stored program text. Duplicate source
   line numbers are rejected, rather than silently replacing an earlier line. */
static char *source_keys(const char *path){
    size_t length;unsigned char *bytes=read_file(path,&length);
    char *keys=calloc(length+2,1);if(!keys)tool_error("out of memory");
    unsigned char seen[65530]={0};size_t source=0,dest=0;unsigned physical=0;
    while(source<length){
        size_t begin=source;while(source<length&&bytes[source]!='\r'&&bytes[source]!='\n')source++;
        size_t end=source;physical++;
        if(source<length&&bytes[source]=='\r')source++;
        if(source<length&&bytes[source]=='\n')source++;
        while(begin<end&&(bytes[begin]==' '||bytes[begin]=='\t'))begin++;
        if(begin==end)continue;
        if(end-begin>250)tool_error("source line exceeds safe 250-character input limit");
        unsigned number=0;size_t pos=begin;
        if(!isdigit(bytes[pos])){fprintf(stderr,"source line %u: missing BASIC line number\n",physical);exit(1);}
        while(pos<end&&isdigit(bytes[pos])){number=number*10+bytes[pos++]-'0';if(number>65529)tool_error("line number exceeds 65529");}
        if(seen[number]++)tool_error("duplicate BASIC line number");
        while(pos<end&&bytes[pos]==' ')pos++;
        if(pos==end)tool_error("empty numbered line would delete a program line");
        for(size_t index=begin;index<end;index++){
            if(bytes[index]<32||bytes[index]>126)tool_error("source must contain printable ASCII and newlines");
            keys[dest++]=(char)bytes[index];
        }
        keys[dest++]='\r';
    }
    free(bytes);return keys;
}

/* Static syntax checking operates on ROM-produced tokens. It never runs user
   statements, follows GOTO branches, or evaluates PEEK/USR expressions. The
   parser and its explicit unsupported-statement diagnostics live separately. */
#include "basic_syntax.inc"

/* Z80 LIST prints numbered lines via CALL 0033h. Discard only prompt/echo
   lines; preserve the ROM's spacing, keyword spellings, DATA and REM contents.
   The leading digit test cannot mistake READY or the echoed LIST for a line. */
static char *listing(void){
    execute("LIST\r");char *text=calloc(output_length+1,1);if(!text)tool_error("out of memory");
    size_t dest=0;char *line=output;
    while(*line){char *end=strchr(line,'\n');if(!end)end=line+strlen(line);
        char *start=line;while(start<end&&*start==' ')start++;
        if(start<end&&isdigit((unsigned char)*start)){size_t amount=(size_t)(end-start);memcpy(text+dest,start,amount);dest+=amount;text[dest++]='\n';}
        line=*end?end+1:end;
    }
    return text;
}

/* Host CLI entry point, not a ROM routine. Every conversion boots a fresh ROM,
   so token tables, text pointers and LIST formatting come from the pinned ROM. */
int main(int argc,char **argv){
    if(argc<3){fprintf(stderr,"usage: basic-tool tokenize source.bas image.basram\n"
        "       basic-tool check source.bas\n       basic-tool detokenize image.basram output.bas\n"
        "       basic-tool list image.basram\n       basic-tool run image.basram\n");return 1;}
    boot();
    if(!strcmp(argv[1],"tokenize")||!strcmp(argv[1],"check")){
        if(!strcmp(argv[1],"tokenize")&&argc!=4)tool_error("tokenize requires an output path");
        char *keys=source_keys(argv[2]);execute(keys);free(keys);
        if(strstr(output,"Error"))tool_error("ROM rejected source during line entry");
        size_t length=rd16(VAR_PTR)-BASIC_BASE;validate_image(mem+BASIC_BASE,length);
        int errors=check_program(mem+BASIC_BASE,length);
        if(errors){fprintf(stderr,"%d syntax/coverage error(s); no image written\n",errors);return 1;}
        if(!strcmp(argv[1],"tokenize")){write_file(argv[3],mem+BASIC_BASE,length);fprintf(stderr,"wrote %zu bytes at Z80 address %04X\n",length,BASIC_BASE);}
        else puts("syntax check passed (see documented grammar coverage)");
    }else{
        size_t length;unsigned char *bytes=read_file(argv[2],&length);install_image(bytes,length);free(bytes);
        if(!strcmp(argv[1],"run")){live_output=1;execute("RUN\r");if(strstr(output,"Error"))return 1;}
        else if(!strcmp(argv[1],"list")){char *text=listing();fputs(text,stdout);free(text);}
        else if(!strcmp(argv[1],"detokenize")){if(argc!=4)tool_error("detokenize requires an output path");char *text=listing();write_file(argv[3],text,strlen(text));free(text);}
        else tool_error("unknown command");
    }
    return 0;
}
