/* Included once per arithmetic precision. Every polynomial multiply/add has
 * REAL operands and an assignment boundary: SINGLE really uses 32-bit float,
 * DOUBLE uses 64-bit double. No implicit double-precision evaluation of SINGLE
 * and no FMA contraction (the Makefile sets -ffp-contract=off).
 *
 * ROM 14A9: LD A,(HL); INC HL; CALL 09B1 (load leading coefficient)
 *     14BB CALL 0847 (multiply); 14C3 CALL 0716 (add); JR 14B2
 * This is Horner's algorithm. The ROM's first table byte is a coefficient
 * COUNT, not a polynomial degree. That distinction matters for the last term.
 */
static REAL NAME(polynomial)(REAL x,const REAL *coefficients,unsigned count){
 REAL value=coefficients[0];for(unsigned i=1;i<count;i++){value=value*x;value=value+coefficients[i];}return value;
}
/* ROM 149A: save x; CALL 0847 to square it; CALL 14A9; multiply by saved x.
 * This shared odd polynomial evaluates x * P(x*x), directly in registers. */
static REAL NAME(odd)(REAL x,const REAL *coefficients,unsigned count){
 REAL square=x*x,answer=NAME(polynomial)(square,coefficients,count);return x*answer;
}
/* ROM 0809..0847 LOG:
 *   CALL 0955 / OR A / JP PE,1E4A             reject nonpositive x
 *   LD A,(4124); SUB 80h; LD (4124),80h      extract exponent, normalize
 *   BC=8035h DE=04F3h; ADD; INC B; DIV       form 1.414.../(m+.707...)
 *   LD HL,07F8; CALL 0710                   z = 1 - that quotient
 *   LD HL,07FC; CALL 149A                   z*P(z*z)
 *   BC=8080h DE=0; ADD; add saved exponent   k - .5 + odd polynomial
 *   BC=8031h DE=7218h; multiply             multiply by ln(2)
 * Native exponent extraction uses IEEE word fields, not eight-bit MBF bytes.
 */
static REAL NAME(logarithm)(REAL x){
 static const REAL coefficients[]={ (REAL)0x1.32ad54p-1,(REAL)0x1.ec45e2p-1,(REAL)0x1.71548ap+1 };
 int exponent;REAL mantissa=FRACTION(x,&exponent);
 REAL denominator=mantissa+(REAL)0x1.6a09e6p-1;
 REAL z=(REAL)1-(REAL)0x1.6a09e6p+0/denominator;
 REAL answer=NAME(odd)(z,coefficients,3);answer=answer-(REAL)0.5;answer=answer+(REAL)exponent;
 return answer*(REAL)0x1.62e43p-1;
}
/* ROM 1439..1476 EXP:
 * save x; multiply by log2(e); CALL 0B40 (floor); save n+1 as exponent;
 * CALL 070B (+1); CALL 0841 (*ln2); CALL 0713; CALL 0982
 * produces r=(floor(x*log2(e))+1)*ln2-x, NOT merely fract(x*log2(e)).
 * LD HL,1479; CALL 14A9 approximates exp(-r); multiply by 2^(n+1).
 * Native scaling handles the larger IEEE exponent ranges, including subnormals.
 */
