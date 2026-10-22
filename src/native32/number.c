/* Native32 numeric policy. This deliberately stops reproducing Z80 INTEGER
 * limits, MBF encodings and incidental rounding bugs. Integer overflow promotes
 * to DOUBLE, not SINGLE, so ordinary32-bit integer results are not immediately
 * rounded to24-bit precision. No host strtod, printf or libm is used here.
 */
#include "native.h"
#include <stdlib.h>
#include <string.h>
#include <float.h>
_Static_assert(FLT_RADIX==2&&FLT_MANT_DIG==24&&DBL_MANT_DIG==53,"IEEE numeric precision");
N32Value n32_integer(int32_t value){N32Value out={0};out.as.integer=value;return out;}
N32Value n32_single(float value){N32Value out={.type=N32_SINGLE};out.as.single=value;return out;}
N32Value n32_double(double value){
 N32Value out={.type=N32_DOUBLE};uint64_t bits;memcpy(&bits,&value,8);
 out.as.wide.low=(uint32_t)bits;out.as.wide.high=(uint32_t)(bits>>32);return out;
}
double n32_real(const N32Value *value){
 if(value->type==N32_INTEGER)return value->as.integer;
 if(value->type==N32_SINGLE)return value->as.single;
 uint64_t bits=((uint64_t)value->as.wide.high<<32)|value->as.wide.low;double real;memcpy(&real,&bits,8);return real;
}
static bool real_finite(double value){uint64_t bits;memcpy(&bits,&value,8);return (bits&UINT64_C(0x7ff0000000000000))!=UINT64_C(0x7ff0000000000000);}
int n32_to_integer(const N32Value *value,int32_t *result){
 if(!value||!result)return N32_DOMAIN;
 if(value->type==N32_STRING)return N32_TYPE;
 if(value->type==N32_INTEGER){*result=value->as.integer;return N32_OK;}
 double real=n32_real(value);
 /* Check the full half-open signed32 interval BEFORE the C conversion. This
  * permits INT32_MIN and rejects positive2^31 without undefined conversion. */
 if(!real_finite(real)||real< -2147483648.0||real>=2147483648.0)return N32_OVERFLOW;
 *result=(int32_t)real;return N32_OK;
}
int n32_convert(const N32Value *value,N32Type type,N32Value *result){
 if(!value||!result||type>N32_STRING)return N32_DOMAIN;
 if(type==N32_STRING)return value->type==N32_STRING?n32_copy(value,result):N32_TYPE;
 if(value->type==N32_STRING)return N32_TYPE;
 N32Value answer;int status=N32_OK;
 if(type==N32_INTEGER){int32_t integer;status=n32_to_integer(value,&integer);if(status!=N32_OK)return status;answer=n32_integer(integer);}
 else{double real=n32_real(value);if(!real_finite(real))return N32_OVERFLOW;
  if(type==N32_SINGLE){float single=(float)real;if(!real_finite(single))return N32_OVERFLOW;answer=n32_single(single);}
  else answer=n32_double(real);
 }
 *result=answer;return status;
}
int n32_string(const uint8_t *bytes,size_t length,N32Value *result){
 if(!result||(!bytes&&length)||length>255)return N32_DOMAIN;
 uint8_t *copy=length?malloc(length):NULL;if(length&&!copy)return N32_MEMORY;
 if(length)memcpy(copy,bytes,length);*result=(N32Value){.type=N32_STRING,.as.string={copy,(uint8_t)length}};return N32_OK;
}
int n32_copy(const N32Value *value,N32Value *result){
 if(!value||!result)return N32_DOMAIN;
 if(value==result)return N32_OK;
 if(value->type==N32_STRING)return n32_string(value->as.string.bytes,value->as.string.length,result);
 *result=*value;return N32_OK;
}
void n32_clear(N32Value *value){if(value){if(value->type==N32_STRING)free(value->as.string.bytes);*value=n32_integer(0);}}
static int real_result(double real,N32Type type,N32Value *result){
 if(!real_finite(real))return N32_OVERFLOW;
 N32Value answer=n32_double(real);return n32_convert(&answer,type,result);
}
int n32_compare(const N32Value *a,const N32Value *b,int *order){
 if((a->type==N32_STRING)!=(b->type==N32_STRING))return N32_TYPE;
 if(a->type==N32_STRING){size_t n=a->as.string.length<b->as.string.length?a->as.string.length:b->as.string.length;
  int cmp=n?memcmp(a->as.string.bytes,b->as.string.bytes,n):0;
  *order=cmp<0?-1:cmp>0?1:a->as.string.length>b->as.string.length?1:a->as.string.length<b->as.string.length?-1:0;
 }else if(a->type==N32_INTEGER&&b->type==N32_INTEGER)*order=a->as.integer>b->as.integer?1:a->as.integer<b->as.integer?-1:0;
 else{double x=n32_real(a),y=n32_real(b);*order=x>y?1:x<y?-1:0;}
 return N32_OK;
}
int n32_binary(uint8_t op,const N32Value *a,const N32Value *b,N32Value *result){
 if(a->type==N32_STRING||b->type==N32_STRING){
  if(op!='+'||a->type!=N32_STRING||b->type!=N32_STRING)return N32_TYPE;
  size_t total=(size_t)a->as.string.length+b->as.string.length;if(total>255)return N32_DOMAIN;
  uint8_t text[255];if(a->as.string.length)memcpy(text,a->as.string.bytes,a->as.string.length);
  if(b->as.string.length)memcpy(text+a->as.string.length,b->as.string.bytes,b->as.string.length);
  return n32_string(text,total,result);
 }
 if(op==T_AND||op==T_OR){int32_t x,y;int status=n32_to_integer(a,&x);if(status)return status;status=n32_to_integer(b,&y);if(status)return status;
  uint32_t bits=op==T_AND?(uint32_t)x&(uint32_t)y:(uint32_t)x|(uint32_t)y;int32_t signed_bits;memcpy(&signed_bits,&bits,4);*result=n32_integer(signed_bits);return N32_OK;
 }
 if(a->type==N32_INTEGER&&b->type==N32_INTEGER&&(op=='+'||op=='-'||op=='*')){
  int32_t answer;bool overflow=op=='+'?__builtin_add_overflow(a->as.integer,b->as.integer,&answer):op=='-'?__builtin_sub_overflow(a->as.integer,b->as.integer,&answer):__builtin_mul_overflow(a->as.integer,b->as.integer,&answer);
  if(!overflow){*result=n32_integer(answer);return N32_OK;}
 }
 N32Type type=a->type==N32_DOUBLE||b->type==N32_DOUBLE||
  (a->type==N32_INTEGER&&b->type==N32_INTEGER)?N32_DOUBLE:N32_SINGLE;
 /* Explicit SINGLE expressions really calculate in binary32, rather than
  * calculating in binary64 and rounding only the final displayed value. */
 if(type==N32_SINGLE){float x=(float)n32_real(a),y=(float)n32_real(b),v;
  switch(op){case '+':v=x+y;break;case '-':v=x-y;break;case '*':v=x*y;break;case '/':if(y==0)return N32_DIVZERO;v=x/y;break;default:return N32_UNSUPPORTED;}
  return real_result(v,type,result);
 }
 double x=n32_real(a),y=n32_real(b),v;
 switch(op){case '+':v=x+y;break;case '-':v=x-y;break;case '*':v=x*y;break;case '/':if(y==0)return N32_DIVZERO;v=x/y;break;default:return N32_UNSUPPORTED;}
 return real_result(v,type,result);
}
int n32_unary(uint8_t op,const N32Value *value,N32Value *result){
 if(value->type==N32_STRING)return N32_TYPE;
 if(op=='+'){*result=*value;return N32_OK;}
 if(op==T_NOT){int32_t integer;int status=n32_to_integer(value,&integer);if(status)return status;uint32_t bits=~(uint32_t)integer;memcpy(&integer,&bits,4);*result=n32_integer(integer);return N32_OK;}
 if(op!='-')return N32_UNSUPPORTED;
 if(value->type==N32_INTEGER){if(value->as.integer!=INT32_MIN){*result=n32_integer(-value->as.integer);return N32_OK;}*result=n32_double(2147483648.0);return N32_OK;}
 return real_result(-n32_real(value),value->type,result);
}
int n32_parse_number(const uint8_t *text,size_t length,size_t *used,N32Value *result){
 if(!text||!used||!result)return N32_DOMAIN;
 size_t at=0,digits=0,fraction=0;bool point=false;double value=0;uint64_t integer=0;bool integer_fits=true;
 while(at<length){uint8_t ch=text[at];
  if(ch>='0'&&ch<='9'){unsigned digit=ch-'0';value=value*10.0+digit;if(!real_finite(value))return N32_OVERFLOW;
   if(integer_fits){if(integer>(UINT64_MAX-digit)/10)integer_fits=false;else integer=integer*10+digit;}
   digits++;if(point)fraction++;at++;
  }else if(ch=='.'&&!point){point=true;at++;}else break;
 }
 if(!digits)return N32_SYNTAX;
 N32Type type=!point&&integer_fits&&integer<=INT32_MAX?N32_INTEGER:N32_SINGLE;
 if(!point&&(!integer_fits||integer>INT32_MAX))type=N32_DOUBLE;
 int exponent=0;bool exponent_seen=false;
 if(at<length&&(text[at]=='E'||text[at]=='e'||text[at]=='D'||text[at]=='d')){
  exponent_seen=true;type=text[at]=='D'||text[at]=='d'?N32_DOUBLE:N32_SINGLE;at++;bool negative=false;
  if(at<length&&(text[at]=='+'||text[at]=='-'))negative=text[at++]=='-';size_t start=at;
  while(at<length&&text[at]>='0'&&text[at]<='9'){if(exponent>400)return N32_OVERFLOW;exponent=exponent*10+(text[at++]-'0');}
  if(at==start)return N32_SYNTAX;
  if(negative)exponent=-exponent;
 }
 if(at<length&&(text[at]=='%'||text[at]=='!'||text[at]=='#')){type=text[at]=='%'?N32_INTEGER:text[at]=='!'?N32_SINGLE:N32_DOUBLE;at++;}
 if(fraction>4096)return N32_OVERFLOW;exponent-=(int)fraction;
 if(exponent>400)return N32_OVERFLOW;
 if(exponent< -400)value=0;
 else{while(exponent>0){value*=10;exponent--;}while(exponent<0){value/=10;exponent++;}}
 N32Value answer;
 if(type==N32_INTEGER&&!point&&!exponent_seen&&integer_fits&&integer<=INT32_MAX)answer=n32_integer((int32_t)integer);
 else{N32Value real=n32_double(value);int status=n32_convert(&real,type,&answer);if(status)return status;}
 *result=answer;*used=at;return N32_OK;
}
int n32_format(const N32Value *value,char *text,size_t capacity,size_t *length){
 if(!value||!text||!length||capacity<32)return N32_DOMAIN;
 if(value->type==N32_STRING)return N32_TYPE;
 char out[48];size_t n=0;
 if(value->type==N32_INTEGER){uint32_t magnitude=(uint32_t)value->as.integer;if(value->as.integer<0){out[n++]='-';magnitude=0u-magnitude;}
  char reverse[10];size_t digits=0;do{reverse[digits++]=(char)('0'+magnitude%10);magnitude/=10;}while(magnitude);while(digits)out[n++]=reverse[--digits];
 }else{
  double real=n32_real(value);if(!real_finite(real))return N32_OVERFLOW;
  if(real<0){out[n++]='-';real=-real;}
  /* Exact integral binary64 values up to2^53 get integer decimal conversion,
   * avoiding normalization error in a promoted INTEGER such as2147483648. */
  if(real<=9007199254740991.0&&(double)(uint64_t)real==real){
   uint64_t whole=(uint64_t)real;char reverse[16];size_t digits=0;
   do{reverse[digits++]=(char)('0'+whole%10);whole/=10;}while(whole);
   while(digits)out[n++]=reverse[--digits];
  }else{int exponent=0;while(real>=10){real/=10;exponent++;}while(real<1){real*=10;exponent--;}
   unsigned precision=value->type==N32_SINGLE?9:17;uint8_t digits[18];
   for(unsigned i=0;i<=precision;i++){unsigned digit=(unsigned)real;if(digit>9)digit=9;digits[i]=(uint8_t)digit;real=(real-digit)*10;}
   if(digits[precision]>=5){unsigned i=precision;while(i&&digits[i-1]==9)digits[--i]=0;if(i)digits[i-1]++;else{digits[0]=1;exponent++;}}
   while(precision>1&&!digits[precision-1])precision--;
   bool scientific=exponent< -4||exponent>=(value->type==N32_SINGLE?9:17);
   if(scientific){out[n++]=(char)('0'+digits[0]);if(precision>1)out[n++]='.';for(unsigned i=1;i<precision;i++)out[n++]=(char)('0'+digits[i]);out[n++]=value->type==N32_DOUBLE?'D':'E';out[n++]=exponent<0?'-':'+';
    unsigned e=(unsigned)(exponent<0?-exponent:exponent);if(e>=100)out[n++]=(char)('0'+e/100);if(e>=10)out[n++]=(char)('0'+(e/10)%10);out[n++]=(char)('0'+e%10);
   }else{int point_at=exponent+1;if(point_at<=0){out[n++]='0';out[n++]='.';for(int i=0;i< -point_at;i++)out[n++]='0';}
    for(unsigned i=0;i<precision;i++){if(point_at>0&&(int)i==point_at)out[n++]='.';out[n++]=(char)('0'+digits[i]);}
    for(int i=(int)precision;i<point_at;i++)out[n++]='0';
   }
  }
 }
 if(n>=capacity)return N32_DOMAIN;memcpy(text,out,n);text[n]=0;*length=n;return N32_OK;
}
const char *n32_error(int status){static const char *const names[]={"OK","SN","TM","OV","/0","FC","OM","BS","UL","UNSUPPORTED","LIMIT","IO","WAIT"};return (unsigned)status<sizeof names/sizeof *names?names[status]:"ERROR";}
