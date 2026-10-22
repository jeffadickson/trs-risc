/* DOUBLE EXP preserves the Level II algorithm: binary exponent reduction,
 * Horner approximation to exp(-r), then power-of-two scaling. Unlike the ROM
 * SINGLE path, its constants and polynomial target binary64 accuracy.
 *
 * ln2_hi has 32 significant bits, so multiplying it by |k| <= 1075 is exact.
 * ln2_lo supplies the remaining bits. Subtract x BEFORE adding k*ln2_lo;
 * using one rounded ln2 constant loses low bits through cancellation for
 * large arguments. The polynomial uses exact-rounded (-1)^j/j! coefficients
 * through degree 17. On 0 <= r <= ln(2), the degree-18 remainder is < 3e-19.
 * This is a bounded Taylor construction, not a claim of minimax optimality or
 * correctly rounded EXP for every binary64 input. See the independent vectors.
 */
static double double_exponential(double x){
 if(x< -746)return 0;
 int scale=(int)floor_word(x*0x1.71547652b82fep+0)+1;
 double reduced=(double)scale*0x1.62e42fee00000p-1-x;
 reduced=reduced+(double)scale*0x1.a39ef35793c76p-33;
 double answer=double_polynomial(reduced,n32_exp_coefficients,18);
 /* Construct exact binary powers with IEEE words. A single final multiply
  * rounds into the subnormal range; repeated halving there would double-round.
  * Large positive exponents similarly postpone overflow until the last step. */
 int power=scale;
 if(power>1023)power-=1023;
 else if(power< -1022)power+=1022;
 N32Value factor=n32_double(0);factor.as.wide.high=(uint32_t)(power+1023)<<20;
 answer=answer*n32_real(&factor);
 if(scale>1023)answer=answer*0x1p1023;
 else if(scale< -1022)answer=answer*0x1p-1022;
 return answer;
}
