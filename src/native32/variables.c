/* Homogeneous native storage, with no16-bit emulated addresses. Scalar headers
 * have no dimension slots; an array's variable-length tail stores32-bit bounds.
 * Names retain BASIC's two significant alphanumeric characters. */
#include "native.h"
#include <stdlib.h>
#include <string.h>
struct N32Variable {struct N32Variable *next;void *cells;size_t count;char name[4];N32Type type;uint8_t rank;uint32_t bounds[];};
static size_t cell_size(N32Type type){return type==N32_DOUBLE?8:type==N32_STRING?sizeof(N32String):4;}
static N32Variable *find(N32Machine *m,const N32Reference *r){for(N32Variable *v=m->variables;v;v=v->next)if(v->type==r->type&&(!v->rank)==(!r->rank)&&!strcmp(v->name,r->name))return v;return NULL;}
void n32_variables_clear(N32Machine *m){
 N32Variable *v=m->variables;while(v){N32Variable *next=v->next;if(v->type==N32_STRING){N32String *strings=v->cells;for(size_t i=0;i<v->count;i++)free(strings[i].bytes);}free(v->cells);free(v);v=next;}m->variables=NULL;
 memset(m->defaults,N32_SINGLE,sizeof m->defaults);m->loop_count=m->return_count=0;
}
int n32_dimension(N32Machine *m,const N32Reference *r){
 if(r->rank>N32_RANK||r->type>N32_STRING)return N32_SUBSCRIPT;
 if(find(m,r))return N32_SUBSCRIPT;size_t count=1;
 for(unsigned i=0;i<r->rank;i++){if(r->indices[i]>INT32_MAX)return N32_SUBSCRIPT;size_t extent=(size_t)r->indices[i]+1;if(count>SIZE_MAX/extent)return N32_MEMORY;count*=extent;}
 size_t cell=cell_size(r->type);if(count>SIZE_MAX/cell||count*cell>16u*1024u*1024u)return N32_MEMORY;
 N32Variable *v=calloc(1,sizeof *v+r->rank*sizeof(uint32_t));if(!v)return N32_MEMORY;
 v->cells=calloc(count,cell);if(!v->cells){free(v);return N32_MEMORY;}
 memcpy(v->name,r->name,4);v->type=r->type;v->rank=r->rank;v->count=count;
 for(unsigned i=0;i<r->rank;i++)v->bounds[i]=r->indices[i];v->next=m->variables;m->variables=v;return N32_OK;
}
static int locate(N32Machine *m,const N32Reference *r,N32Variable **found,size_t *offset){
 N32Variable *v=find(m,r);
 if(!v){N32Reference bounds=*r;for(unsigned i=0;i<r->rank;i++){if(r->indices[i]>10)return N32_SUBSCRIPT;bounds.indices[i]=10;}
  int status=n32_dimension(m,&bounds);if(status)return status;v=find(m,r);
 }
 if(v->rank!=r->rank)return N32_SUBSCRIPT;
 size_t at=0;for(unsigned i=0;i<r->rank;i++){if(r->indices[i]>v->bounds[i])return N32_SUBSCRIPT;at=at*((size_t)v->bounds[i]+1)+r->indices[i];}
 *found=v;*offset=at;return N32_OK;
}
int n32_get(N32Machine *m,const N32Reference *r,N32Value *result){
 if(!r->rank&&!find(m,r)){if(r->type==N32_STRING)return n32_string(NULL,0,result);N32Value zero=n32_integer(0);return n32_convert(&zero,r->type,result);}
 N32Variable *v;size_t at;int status=locate(m,r,&v,&at);if(status)return status;
 if(r->type==N32_STRING){N32String *s=v->cells;return n32_string(s[at].bytes,s[at].length,result);}
 N32Value value={.type=r->type};memcpy(&value.as,(uint8_t *)v->cells+at*cell_size(r->type),cell_size(r->type));*result=value;return N32_OK;
}
int n32_set(N32Machine *m,const N32Reference *r,const N32Value *value){
 N32Value copy={0};int status=n32_convert(value,r->type,&copy);if(status)return status;
 N32Variable *v;size_t at;status=locate(m,r,&v,&at);
 if(!status){if(r->type==N32_STRING){N32String *s=v->cells;free(s[at].bytes);s[at]=copy.as.string;copy=n32_integer(0);}
  else memcpy((uint8_t *)v->cells+at*cell_size(r->type),&copy.as,cell_size(r->type));}
 n32_clear(&copy);return status;
}