#ifdef N32_DOUBLE_KERNEL
#include "exp_coefficients.inc"
#include "exp_double.inc"
#else
static REAL NAME(exponential)(REAL x){
 static const REAL coefficients[]={
  (REAL)-0x1.285c8p-13,(REAL)0x1.5c9eep-10,(REAL)-0x1.1004dcp-7,(REAL)0x1.5541ccp-5,
  (REAL)-0x1.5554ap-3,(REAL)0x1.fffffep-2,(REAL)-1,(REAL)1 };
 /* Bounds precede the integer cast. Values outside these broad bounds cannot
  * have finite nonzero results in either supported IEEE format. */
 if(x<(REAL)-2048)return (REAL)0;
 REAL binary=x*(REAL)0x1.715476p+0;int scale=(int)floor_word(binary)+1;
 REAL reduced=(REAL)scale*(REAL)0x1.62e43p-1;reduced=reduced-x;
 REAL answer=NAME(polynomial)(reduced,coefficients,8);
 while(scale>32){answer=answer*(REAL)0x1p32;scale-=32;}
 while(scale< -32){answer=answer*(REAL)0x1p-32;scale+=32;}
 while(scale>0){answer=answer*(REAL)2;scale--;}
 while(scale<0){answer=answer*(REAL)0.5;scale++;}return answer;
}
#endif
/* ROM 1547..1588 SIN:
 * divide by 2*pi; save fraction minus floor; form .25 minus fraction;
 * if negative add .5; negate when positive; add .25; restore initial sign;
 * LD HL,1593; JP 149A (five-coefficient odd polynomial).
 * This retains the original folding order, including its limited approximation
 * away from the best-conditioned interval. Do not replace it with host sin().
 */
static REAL NAME(sine)(REAL x){
 static const REAL coefficients[]={ (REAL)0x1.3daf74p+5,(REAL)-0x1.324cc8p+6,
  (REAL)0x1.4668bp+6,(REAL)-0x1.4abbcp+5,(REAL)0x1.921fb4p+2 };
 REAL turns=x/(REAL)0x1.921fb6p+2;REAL fraction=turns-(REAL)floor_word(turns);
 REAL folded=(REAL)0.25-fraction;bool negative=folded<0;
 if(negative)folded=folded+(REAL)0.5;
 if(folded>=0)folded=-folded;folded=folded+(REAL)0.25;if(negative)folded=-folded;
 return NAME(odd)(folded,coefficients,5);
}
/* ROM 15BD..15E2 ATN: save sign; negate negative argument; if exponent >=81h
 * replace x by 1/x and arrange return through 0710 (pi/2 minus result).
 * LD HL,15E3; CALL 149A evaluates the nine-coefficient odd polynomial.
 */
static REAL NAME(arctangent)(REAL x){
 static const REAL coefficients[]={ (REAL)0x1.77ae94p-9,(REAL)-0x1.08dc04p-6,
  (REAL)0x1.5f83fcp-5,(REAL)-0x1.3462e8p-4,(REAL)0x1.b47b08p-4,
  (REAL)-0x1.22ff9p-3,(REAL)0x1.9977c8p-3,(REAL)-0x1.5554d8p-2,(REAL)1 };
 bool negative=x<0;if(negative)x=-x;bool reciprocal=x>=(REAL)1;
 if(reciprocal)x=(REAL)1/x;REAL answer=NAME(odd)(x,coefficients,9);
 if(reciprocal)answer=(REAL)0x1.921fb6p+0-answer;return negative?-answer:answer;
}
static int NAME(math)(uint8_t token,REAL x,REAL *result){
 switch(token){
 case T_LOG:*result=NAME(logarithm)(x);break;
 case T_EXP:*result=NAME(exponential)(x);break;
 /* ROM 13E7: save x, load .5 from 1380, enter power at 13F5.
  * For a nonnegative root this is EXP(LOG(x)*.5), with zero special-cased. */
 case T_SQR:*result=x==0?(REAL)0:NAME(exponential)(NAME(logarithm)(x)*(REAL)0.5);break;
 case T_SIN:*result=NAME(sine)(x);break;
 /* ROM 1541 LD HL,158B; CALL 070B; falls through into SIN. */
 case T_COS:*result=NAME(sine)(x+(REAL)0x1.921fb6p+0);break;
 /* ROM 15A8 saves x; calls SIN and COS; JP 08A0 divides saved sin by cos. */
 case T_TAN:{REAL numerator=NAME(sine)(x),denominator=NAME(sine)(x+(REAL)0x1.921fb6p+0);if(denominator==0)return N32_DIVZERO;*result=numerator/denominator;break;}
 case T_ATN:*result=NAME(arctangent)(x);break;
 default:return N32_UNSUPPORTED;
 }return N32_OK;
}
