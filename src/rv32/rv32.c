/* Small single-hart RV32IMAC board simulator. The guest is the real native32
 * freestanding ELF, not the direct C/WASM interpreter. No F/D instruction is
 * implemented: guest floating arithmetic must use its integer word engine.
 * Fixed board: 256 KiB flash, 2 MiB RAM, polled 16550 UART. No OS/MMU model.
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
__attribute__((import_module("env"),import_name("uart_write"))) extern void uart_write(unsigned);
#define FLASH UINT32_C(0x20000000)
#define RAM UINT32_C(0x80000000)
static uint8_t flash[256*1024],ram[2*1024*1024],image[512*1024],keys[16384];
static uint32_t registers[32],pc,trap_pc,trap_instruction;static uint64_t cycles;
static unsigned fault,read_key,write_key,compressed,reservation_valid;static uint32_t reservation;static uint8_t lcr;
static int32_t sext(uint32_t value,unsigned bits){uint32_t mask=UINT32_C(1)<<(bits-1);return (int32_t)((value^mask)-mask);}
static uint8_t *region(uint32_t address,size_t length){
 if(address>=FLASH&&(uint64_t)address+length<=FLASH+sizeof flash)return flash+(address-FLASH);
 if(address>=RAM&&(uint64_t)address+length<=RAM+sizeof ram)return ram+(address-RAM);
 return NULL;
}
static uint32_t load(uint32_t address,unsigned width){
 if(address>=0x10000000&&address<0x10000008&&width==1){
  if(address==0x10000005)return 0x60|(read_key!=write_key);
  if(address==0x10000003)return lcr;
  if(address==0x10000000&&!(lcr&0x80)){if(read_key==write_key)return 0;uint8_t key=keys[read_key];read_key=(read_key+1)%sizeof keys;return key;}return 0;
 }
 uint8_t *bytes=region(address,width);if(!bytes){fault=2;return 0;}uint32_t value=0;for(unsigned i=0;i<width;i++)value|=(uint32_t)bytes[i]<<(8*i);return value;
}
static void store(uint32_t address,unsigned width,uint32_t value){
 if(address>=0x10000000&&address<0x10000008&&width==1){if(address==0x10000003)lcr=(uint8_t)value;if(address==0x10000000&&!(lcr&0x80))uart_write(value&255);return;}
 if(address<RAM||(uint64_t)address+width>RAM+sizeof ram){fault=3;return;}
 reservation_valid=0;for(unsigned i=0;i<width;i++)ram[address-RAM+i]=(uint8_t)(value>>(8*i));
}
static uint32_t immediate(uint32_t imm,unsigned rs,unsigned f,unsigned rd,unsigned opcode){return ((imm&4095)<<20)|(rs<<15)|(f<<12)|(rd<<7)|opcode;}
static uint32_t regop(unsigned rs2,unsigned rs1,unsigned f,unsigned rd,unsigned high){return (high<<25)|(rs2<<20)|(rs1<<15)|(f<<12)|(rd<<7)|0x33;}
static uint32_t jump(int32_t offset,unsigned rd){uint32_t x=(uint32_t)offset;return ((x>>20&1)<<31)|((x>>1&1023)<<21)|((x>>11&1)<<20)|((x>>12&255)<<12)|(rd<<7)|0x6f;}
static uint32_t branch(int32_t offset,unsigned rs,unsigned f){uint32_t x=(uint32_t)offset;return ((x>>12&1)<<31)|((x>>5&63)<<25)|(rs<<15)|(f<<12)|((x>>1&15)<<8)|((x>>11&1)<<7)|0x63;}
static uint32_t sw(unsigned offset,unsigned rs1,unsigned rs2){return ((offset>>5)<<25)|(rs2<<20)|(rs1<<15)|(2<<12)|((offset&31)<<7)|0x23;}
/* Expand compressed integer encodings to ordinary RV32 instructions. Floating
 * compressed encodings and RV64-only encodings remain illegal. */
