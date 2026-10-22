/* Functional port of the pinned Model I Level II ROM's transcendental path.
 * This is ordinary arithmetic on native IEEE values, not a Z80 state machine.
 * Coefficients are exact conversions of ROM MBF SINGLE constants, so changing
 * register width does not silently replace Microsoft's approximation with libm.
 * The ROM's polynomial degree limits accuracy even for our DOUBLE entry points.
 * See math_kernel.inc for mixed Z80/algorithm comments at each function. */
#include "native.h"
#include <string.h>

/* Native replacement for 0AFBh's byte-by-byte shift and negative correction.
 * Clear IEEE fraction bits with a word mask. Large values are already integral;
 * avoid converting them to int32_t merely to perform range reduction. */
static double floor_word(double value){
 N32Value words=n32_double(value);uint64_t bits=((uint64_t)words.as.wide.high<<32)|words.as.wide.low;
 unsigned exponent=(unsigned)((bits>>52)&2047);
 if(exponent<1023)return value<0?-1:0;
 if(exponent>=1075)return value;
 uint64_t mask=(UINT64_C(1)<<(1075-exponent))-1;
 bool fraction=(bits&mask)!=0;bits&=~mask;
 words.as.wide.low=(uint32_t)bits;words.as.wide.high=(uint32_t)(bits>>32);
 double whole=n32_real(&words);return fraction&&value<0?whole-1:whole;
}
static float fraction_single(float value,int *exponent){
 uint32_t bits;memcpy(&bits,&value,4);int adjust=0;
 if(value<0x1p-126f){value*=0x1p24f;adjust=-24;memcpy(&bits,&value,4);}
 *exponent=(int)((bits>>23)&255)-126+adjust;
 bits=(bits&0x7fffff)|UINT32_C(0x3f000000);memcpy(&value,&bits,4);return value;
}
static double fraction_double(double value,int *exponent){
 N32Value words=n32_double(value);int adjust=0;
 if(value<0x1p-1022){value*=0x1p54;adjust=-54;words=n32_double(value);}
 *exponent=(int)((words.as.wide.high>>20)&2047)-1022+adjust;
 words.as.wide.high=(words.as.wide.high&0xfffff)|UINT32_C(0x3fe00000);return n32_real(&words);
}
#define REAL float
#define NAME(x) single_##x
#define FRACTION fraction_single
#include "math_kernel.inc"
#undef REAL
#undef NAME
#undef FRACTION
#define REAL double
#define NAME(x) double_##x
#define FRACTION fraction_double
#include "math_kernel.inc"
#undef REAL
#undef NAME
#undef FRACTION

int n32_math(uint8_t token,const N32Value *input,N32Value *result){
 if(input->type==N32_STRING)return N32_TYPE;
 double real=n32_real(input);N32Value bits=n32_double(real);
 if((bits.as.wide.high&0x7ff00000)==0x7ff00000)return N32_OVERFLOW;
 if(token==T_EXP&&real>1024)return N32_OVERFLOW;
 if((token==T_LOG&&real<=0)||(token==T_SQR&&real<0))return N32_DOMAIN;
 N32Value answer;
 if(input->type==N32_DOUBLE){double out;int status=double_math(token,real,&out);if(status)return status;answer=n32_double(out);}
 else{float out;int status=single_math(token,(float)real,&out);if(status)return status;answer=n32_single(out);}
 /* Conversion checks infinity before publishing the result, including TAN at
  * an exactly zero polynomial denominator and EXP beyond the IEEE range. */
 return n32_convert(&answer,answer.type,result);
}
int n32_math_power(const N32Value *base,const N32Value *exponent,N32Value *result){
 if(base->type==N32_STRING||exponent->type==N32_STRING)return N32_TYPE;
 double x=n32_real(base),y=n32_real(exponent);
 N32Value checked;int validation=n32_convert(base,N32_DOUBLE,&checked);if(validation)return validation;validation=n32_convert(exponent,N32_DOUBLE,&checked);if(validation)return validation;
 if(y==0){*result=base->type==N32_DOUBLE||exponent->type==N32_DOUBLE?n32_double(1):n32_single(1);return N32_OK;}
 if(x==0){if(y<0)return N32_DIVZERO;*result=base->type==N32_DOUBLE||exponent->type==N32_DOUBLE?n32_double(0):n32_single(0);return N32_OK;}
 bool negative=x<0;if(negative&&floor_word(y)!=y)return N32_DOMAIN;
 N32Type type=base->type==N32_DOUBLE||exponent->type==N32_DOUBLE?N32_DOUBLE:N32_SINGLE;
 N32Value positive=type==N32_DOUBLE?n32_double(negative?-x:x):n32_single((float)(negative?-x:x)),logarithm,product;
 int status=n32_math(T_LOG,&positive,&logarithm);if(status)return status;
 N32Value power=type==N32_DOUBLE?n32_double(y):n32_single((float)y);
 status=n32_binary('*',&logarithm,&power,&product);if(status)return status;
 status=n32_math(T_EXP,&product,result);
 /* Native parity test does not narrow an exponent to the Z80's 8-bit low
  * byte. Beyond 2^53 every representable binary64 integer is even. */
 if(!status&&negative&&y-floor_word(y*0.5)*2!=0)status=n32_unary('-',result,result);
 return status;
}
