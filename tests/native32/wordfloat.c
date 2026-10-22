/* Differential tests against host IEEE hardware. Production wordfloat uses
 * integer operations; the host oracle uses separately compiled float/double
 * operations. NaN payloads are intentionally not part of the contract. */
#include "wordfloat/wordfloat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
static uint64_t state=UINT64_C(0xa930cb17f421);static unsigned checks;
static uint64_t random_word(void){state^=state<<13;state^=state>>7;state^=state<<17;return state;}
static uint64_t bits(double value){uint64_t raw;memcpy(&raw,&value,8);return raw;}
static double real(uint64_t raw){double value;memcpy(&value,&raw,8);return value;}
static uint32_t fbits(float value){uint32_t raw;memcpy(&raw,&value,4);return raw;}
static float freal(uint32_t raw){float value;memcpy(&value,&raw,4);return value;}
static int nan_value(uint64_t value,unsigned p){uint64_t inf=p==24?UINT64_C(0x7f800000):UINT64_C(0x7ff0000000000000);return (value&inf)==inf&&(value&(p==24?UINT64_C(0x7fffff):UINT64_C(0xfffffffffffff)));}
static void check(const char *operation,uint64_t a,uint64_t b,unsigned p,uint64_t actual,uint64_t expected){
 checks++;if(actual==expected||(nan_value(actual,p)&&nan_value(expected,p)))return;
 fprintf(stderr,"FAIL %s p=%u a=%016"PRIx64" b=%016"PRIx64" actual=%016"PRIx64" expected=%016"PRIx64"\n",operation,p,a,b,actual,expected);exit(1);
}
static void pair(uint64_t a,uint64_t b,unsigned p){
 uint64_t sum,product,quotient;
 if(p==24){a=(uint32_t)a;b=(uint32_t)b;volatile float x=freal((uint32_t)a),y=freal((uint32_t)b);sum=fbits(x+y);product=fbits(x*y);quotient=fbits(x/y);}
 else{volatile double x=real(a),y=real(b);sum=bits(x+y);product=bits(x*y);quotient=bits(x/y);}
 check("add",a,b,p,n32_wf_add(a,b,p),sum);check("mul",a,b,p,n32_wf_mul(a,b,p),product);check("div",a,b,p,n32_wf_div(a,b,p),quotient);
 if(p==53){volatile double x=real(a);check("narrow",a,0,24,n32_wf_convert(a,53,24),fbits((float)x));}
 else{volatile float x=freal((uint32_t)a);check("widen",a,0,53,n32_wf_convert(a,24,53),bits((double)x));}
 if(!nan_value(a,p)&&!nan_value(b,p)){
  double x=p==24?(double)freal((uint32_t)a):real(a),y=p==24?(double)freal((uint32_t)b):real(b);
  int expected=x<y?-1:x>y?1:0,actual=n32_wf_compare(a,b,p,1);checks++;if(actual!=expected){fprintf(stderr,"FAIL compare\n");exit(1);}
 }
}
int main(void){
 static const uint64_t edges[]={0,UINT64_C(0x8000000000000000),1,UINT64_C(0x8000000000000001),
 UINT64_C(0x000fffffffffffff),UINT64_C(0x0010000000000000),UINT64_C(0x3ff0000000000000),UINT64_C(0x3fefffffffffffff),
 UINT64_C(0xbff0000000000000),UINT64_C(0x7fefffffffffffff),UINT64_C(0xffefffffffffffff),UINT64_C(0x7ff0000000000000),UINT64_C(0xfff0000000000000),UINT64_C(0x7ff8000000000000)};
 for(unsigned i=0;i<sizeof edges/sizeof *edges;i++)for(unsigned j=0;j<sizeof edges/sizeof *edges;j++){
  pair(edges[i],edges[j],53);pair(n32_wf_convert(edges[i],53,24),n32_wf_convert(edges[j],53,24),24);
 }
 for(unsigned i=0;i<100000;i++){
  uint64_t a=random_word(),b=random_word();pair(a,b,53);pair(a,b,24);
  check("uint64->double",a,0,53,n32_wf_integer(a,0,53),bits((double)a));
  check("uint64->single",a,0,24,n32_wf_integer(a,0,24),fbits((float)a));
  check("negative magnitude",a,0,53,n32_wf_integer(a,1,53),bits(-(double)a));
  /* Integer conversion is defined only within the C destination's range. */
  double x=real(a);if(x>=0&&x<0x1p64)check("double->uint64",a,0,53,n32_wf_fix(a,53),(uint64_t)x);
 }
 /* Concentrate on alignment, cancellation and gradual underflow, which
  * uniform random exponents otherwise exercise relatively infrequently. */
 for(unsigned i=0;i<20000;i++){
  uint64_t a=(random_word()&UINT64_C(0x800fffffffffffff))|((uint64_t)(64+i%1900)<<52);
  uint64_t b=(random_word()&UINT64_C(0x800fffffffffffff))|((uint64_t)(4+i%1900+i%120)<<52);
  pair(a,b,53);pair(a,a^UINT64_C(0x8000000000000000),53);
  pair(random_word()&UINT64_C(0x800fffffffffffff),random_word()&UINT64_C(0x800fffffffffffff),53);
  uint32_t x=(uint32_t)random_word()&UINT32_C(0x807fffff),y=(uint32_t)random_word()&UINT32_C(0x807fffff);
  pair(x,y,24);pair(x|UINT32_C(0x3f800000),y|UINT32_C(0x3f800000),24);
 }
 printf("PASS word floating point: %u exact hardware comparisons\n",checks);return 0;
}