static uint32_t expand(uint16_t c){
 unsigned quadrant=c&3,op=c>>13,rd=c>>7&31,rs2=c>>2&31,a=8+(c>>7&7),b=8+(c>>2&7);
 int32_t six=sext((c>>2&31)|((c>>12&1)<<5),6);unsigned offset;
 if(quadrant==0){
  if(op==0){offset=((c>>7&15)<<6)|((c>>11&3)<<4)|((c>>5&1)<<3)|((c>>6&1)<<2);if(offset)return immediate(offset,2,0,b,0x13);}
  offset=((c>>10&7)<<3)|((c>>6&1)<<2)|((c>>5&1)<<6);
  if(op==2)return immediate(offset,a,2,b,3);if(op==6)return sw(offset,a,b);
 }else if(quadrant==1){
  if(op==0)return immediate((uint32_t)six,rd,0,rd,0x13);
  if(op==2)return immediate((uint32_t)six,0,0,rd,0x13);
  if(op==3){if(rd==2){offset=((c>>12&1)<<9)|((c>>6&1)<<4)|((c>>5&1)<<6)|((c>>3&3)<<7)|((c>>2&1)<<5);if(offset)return immediate((uint32_t)sext(offset,10),2,0,2,0x13);}else if(rd&&six)return ((uint32_t)six<<12)|(rd<<7)|0x37;}
  if(op==1||op==5){offset=((c>>12&1)<<11)|((c>>11&1)<<4)|((c>>9&3)<<8)|((c>>8&1)<<10)|((c>>7&1)<<6)|((c>>6&1)<<7)|((c>>3&7)<<1)|((c>>2&1)<<5);return jump(sext(offset,12),op==1?1:0);}
  if(op==6||op==7){offset=((c>>12&1)<<8)|((c>>10&3)<<3)|((c>>5&3)<<6)|((c>>3&3)<<1)|((c>>2&1)<<5);return branch(sext(offset,9),a,op==6?0:1);}
  if(op==4){unsigned mode=c>>10&3;if(mode==2)return immediate((uint32_t)six,a,7,a,0x13);if(c&0x1000)return 0;
   if(mode<2)return immediate((c>>2&31)|(mode==1?0x400:0),a,5,a,0x13);
   unsigned select=c>>5&3;return regop(b,a,select==0?0:select==1?4:select==2?6:7,a,select==0?32:0);}
 }else if(quadrant==2){
  if(op==0&&!(c&0x1000))return immediate(c>>2&31,rd,1,rd,0x13);
  if(op==2&&rd){offset=((c>>12&1)<<5)|((c>>4&7)<<2)|((c>>2&3)<<6);return immediate(offset,2,2,rd,3);}
  if(op==6){offset=((c>>9&15)<<2)|((c>>7&3)<<6);return sw(offset,2,rs2);}
  if(op==4){if(!(c&0x1000)){if(!rs2&&rd)return immediate(0,rd,0,0,0x67);if(rs2)return regop(rs2,0,0,rd,0);}
   else{if(!rd&&!rs2)return 0x00100073;if(!rs2)return immediate(0,rd,0,1,0x67);return regop(rs2,rd,0,rd,0);}}
 }return 0;
}
static void step(void){
 trap_pc=pc;uint32_t instruction=load(pc,2),length=4;if(fault)return;
 if((instruction&3)!=3){length=2;if(!compressed){fault=4;return;}instruction=expand((uint16_t)instruction);}else instruction=load(pc,4);
 trap_instruction=instruction;if(fault)return;uint32_t next=pc+length,opcode=instruction&127,rd=instruction>>7&31,f=instruction>>12&7,rs1=instruction>>15&31,rs2=instruction>>20&31,high=instruction>>25;
 uint32_t x=registers[rs1],y=registers[rs2],result=0;int32_t imm=sext(instruction>>20,12);int writes=1;
 switch(opcode){
 case 0x37:result=instruction&0xfffff000;break;
 case 0x17:result=pc+(instruction&0xfffff000);break;
 case 0x6f:{uint32_t offset=((instruction>>31)<<20)|((instruction>>21&1023)<<1)|((instruction>>20&1)<<11)|(instruction&0xff000);result=next;next=pc+(uint32_t)sext(offset,21);break;}
 case 0x67:if(f){fault=4;break;}result=next;next=(x+(uint32_t)imm)&~1u;break;
 case 0x63:{writes=0;uint32_t offset=((instruction>>31)<<12)|((instruction>>7&1)<<11)|((instruction>>25&63)<<5)|((instruction>>8&15)<<1);int take=0;
  switch(f){case 0:take=x==y;break;case 1:take=x!=y;break;case 4:take=(int32_t)x<(int32_t)y;break;case 5:take=(int32_t)x>=(int32_t)y;break;case 6:take=x<y;break;case 7:take=x>=y;break;default:fault=4;}
  if(take)next=pc+(uint32_t)sext(offset,13);break;}
 case 3:{uint32_t address=x+(uint32_t)imm;switch(f){case 0:result=(uint32_t)sext(load(address,1),8);break;case 1:result=(uint32_t)sext(load(address,2),16);break;case 2:result=load(address,4);break;case 4:result=load(address,1);break;case 5:result=load(address,2);break;default:fault=4;}break;}
 case 0x23:{writes=0;uint32_t offset=(instruction>>25<<5)|(instruction>>7&31),address=x+(uint32_t)sext(offset,12);if(f>2)fault=4;else store(address,1u<<f,y);break;}
 case 0x13:switch(f){case 0:result=x+(uint32_t)imm;break;case 2:result=(int32_t)x<imm;break;case 3:result=x<(uint32_t)imm;break;case 4:result=x^(uint32_t)imm;break;case 6:result=x|(uint32_t)imm;break;case 7:result=x&(uint32_t)imm;break;case 1:if(high)fault=4;result=x<<rs2;break;case 5:if(high==0)result=x>>rs2;else if(high==32)result=(uint32_t)((int32_t)x>>rs2);else fault=4;break;}break;
 case 0x33:
  if(high==1){switch(f){case 0:result=x*y;break;case 1:result=(uint32_t)((uint64_t)((int64_t)(int32_t)x*(int64_t)(int32_t)y)>>32);break;case 2:result=(uint32_t)(((uint64_t)(int64_t)(int32_t)x*y)>>32);break;case 3:result=(uint32_t)(((uint64_t)x*y)>>32);break;
   case 4:result=!y?UINT32_MAX:x==0x80000000&&y==UINT32_MAX?x:(uint32_t)((int32_t)x/(int32_t)y);break;
   case 5:result=y?x/y:UINT32_MAX;break;case 6:result=!y?x:x==0x80000000&&y==UINT32_MAX?0:(uint32_t)((int32_t)x%(int32_t)y);break;case 7:result=y?x%y:x;break;}
  }else switch(f){case 0:if(high==0)result=x+y;else if(high==32)result=x-y;else fault=4;break;case 1:result=x<<(y&31);if(high)fault=4;break;case 2:result=(int32_t)x<(int32_t)y;if(high)fault=4;break;case 3:result=x<y;if(high)fault=4;break;case 4:result=x^y;if(high)fault=4;break;case 5:if(high==0)result=x>>(y&31);else if(high==32)result=(uint32_t)((int32_t)x>>(y&31));else fault=4;break;case 6:result=x|y;if(high)fault=4;break;case 7:result=x&y;if(high)fault=4;break;}break;
 case 0x0f:writes=0;break; /* one-hart FENCE/FENCE.I */
 case 0x2f:{if(f!=2||(x&3)){fault=4;break;}unsigned operation=instruction>>27;uint32_t old=load(x,4),value=0;result=old;
  switch(operation){case 2:writes=1;if(rs2)fault=4;reservation=x;reservation_valid=1;break;case 3:result=!(reservation_valid&&reservation==x);if(!result)store(x,4,y);reservation_valid=0;break;case 1:value=y;store(x,4,value);break;case 0:value=old+y;store(x,4,value);break;case 4:store(x,4,old^y);break;case 8:store(x,4,old|y);break;case 12:store(x,4,old&y);break;case 16:store(x,4,(int32_t)old<(int32_t)y?old:y);break;case 20:store(x,4,(int32_t)old>(int32_t)y?old:y);break;case 24:store(x,4,old<y?old:y);break;case 28:store(x,4,old>y?old:y);break;default:fault=4;}break;}
 case 0x73:{unsigned csr=instruction>>20;if(f&&rs1==0&&(csr==0xc00||csr==0xc02||csr==0xc80||csr==0xc82))result=(csr&128)?(uint32_t)(cycles>>32):(uint32_t)cycles;else fault=5;break;}
 default:fault=4;break;
 }
 if(next&1)fault=4;if(fault)return;if(writes&&rd)registers[rd]=result;registers[0]=0;pc=next;cycles++;
}
static uint32_t u32(const uint8_t *bytes){return (uint32_t)bytes[0]|(uint32_t)bytes[1]<<8|(uint32_t)bytes[2]<<16|(uint32_t)bytes[3]<<24;}
static unsigned u16(const uint8_t *bytes){return bytes[0]|bytes[1]<<8;}
uintptr_t rv32_image(void){return (uintptr_t)image;}
unsigned rv32_capacity(void){return sizeof image;}
/* Fixed-address ELF32 ET_EXEC only. Segment validation occurs before state
 * mutation; this is not a relocating linker or a Linux process loader. */
