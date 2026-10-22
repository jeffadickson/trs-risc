/* Native lexical data only. Tokens never dispatch Z80 instructions. A quoted
 * string/REM tail is copied byte-for-byte, so keywords inside it are data. */
#include "native.h"
#include <string.h>
static const char *const keywords[]={"PRINT","LET","END","STOP","RUN","LIST","NEW","CLEAR","FOR","NEXT","TO","STEP",
 "GOTO","GOSUB","RETURN","IF","THEN","ELSE","DIM","INPUT","DEFINT","DEFSNG","DEFDBL","DEFSTR",
 "REM","AND","OR","NOT","ABS","SGN","INT","FIX","CINT","CSNG","CDBL","LEN","ASC","CHR$",
 "STR$","VAL","LEFT$","RIGHT$","MID$","RND","DATA","READ","RESTORE","TRON","TROFF","SQR","SIN","COS","TAN","ATN","EXP","LOG"};
const char *n32_keyword(uint8_t token){unsigned index=(unsigned)token-128;return index<sizeof keywords/sizeof *keywords?keywords[index]:NULL;}
static uint8_t upper(uint8_t c){return c>='a'&&c<='z'?(uint8_t)(c-32):c;}
static bool alpha(uint8_t c){return upper(c)>='A'&&upper(c)<='Z';}
int n32_tokenize(const char *source,uint8_t *tokens,size_t capacity,size_t *length){
 if(!source||!tokens||!length)return N32_DOMAIN;size_t used=0,at=0;bool quote=false,comment=false;
 while(source[at]){uint8_t ch=(uint8_t)source[at];if(ch<32&&ch!='\t')return N32_SYNTAX;if(ch>126)return N32_SYNTAX;
  if(used==capacity)return N32_LIMIT;
  if(!comment&&ch=='"')quote=!quote;
  if(!quote&&!comment&&ch=='\''){tokens[used++]=T_REM;at++;comment=true;continue;}
  if(!quote&&!comment&&alpha(ch)){
   size_t end=at;while(alpha((uint8_t)source[end])||(source[end]>='0'&&source[end]<='9'))end++;
   if(source[end]=='$')end++;
   uint8_t token=0;
   for(unsigned i=0;i<sizeof keywords/sizeof *keywords;i++){
    const char *word=keywords[i];size_t n=strlen(word);if(n!=end-at)continue;bool same=true;
    for(size_t j=0;j<n;j++)if(upper((uint8_t)source[at+j])!=(uint8_t)word[j])same=false;
    if(same){token=(uint8_t)(128+i);break;}
   }
   if(token){tokens[used++]=token;at=end;if(token==T_REM||token==T_DATA)comment=true;continue;}
   while(at<end){if(used==capacity)return N32_LIMIT;tokens[used++]=upper((uint8_t)source[at++]);}continue;
  }
  tokens[used++]=ch;at++;
 }
 if(quote)return N32_SYNTAX;*length=used;return N32_OK;
}
int n32_list_tokens(const uint8_t *tokens,size_t length,char *text,size_t capacity,size_t *used){
 size_t out=0;bool quote=false,comment=false;
 for(size_t i=0;i<length;i++){
  uint8_t token=tokens[i];if(token=='"')quote=!quote;
  const char *word=!quote&&!comment?n32_keyword(token):NULL;
  if(word){size_t n=strlen(word);if(n>capacity-out)return N32_LIMIT;memcpy(text+out,word,n);out+=n;if(token==T_REM||token==T_DATA)comment=true;}
  else{if(out==capacity)return N32_LIMIT;text[out++]=(char)token;}
 }
 *used=out;return N32_OK;
}
