/* Freestanding byte primitives and a bounded native heap. Heap capacity is RAM,
 * not ROM payload. No Linux libc, ROM image or translated Z80 code is linked.
 * Header/arena overlays intentionally compile with-fno-strict-aliasing. */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
void *memcpy(void *dst,const void *src,size_t n){uint8_t *d=dst;const uint8_t *s=src;for(size_t i=0;i<n;i++)d[i]=s[i];return dst;}
void *memmove(void *dst,const void *src,size_t n){uint8_t *d=dst;const uint8_t *s=src;if((uintptr_t)d<(uintptr_t)s)for(size_t i=0;i<n;i++)d[i]=s[i];else while(n){n--;d[n]=s[n];}return dst;}
void *memset(void *dst,int byte,size_t n){uint8_t *d=dst;for(size_t i=0;i<n;i++)d[i]=(uint8_t)byte;return dst;}
int memcmp(const void *a,const void *b,size_t n){const uint8_t *x=a,*y=b;for(size_t i=0;i<n;i++)if(x[i]!=y[i])return x[i]<y[i]?-1:1;return 0;}
int strcmp(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return (uint8_t)*a-(uint8_t)*b;}
size_t strlen(const char *s){size_t n=0;while(s[n])n++;return n;}
#ifndef N32_HEAP_BYTES
#define N32_HEAP_BYTES (1024u*1024u)
#endif
typedef struct Block {size_t size;struct Block *next;bool available;} Block;
#define HEADER ((sizeof(Block)+15u)&~(size_t)15u)
static _Alignas(16) uint8_t heap[N32_HEAP_BYTES];static Block *first;
void *malloc(size_t n){
 if(!first){first=(Block *)heap;first->size=sizeof heap-HEADER;first->next=NULL;first->available=true;}
 if(n>SIZE_MAX-15)return NULL;n=(n+15)&~(size_t)15;if(!n)n=16;
 for(Block *b=first;b;b=b->next)if(b->available&&b->size>=n){
  if(b->size-n>=HEADER+16){Block *tail=(Block *)((uint8_t *)b+HEADER+n);tail->size=b->size-n-HEADER;tail->next=b->next;tail->available=true;b->next=tail;b->size=n;}
  b->available=false;return (uint8_t *)b+HEADER;
 }return NULL;
}
void free(void *p){if(!p)return;Block *b=(Block *)((uint8_t *)p-HEADER);b->available=true;
 for(Block *v=first;v&&v->next;)if(v->available&&v->next->available){v->size+=HEADER+v->next->size;v->next=v->next->next;}else v=v->next;
}
void *calloc(size_t count,size_t size){if(size&&count>SIZE_MAX/size)return NULL;size_t n=count*size;void *p=malloc(n);if(p)memset(p,0,n);return p;}
void *realloc(void *p,size_t n){if(!p)return malloc(n);if(!n){free(p);return NULL;}Block *b=(Block *)((uint8_t *)p-HEADER);if(n<=b->size)return p;void *copy=malloc(n);if(!copy)return NULL;memcpy(copy,p,b->size);free(p);return copy;}
