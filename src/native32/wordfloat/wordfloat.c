/* ROM-style floating arithmetic using integer registers only.
 *
 * The Level II ROM operates on sign, exponent and byte-array significands.
 * We preserve that decomposition but use RV32 words: SINGLE has a 24-bit
 * significand; DOUBLE has 53 bits in two words. Three extra low bits retain
 * guard, round and sticky information. All arithmetic shares one pack/round
 * path and rounds to nearest, ties to even, including subnormal results.
 *
 * This is new IEEE arithmetic, not byte-for-byte MBF compatibility. No host
 * float operation or compiler floating helper is called from this engine.
 */
#include "wordfloat.h"
#include <stdbool.h>
typedef struct {uint64_t significand;int exponent;unsigned sign,kind;} Parts;
enum {FINITE,ZERO,INFINITY_VALUE,NAN_VALUE};
static unsigned exponent_bits(unsigned precision){return precision==24?8:11;}
static int bias(unsigned precision){return precision==24?127:1023;}
static uint64_t sign_bit(unsigned precision){return UINT64_C(1)<<(precision+exponent_bits(precision)-1);}
static uint64_t hidden(unsigned precision){return UINT64_C(1)<<(precision-1);}
static uint64_t infinity(unsigned precision){return ((UINT64_C(1)<<exponent_bits(precision))-1)<<(precision-1);}
static uint64_t nan_bits(unsigned precision){return infinity(precision)|(hidden(precision)>>1);}
/* Right shift with a sticky low bit: all discarded nonzero information is
 * retained, even when the exponent gap is larger than either register pair. */
static uint64_t shift_jam(uint64_t value,unsigned shift){
 if(!shift)return value;if(shift>=64)return value!=0;
 return (value>>shift)|((value<<(64-shift))!=0);
}
static Parts unpack(uint64_t bits,unsigned precision){
 unsigned ebits=exponent_bits(precision);unsigned field=(unsigned)((bits>>(precision-1))&((1u<<ebits)-1));
 Parts value={.significand=bits&(hidden(precision)-1),.exponent=1-bias(precision),.sign=(bits&sign_bit(precision))!=0,.kind=FINITE};
 if(field==(1u<<ebits)-1){value.kind=value.significand?NAN_VALUE:INFINITY_VALUE;return value;}
 if(field){value.significand|=hidden(precision);value.exponent=(int)field-bias(precision);}
 else if(!value.significand)value.kind=ZERO;
 else while(value.significand<hidden(precision)){value.significand<<=1;value.exponent--;}
 return value;
}
/* Native replacement for the ROM's byte normalization/rounding loops
 * (0765h..07B1h for SINGLE). The IEEE hidden bit is implicit in storage.
 * Rounding happens AFTER shifting into the subnormal range, avoiding an
 * intermediate rounding followed by a second underflow rounding. */
static uint64_t pack(unsigned sign,int exponent,uint64_t extended,unsigned precision){
 uint64_t sign_mask=sign?sign_bit(precision):0;
 int minimum=1-bias(precision);
 if(exponent<minimum){extended=shift_jam(extended,(unsigned)(minimum-exponent));exponent=minimum;}
 uint64_t significand=extended>>3;unsigned tail=(unsigned)(extended&7);
 if(tail>4||(tail==4&&(significand&1)))significand++;
 if(significand>=hidden(precision)*2){significand>>=1;exponent++;}
 if(exponent>bias(precision))return sign_mask|infinity(precision);
 unsigned field=significand>=hidden(precision)?(unsigned)(exponent+bias(precision)):0;
 return sign_mask|((uint64_t)field<<(precision-1))|(significand&(hidden(precision)-1));
}
uint64_t n32_wf_add(uint64_t left,uint64_t right,unsigned precision){
 Parts x=unpack(left,precision),y=unpack(right,precision);
 if(x.kind==NAN_VALUE||y.kind==NAN_VALUE)return nan_bits(precision);
 if(x.kind==INFINITY_VALUE||y.kind==INFINITY_VALUE){
  if(x.kind==y.kind&&x.sign!=y.sign)return nan_bits(precision);
  return x.kind==INFINITY_VALUE?left:right;
 }
 if(x.kind==ZERO&&y.kind==ZERO)return x.sign&&y.sign?sign_bit(precision):0;
 if(x.kind==ZERO)return right;if(y.kind==ZERO)return left;
 /* Align the smaller significand, then add/subtract word pairs. This replaces
  * the byte carry chains used by Z80 ADD/ADC and SUB/SBC in the ROM. */
 if(x.exponent<y.exponent||(x.exponent==y.exponent&&x.significand<y.significand)){Parts swap=x;x=y;y=swap;}
 uint64_t large=x.significand<<3,small=shift_jam(y.significand<<3,(unsigned)(x.exponent-y.exponent));
 uint64_t result;
 if(x.sign==y.sign){result=large+small;if(result>=hidden(precision)*16){result=shift_jam(result,1);x.exponent++;}}
 else{result=large-small;if(!result)return 0;while(result<hidden(precision)*8){result<<=1;x.exponent--;}}
 return pack(x.sign,x.exponent,result,precision);
}
/* Four native 32x32 products replace the ROM's repeated single-bit adds and
 * shifts (DOUBLE multiplication at 0DA1h..0DD2h). The full 106-bit product is
 * retained; no general 128-bit library or __int128 type is required. */
