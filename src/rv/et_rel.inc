/* Single-object, freestanding ET_REL linking. This is separate from PT_LOAD
   executable loading and static PIE's symbol-free dynamic relocator.
   Metadata and patches live outside guest RAM: validate the WHOLE object before
   copying sections or applying any relocation. Fixed caps bound host/WASM work. */
#define REL_SECTIONS 128u
#define REL_SYMBOLS 512u
#define REL_PATCHES 4096u
#define IMAGE_TOP (32u*1024u*1024u)
typedef struct {
    uint64_t offset,size,flags,alignment,address,entry;
    unsigned type,link,info,name;
} RelSection;
typedef struct {uint64_t value;unsigned name,binding;} RelSymbol;
typedef struct {uint64_t address,value;} RelPatch;
static RelSection rel_sections[REL_SECTIONS];
static RelSymbol rel_symbols[REL_SYMBOLS];
static RelPatch rel_patches[REL_PATCHES];

static int rel_fail(const char *message){fault(message);return 0;}
static int rel_overlap(uint64_t a,uint64_t n,uint64_t b,uint64_t m){return n&&m&&a<b+m&&b<a+n;}
/* String offsets are section-relative. Limit individual names as well as table
   counts; hostile unterminated strings must not cause unbounded repeated scans. */
static const char *rel_string(const RelSection *strings,unsigned offset){
    if(offset>=strings->size){fault("ET_REL string offset out of bounds");return NULL;}
    const char *name=(const char *)elf_image+strings->offset+offset;
    uint64_t available=strings->size-offset;
    for(unsigned index=0;index<256&&index<available;index++)if(!name[index])return name;
    fault("ET_REL unterminated/overlong name");return NULL;
}

