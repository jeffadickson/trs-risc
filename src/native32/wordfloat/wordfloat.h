#ifndef N32_WORDFLOAT_H
#define N32_WORDFLOAT_H
#include <stdint.h>
/* Raw IEEE bits, precision 24 (binary32) or 53 (binary64). No C floating
 * arithmetic occurs in this API. DOUBLE mantissas use two RV32 words. */
uint64_t n32_wf_add(uint64_t left,uint64_t right,unsigned precision);
uint64_t n32_wf_mul(uint64_t left,uint64_t right,unsigned precision);
uint64_t n32_wf_div(uint64_t left,uint64_t right,unsigned precision);
uint64_t n32_wf_convert(uint64_t bits,unsigned from,unsigned to);
uint64_t n32_wf_integer(uint64_t magnitude,int negative,unsigned precision);
uint64_t n32_wf_fix(uint64_t bits,unsigned precision);
int n32_wf_compare(uint64_t left,uint64_t right,unsigned precision,int unordered);
#endif
