/* Compiler ABI bridge only: arguments/results are moved as raw IEEE bits.
 * These strong definitions replace generic compiler-rt floating helpers in
 * RV32IMA builds. There are no float expressions in their implementations. */
#include "wordfloat.h"
#include <string.h>
static uint64_t dbits(double value){uint64_t bits;memcpy(&bits,&value,8);return bits;}
static double dvalue(uint64_t bits){double value;memcpy(&value,&bits,8);return value;}
static uint32_t fbits(float value){uint32_t bits;memcpy(&bits,&value,4);return bits;}
static float fvalue(uint64_t bits){uint32_t word=(uint32_t)bits;float value;memcpy(&value,&word,4);return value;}
double __adddf3(double x,double y){return dvalue(n32_wf_add(dbits(x),dbits(y),53));}
double __subdf3(double x,double y){return dvalue(n32_wf_add(dbits(x),dbits(y)^UINT64_C(0x8000000000000000),53));}
double __muldf3(double x,double y){return dvalue(n32_wf_mul(dbits(x),dbits(y),53));}
double __divdf3(double x,double y){return dvalue(n32_wf_div(dbits(x),dbits(y),53));}
float __addsf3(float x,float y){return fvalue(n32_wf_add(fbits(x),fbits(y),24));}
float __subsf3(float x,float y){return fvalue(n32_wf_add(fbits(x),fbits(y)^UINT32_C(0x80000000),24));}
float __mulsf3(float x,float y){return fvalue(n32_wf_mul(fbits(x),fbits(y),24));}
float __divsf3(float x,float y){return fvalue(n32_wf_div(fbits(x),fbits(y),24));}
double __extendsfdf2(float x){return dvalue(n32_wf_convert(fbits(x),24,53));}
float __truncdfsf2(double x){return fvalue(n32_wf_convert(dbits(x),53,24));}
#define COMPARE_D(name,unordered) int name(double x,double y){return n32_wf_compare(dbits(x),dbits(y),53,unordered);}
#define COMPARE_F(name,unordered) int name(float x,float y){return n32_wf_compare(fbits(x),fbits(y),24,unordered);}
COMPARE_D(__ledf2,1) COMPARE_D(__ltdf2,1) COMPARE_D(__gedf2,-1) COMPARE_D(__gtdf2,-1)
COMPARE_D(__eqdf2,1) COMPARE_D(__nedf2,1) COMPARE_D(__cmpdf2,1)
COMPARE_F(__lesf2,1) COMPARE_F(__ltsf2,1) COMPARE_F(__gesf2,-1) COMPARE_F(__gtsf2,-1)
COMPARE_F(__eqsf2,1) COMPARE_F(__nesf2,1) COMPARE_F(__cmpsf2,1)
/* Core conversions check their BASIC ranges before these C-ABI conversions.
 * Signed magnitudes are computed with unsigned subtraction, including MIN. */
double __floatsidf(int32_t x){uint32_t magnitude=x<0?0u-(uint32_t)x:(uint32_t)x;return dvalue(n32_wf_integer(magnitude,x<0,53));}
float __floatsisf(int32_t x){uint32_t magnitude=x<0?0u-(uint32_t)x:(uint32_t)x;return fvalue(n32_wf_integer(magnitude,x<0,24));}
double __floatunsidf(uint32_t x){return dvalue(n32_wf_integer(x,0,53));}
float __floatunsisf(uint32_t x){return fvalue(n32_wf_integer(x,0,24));}
double __floatdidf(int64_t x){return dvalue(n32_wf_integer(x<0?0-(uint64_t)x:(uint64_t)x,x<0,53));}
float __floatdisf(int64_t x){return fvalue(n32_wf_integer(x<0?0-(uint64_t)x:(uint64_t)x,x<0,24));}
double __floatundidf(uint64_t x){return dvalue(n32_wf_integer(x,0,53));}
float __floatundisf(uint64_t x){return fvalue(n32_wf_integer(x,0,24));}
int32_t __fixdfsi(double x){return (int32_t)n32_wf_fix(dbits(x),53);}
uint32_t __fixunsdfsi(double x){return (uint32_t)n32_wf_fix(dbits(x),53);}
int64_t __fixdfdi(double x){uint64_t bits=n32_wf_fix(dbits(x),53);int64_t out;memcpy(&out,&bits,8);return out;}
uint64_t __fixunsdfdi(double x){return n32_wf_fix(dbits(x),53);}
int32_t __fixsfsi(float x){return (int32_t)n32_wf_fix(fbits(x),24);}
uint32_t __fixunssfsi(float x){return (uint32_t)n32_wf_fix(fbits(x),24);}
int64_t __fixsfdi(float x){uint64_t bits=n32_wf_fix(fbits(x),24);int64_t out;memcpy(&out,&bits,8);return out;}
uint64_t __fixunssfdi(float x){return n32_wf_fix(fbits(x),24);}