unsigned rv32_load(unsigned length){
 if(length<52||length>sizeof image||memcmp(image,"\177ELF",4)||image[4]!=1||image[5]!=1||u16(image+16)!=2||u16(image+18)!=243||u16(image+42)!=32)return 1;
 uint32_t at=u32(image+28),entry=u32(image+24);unsigned count=u16(image+44);
 if((uint64_t)at+(uint64_t)count*32>length||entry<FLASH||entry>=FLASH+sizeof flash||(entry&1))return 1;
 for(unsigned i=0;i<count;i++){const uint8_t *header=image+at+i*32;if(u32(header)!=1)continue;
  uint32_t offset=u32(header+4),address=u32(header+8),file=u32(header+16),memory=u32(header+20);
  if(file>memory||(uint64_t)offset+file>length||!region(address,memory))return 1;
 }
 memset(flash,0,sizeof flash);memset(ram,0,sizeof ram);memset(registers,0,sizeof registers);fault=0;reservation_valid=0;cycles=0;pc=entry;read_key=write_key=0;lcr=0;compressed=u32(image+36)&1;
 for(unsigned i=0;i<count;i++){const uint8_t *header=image+at+i*32;if(u32(header)!=1)continue;
  uint32_t offset=u32(header+4),address=u32(header+8),physical=u32(header+12),file=u32(header+16);
  memcpy(region(address,file),image+offset,file);
  if(physical!=address&&physical>=FLASH&&(uint64_t)physical+file<=FLASH+sizeof flash)memcpy(region(physical,file),image+offset,file);
 }return 0;
}
unsigned rv32_enqueue(unsigned byte){unsigned next=(write_key+1)%sizeof keys;if(byte>127||next==read_key)return 0;keys[write_key]=(uint8_t)byte;write_key=next;return 1;}
unsigned rv32_run(unsigned budget){for(unsigned i=0;i<budget&&!fault;i++)step();return fault;}
uint64_t rv32_cycles(void){return cycles;}
unsigned rv32_pc(void){return fault?trap_pc:pc;}
unsigned rv32_instruction(void){return trap_instruction;}

unsigned rv32_pending(void){return (write_key+sizeof keys-read_key)%sizeof keys;}