static int boot_rel(unsigned length,int argc,const char *const *argv,uint64_t requested_base){
    uint64_t table=elf_value(40,8,length),stride=elf_value(58,2,length),count=elf_value(60,2,length);
    unsigned names=(unsigned)elf_value(62,2,length),symtab=0,patch_count=0,relocation_count=0;
    uint64_t base=requested_base?requested_base:4u*1024u*1024u,cursor=base,entry=0;
    if((base&(PAGE-1))||base<PAGE||base>=IMAGE_TOP)return rel_fail("invalid ET_REL load base");
    if(elf_image[6]!=1||elf_value(20,4,length)!=1||elf_value(52,2,length)!=64)return rel_fail("invalid ET_REL header version/size");
    if(elf_value(24,8,length)||elf_value(32,8,length)||elf_value(56,2,length))return rel_fail("ET_REL must not contain an entry/program headers");
    /* No extended section numbering, processor flags or alternative ABI in this
       milestone. Instruction bytes themselves remain the CPU decoder's job. */
    if(elf_value(48,4,length))return rel_fail("unsupported ET_REL ISA/ABI flags");
    if(!count||count>REL_SECTIONS||stride!=64||table<64||(table&7)||table>length||count*stride>length-table||!names||names>=count)
        return rel_fail("invalid/extended ET_REL section table");
    memset(rel_sections,0,sizeof rel_sections);memset(rel_symbols,0,sizeof rel_symbols);
    for(unsigned index=0;index<count;index++){
        unsigned at=(unsigned)(table+index*stride);RelSection *part=&rel_sections[index];
        part->name=(unsigned)elf_value(at,4,length);part->type=(unsigned)elf_value(at+4,4,length);
        part->flags=elf_value(at+8,8,length);part->offset=elf_value(at+24,8,length);part->size=elf_value(at+32,8,length);
        part->link=(unsigned)elf_value(at+40,4,length);part->info=(unsigned)elf_value(at+44,4,length);
        part->alignment=elf_value(at+48,8,length);part->entry=elf_value(at+56,8,length);
        if(elf_value(at+16,8,length))return rel_fail("ET_REL section has a virtual address");
        if(!index){
            for(unsigned byte=0;byte<64;byte++)if(elf_image[at+byte])return rel_fail("invalid ET_REL null section");
            continue;
        }
        if(part->alignment>IMAGE_TOP||(part->alignment&&(part->alignment&(part->alignment-1))))return rel_fail("invalid ET_REL section alignment");
        if(part->type!=8){
            if(part->offset>length||part->size>length-part->offset)return rel_fail("ET_REL section outside file");
            if(rel_overlap(part->offset,part->size,0,64)||rel_overlap(part->offset,part->size,table,count*stride))return rel_fail("ET_REL section overlaps ELF metadata");
            for(unsigned previous=1;previous<index;previous++){
                RelSection *other=&rel_sections[previous];
                if(other->type!=8&&rel_overlap(part->offset,part->size,other->offset,other->size))return rel_fail("overlapping ET_REL file sections");
            }
        }
        if(part->flags&2){ /* SHF_ALLOC: only ordinary code/data/BSS. */
            if((part->type!=1&&part->type!=8)||(part->flags&~UINT64_C(7))||((part->flags&5)==5)||((part->flags&4)&&part->type==8))
                return rel_fail("unsupported ET_REL allocatable section");
            /* Separate each nonempty section by a page. This gives code,
               readonly literals and writable data distinct planned extents. */
            uint64_t alignment=part->alignment>PAGE?part->alignment:PAGE;
            uint64_t address=(cursor+alignment-1)&~(alignment-1);
            if(address>=IMAGE_TOP||part->size>IMAGE_TOP-address)return rel_fail("ET_REL sections exceed image arena");
            /* Empty sections still have a meaningful anchor for zero-size
               symbols, but do not consume space or publish a memory extent. */
            part->address=address;if(part->size)cursor=address+part->size;
        }else{
            /* Debug PROGBITS and compiler attributes can be ignored, but never
               silently ignore relocation sections, groups, TLS or dynamic data. */
            /* Assemblers mark SHT_RELA with SHF_INFO_LINK (0x40). */
            if((part->flags&&!(part->type==4&&part->flags==0x40))||!(part->type==1||part->type==2||part->type==3||part->type==4||part->type==0x70000003))
                return rel_fail("unsupported ET_REL metadata section");
            if(part->type==2){if(symtab)return rel_fail("multiple ET_REL symbol tables");symtab=index;}
        }
        if(part->type!=2&&part->type!=4&&(part->link||part->info||part->entry))return rel_fail("unsupported ET_REL section links/entry size");
    }
    RelSection *strings=&rel_sections[names];
    if(strings->type!=3||(strings->flags&2)||!strings->size||elf_image[strings->offset])return rel_fail("invalid ET_REL section names");
    for(unsigned index=1;index<count;index++)if(!rel_string(strings,rel_sections[index].name))return 0;
    if(!symtab)return rel_fail("ET_REL symbol table missing");
    RelSection *symbols=&rel_sections[symtab];
    if(symbols->entry!=24||symbols->size%24||!symbols->size||symbols->size/24>REL_SYMBOLS||!symbols->link||symbols->link>=count)
        return rel_fail("invalid ET_REL symbol table");
    unsigned symbol_count=(unsigned)(symbols->size/24);
    strings=&rel_sections[symbols->link];
    if(strings->type!=3||(strings->flags&2)||!strings->size||elf_image[strings->offset]||!symbols->info||symbols->info>symbol_count)
        return rel_fail("invalid ET_REL symbol strings/local boundary");
    for(unsigned index=0;index<symbol_count;index++){
        unsigned at=(unsigned)(symbols->offset+index*24),name=(unsigned)elf_value(at,4,length);
        unsigned info=(unsigned)elf_value(at+4,1,length),visibility=(unsigned)elf_value(at+5,1,length),section=(unsigned)elf_value(at+6,2,length);
        uint64_t value=elf_value(at+8,8,length),size=elf_value(at+16,8,length);
        if(!index){for(unsigned byte=0;byte<24;byte++)if(elf_image[at+byte])return rel_fail("invalid ET_REL null symbol");continue;}
        const char *text=rel_string(strings,name);if(!text)return 0;
        if((info>>4)>1||(info&15)>4||visibility||((index<symbols->info)!=(info>>4==0)))return rel_fail("unsupported ET_REL symbol binding/type/visibility");
        if(!section)return rel_fail("unresolved ET_REL symbol");
        if(section==0xfff2)return rel_fail("ET_REL COMMON symbols unsupported");
        if(section!=0xfff1){
            if(section>=count)return rel_fail("invalid ET_REL symbol section");
            RelSection *owner=&rel_sections[section];
            if(value>owner->size||size>owner->size-value)return rel_fail("ET_REL symbol outside section");
            /* Nonallocatable debug symbols are validated, but cannot become
               relocation values or the entry point in a runtime image. */
            if(owner->flags&2)value+=owner->address;
        }
        rel_symbols[index]=(RelSymbol){value,name,info>>4};
        if(info>>4){
            if(!*text)return rel_fail("unnamed ET_REL global symbol");
            for(unsigned previous=1;previous<index;previous++)if(rel_symbols[previous].binding&&!strcmp(text,(const char *)elf_image+strings->offset+rel_symbols[previous].name))
                return rel_fail("duplicate ET_REL global symbol");
            if(!strcmp(text,"_start")){
                if((info&15)!=2||section>=count||!(rel_sections[section].flags&4)||value&3||size<4)
                    return rel_fail("invalid ET_REL _start");
                entry=value;
            }
        }
    }
    if(!entry)return rel_fail("ET_REL executable _start missing");
    for(unsigned index=1;index<count;index++){
        RelSection *part=&rel_sections[index];if(part->type!=4)continue;
        if(part->entry!=24||part->size%24||part->link!=symtab||!part->info||part->info>=count||!(rel_sections[part->info].flags&2))
            return rel_fail("invalid ET_REL relocation table/target");
        RelSection *target=&rel_sections[part->info];
        if(part->size/24>REL_PATCHES-relocation_count)return rel_fail("too many ET_REL relocations");
        relocation_count+=(unsigned)(part->size/24);
        for(unsigned record=0;record<part->size/24;record++){
            unsigned at=(unsigned)(part->offset+record*24);
            uint64_t offset=elf_value(at,8,length),info=elf_value(at+8,8,length),symbol=info>>32;
            unsigned type=(unsigned)info;
            if(symbol>=symbol_count)return rel_fail("ET_REL relocation symbol out of bounds");
            if(!type){if(symbol||offset>target->size||elf_value(at+16,8,length))return rel_fail("invalid ET_REL NONE relocation");continue;}
            if(type!=2)return rel_fail("unsupported ET_REL relocation (only NONE/64)");
            unsigned owner=(unsigned)elf_value((unsigned)(symbols->offset+symbol*24+6),2,length);
            /* Index zero denotes S=0, used by the assembler for an absolute
               constant folded into r_addend. Other undefined symbols failed
               validation above; SHN_ABS values do not receive a section base. */
            if(symbol&&owner!=0xfff1&&!(rel_sections[owner].flags&2))return rel_fail("ET_REL relocation references nonallocatable symbol");
            if(offset>target->size||8>target->size-offset||((target->address+offset)&7))return rel_fail("invalid ET_REL relocation destination");
            __int128 value=(__int128)rel_symbols[symbol].value+(__int128)(int64_t)elf_value(at+16,8,length);
            if(value<0||value>(__int128)UINT64_MAX)return rel_fail("ET_REL relocation value overflow");
            uint64_t address=target->address+offset;
            for(unsigned previous=0;previous<patch_count;previous++)if(rel_overlap(address,8,rel_patches[previous].address,8))return rel_fail("overlapping ET_REL relocation writes");
            rel_patches[patch_count++]=(RelPatch){address,(uint64_t)value};
        }
    }
    /* Stack construction may fail on arguments. Do it before section publication
       too; no failed link installs section contents or an earlier valid patch. */
    uint64_t aux[]={6,PAGE,9,entry,0,0};
    if(!startup_stack(argc,argv,aux,sizeof aux/sizeof aux[0]))return 0;
    for(unsigned index=1;index<count;index++){
        RelSection *part=&rel_sections[index];if(!(part->flags&2)||!part->size)continue;
        extents[extent_count++]=(LoadExtent){part->address,part->size,4u|((part->flags&1)?2u:0u)|((part->flags&4)?1u:0u)};
        if(part->type==1)memcpy(guest_ram+part->address,elf_image+part->offset,(size_t)part->size);
        /* Guest RAM was zeroed on reset, including NOBITS and page padding. */
    }
    for(unsigned index=0;index<patch_count;index++)store(rel_patches[index].address,rel_patches[index].value,8);
    guest_pc=entry;load_bias=base;heap_end=align_page(cursor);return 1;
}
