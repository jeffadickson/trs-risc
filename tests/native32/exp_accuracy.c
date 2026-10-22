/* Expected bits come from the independent C# BigInteger generator, not libm.
 * Inputs are exact binary64 bit patterns, including adjacent values near
 * range-reduction, overflow and subnormal boundaries. */
#include "native.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "exp_vectors.inc"
int main(void){
 uint64_t worst=0;unsigned finite=0,overflow=0;
 for(size_t i=0;i<sizeof exp_vectors/sizeof *exp_vectors;i++){
  N32Value input=n32_double(0),result=n32_integer(73);
  input.as.wide.low=(uint32_t)exp_vectors[i].input;input.as.wide.high=(uint32_t)(exp_vectors[i].input>>32);
  int status=n32_math(T_EXP,&input,&result);
  if(exp_vectors[i].expected==UINT64_C(0x7ff0000000000000)){
   if(status!=N32_OVERFLOW||result.type!=N32_INTEGER||result.as.integer!=73){fprintf(stderr,"FAIL overflow at %zu\n",i);return 1;}overflow++;continue;
  }
  if(status||result.type!=N32_DOUBLE){fprintf(stderr,"FAIL EXP status %d at %zu\n",status,i);return 1;}
  uint64_t actual=((uint64_t)result.as.wide.high<<32)|result.as.wide.low;
  uint64_t distance=actual>exp_vectors[i].expected?actual-exp_vectors[i].expected:exp_vectors[i].expected-actual;
  if(distance>worst)worst=distance;finite++;
#ifndef EXP_BASELINE
  if(distance>4){fprintf(stderr,"FAIL EXP input=%016"PRIx64" actual=%016"PRIx64" expected=%016"PRIx64" distance=%"PRIu64" ULP\n",exp_vectors[i].input,actual,exp_vectors[i].expected,distance);return 1;}
#endif
 }
 printf("PASS DOUBLE EXP: %u finite vectors, %u overflow cases, maximum distance %"PRIu64" ULP\n",finite,overflow,worst);return 0;
}
