/* ROM-derived functional port checks, independent from host libm accuracy.
 * Define ROM_ORACLE to call the pinned original Z80 routines through the old
 * translator. Ordinary RV32/WASM builds use fixed reference vectors below. */
#include "native.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#define CHECK(test) do{if(!(test)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#test);exit(1);}}while(0)
#ifdef ROM_ORACLE
extern int rom_math(unsigned entry,float input,double *result);
#endif
static double absolute(double x){return x<0?-x:x;}
static double call(uint8_t token,double x,N32Type type){N32Value input=type==N32_DOUBLE?n32_double(x):n32_single((float)x),out;CHECK(n32_math(token,&input,&out)==N32_OK);CHECK(out.type==type);return n32_real(&out);}
int main(void){
 struct {uint8_t token;unsigned entry;const char *name;} functions[]={
 {T_LOG,0x0809,"LOG"},{T_EXP,0x1439,"EXP"},{T_SQR,0x13e7,"SQR"},
 {T_SIN,0x1547,"SIN"},{T_COS,0x1541,"COS"},{T_TAN,0x15a8,"TAN"},{T_ATN,0x15bd,"ATN"}};
 /* These exact decoded MBF SINGLE outputs are from the pinned ROM, not libm.
  * A tolerance explicitly acknowledges IEEE operation-boundary rounding. */
 CHECK(absolute(call(T_LOG,2,N32_SINGLE)-0.69314718246459961)<2e-7);
 CHECK(absolute(call(T_EXP,1,N32_SINGLE)-2.7182819843292236)<1e-6);
 CHECK(absolute(call(T_SQR,2,N32_SINGLE)-1.4142135381698608)<1e-6);
 CHECK(absolute(call(T_SIN,5.5,N32_SINGLE)- -0.70554047822952271)<1e-6);
 CHECK(absolute(call(T_ATN,1,N32_SINGLE)-0.78539824485778809)<1e-6);
 N32Value input=n32_integer(-1),out=n32_integer(73);CHECK(n32_math(T_SQR,&input,&out)==N32_DOMAIN&&out.as.integer==73);
 input=n32_integer(0);CHECK(n32_math(T_LOG,&input,&out)==N32_DOMAIN);
 input=n32_double(2000);CHECK(n32_math(T_EXP,&input,&out)==N32_OVERFLOW);
 input=n32_double(-2000);CHECK(n32_math(T_EXP,&input,&out)==N32_OK&&n32_real(&out)==0);
 input=n32_single(90);CHECK(n32_math(T_EXP,&input,&out)==N32_OVERFLOW);
 input=n32_double(710);CHECK(n32_math(T_EXP,&input,&out)==N32_OVERFLOW);
 /* Expanded native exponent range is exercised outside the MBF ROM's range.
  * DOUBLE retains its storage precision, but these old polynomials do not
  * acquire 53-bit approximation accuracy merely by using wider registers. */
 CHECK(absolute(call(T_LOG,0x1p-1074,N32_DOUBLE)+744.4400719213812)<0.0001);
 CHECK(absolute(call(T_LOG,0x1p1000,N32_DOUBLE)-693.1471805599453)<0.0001);
 CHECK(call(T_EXP,700,N32_DOUBLE)>1e304);
 CHECK(call(T_SQR,0x1p-1074,N32_DOUBLE)>0);
 CHECK(absolute(call(T_LOG,0x1p-149,N32_SINGLE)+103.2789299)<0.0001);
 N32Value base=n32_double(9),exponent=n32_double(.5);CHECK(n32_math_power(&base,&exponent,&out)==N32_OK&&absolute(n32_real(&out)-3)<1e-5);
 base=n32_double(-9);CHECK(n32_math_power(&base,&exponent,&out)==N32_DOMAIN);
#ifdef ROM_ORACLE
 double original_error;CHECK(rom_math(0x0809,0,&original_error)==2);CHECK(rom_math(0x13e7,-1,&original_error)==2);
 unsigned total=0,identical=0;double worst=0;
 for(unsigned fn=0;fn<sizeof functions/sizeof *functions;fn++){
  double largest=0;
  for(int i=-96;i<=96;i++){
   float x=(float)i/16;if(functions[fn].token==T_LOG||functions[fn].token==T_SQR)x=(float)(i+97)/16;
   double original;CHECK(rom_math(functions[fn].entry,x,&original)==0);
   double native=call(functions[fn].token,x,N32_SINGLE);
   double error=absolute(native-original)/(absolute(original)>1?absolute(original):1);
   if(error>largest)largest=error;if(error>worst)worst=error;total++;if(native==original)identical++;
   if(error>0.00002){fprintf(stderr,"%s(%g): native %.17g ROM %.17g error %.9g\n",functions[fn].name,x,native,original,error);return 1;}
  }
  printf("PASS %s: 193 ROM comparisons, maximum scaled error %.9g\n",functions[fn].name,largest);
 }
 printf("ROM comparisons: %u total, %u exact, maximum scaled error %.9g\n",total,identical,worst);
#else
 (void)functions;
#endif
 puts("PASS native math: ROM vectors, domain errors, expanded IEEE ranges, fractional power");return 0;
}