static uint64_t product_extended(uint64_t left,uint64_t right,unsigned precision){
 uint32_t a0=(uint32_t)left,a1=(uint32_t)(left>>32),b0=(uint32_t)right,b1=(uint32_t)(right>>32);
 uint64_t p0=(uint64_t)a0*b0,p1=(uint64_t)a0*b1,p2=(uint64_t)a1*b0,p3=(uint64_t)a1*b1;
 uint64_t low=p0+(p1<<32),carry=low<p0,previous=low;low+=p2<<32;carry+=low<previous;
 uint64_t high=p3+(p1>>32)+(p2>>32)+carry;
 unsigned shift=precision-4;
 return (low>>shift)|(high<<(64-shift))|((low<<(64-shift))!=0);
}
uint64_t n32_wf_mul(uint64_t left,uint64_t right,unsigned precision){
 Parts x=unpack(left,precision),y=unpack(right,precision);unsigned sign=x.sign^y.sign;
 if(x.kind==NAN_VALUE||y.kind==NAN_VALUE)return nan_bits(precision);
 if(x.kind==INFINITY_VALUE||y.kind==INFINITY_VALUE){if(x.kind==ZERO||y.kind==ZERO)return nan_bits(precision);return (sign?sign_bit(precision):0)|infinity(precision);}
 if(x.kind==ZERO||y.kind==ZERO)return sign?sign_bit(precision):0;
 uint64_t extended=product_extended(x.significand,y.significand,precision);int exponent=x.exponent+y.exponent;
 if(extended>=hidden(precision)*16){extended=shift_jam(extended,1);exponent++;}
 return pack(sign,exponent,extended,precision);
}
uint64_t n32_wf_div(uint64_t left,uint64_t right,unsigned precision){
 Parts x=unpack(left,precision),y=unpack(right,precision);unsigned sign=x.sign^y.sign;uint64_t sign_mask=sign?sign_bit(precision):0;
 if(x.kind==NAN_VALUE||y.kind==NAN_VALUE)return nan_bits(precision);
 if((x.kind==ZERO&&y.kind==ZERO)||(x.kind==INFINITY_VALUE&&y.kind==INFINITY_VALUE))return nan_bits(precision);
 if(x.kind==INFINITY_VALUE||y.kind==ZERO)return sign_mask|infinity(precision);
 if(x.kind==ZERO||y.kind==INFINITY_VALUE)return sign_mask;
 int exponent=x.exponent-y.exponent;uint64_t remainder=x.significand,divisor=y.significand,quotient=0;
 if(remainder<divisor){remainder<<=1;exponent--;}
 /* Restoring division, as in the ROM, but each subtract/compare handles a
  * whole RV32 word pair instead of seven separate significand bytes. Emit
  * p+3 bits and jam the final remainder into the sticky bit. */
 for(unsigned i=0;i<precision+3;i++){quotient<<=1;if(remainder>=divisor){remainder-=divisor;quotient|=1;}remainder<<=1;}
 if(remainder)quotient|=1;return pack(sign,exponent,quotient,precision);
}
uint64_t n32_wf_convert(uint64_t bits,unsigned from,unsigned to){
 Parts value=unpack(bits,from);uint64_t sign=value.sign?sign_bit(to):0;
 if(value.kind==NAN_VALUE)return sign|nan_bits(to);
 if(value.kind==INFINITY_VALUE)return sign|infinity(to);
 if(value.kind==ZERO)return sign;
 uint64_t extended=value.significand<<3;
 if(from>to)extended=shift_jam(extended,from-to);else extended<<=to-from;
 return pack(value.sign,value.exponent,extended,to);
}
uint64_t n32_wf_integer(uint64_t magnitude,int negative,unsigned precision){
 if(!magnitude)return 0;uint64_t scan=magnitude;unsigned top=0;while(scan>>1){scan>>=1;top++;}
 unsigned target=precision+2;uint64_t extended=top>target?shift_jam(magnitude,top-target):magnitude<<(target-top);
 return pack(negative!=0,(int)top,extended,precision);
}
uint64_t n32_wf_fix(uint64_t bits,unsigned precision){
 Parts value=unpack(bits,precision);if(value.kind==ZERO||value.exponent<0)return 0;
 if(value.kind!=FINITE||value.exponent>63)return UINT64_MAX;
 int shift=value.exponent-(int)(precision-1);
 uint64_t magnitude=shift>=0?value.significand<<shift:value.significand>>-shift;
 return value.sign?0-magnitude:magnitude;
}
int n32_wf_compare(uint64_t left,uint64_t right,unsigned precision,int unordered){
 Parts x=unpack(left,precision),y=unpack(right,precision);
 if(x.kind==NAN_VALUE||y.kind==NAN_VALUE)return unordered;
 if((x.kind==ZERO&&y.kind==ZERO)||left==right)return 0;
 if(x.sign!=y.sign)return x.sign?-1:1;
 int result=left<right?-1:1;return x.sign?-result:result;
}
