/* Native RV32 BASIC rebuild. Tokens/characters stay bytes; INTEGER values,
 * line numbers, indices and loop counters are32 bits. No Z80 state exists.
 * The previous compatibility port is unavailable; this is a new implementation.
 */
#ifndef N32_NATIVE_H
#define N32_NATIVE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
enum {N32_OK,N32_SYNTAX,N32_TYPE,N32_OVERFLOW,N32_DIVZERO,N32_DOMAIN,
      N32_MEMORY,N32_SUBSCRIPT,N32_UNDEFINED,N32_UNSUPPORTED,N32_LIMIT,N32_IO,N32_WAIT};
typedef uint8_t N32Type;
enum {N32_INTEGER,N32_SINGLE,N32_DOUBLE,N32_STRING};
/* Word-pair storage avoids imposing double's8-byte alignment on every value.
 * Calculation still uses real IEEE doubles: the compiler lowers operations
 * either to its soft-float routines or to F/D instructions in an FPU build. */
typedef struct {uint32_t low,high;} N32Wide;
typedef struct {uint8_t *bytes;uint8_t length;} N32String;
typedef struct {union {int32_t integer;float single;N32Wide wide;N32String string;} as;N32Type type;} N32Value;
_Static_assert(sizeof(int32_t)==4&&sizeof(float)==4&&sizeof(double)==8,"native numeric widths");
N32Value n32_integer(int32_t value);
N32Value n32_single(float value);
N32Value n32_double(double value);
double n32_real(const N32Value *value);
int n32_to_integer(const N32Value *value,int32_t *result);
int n32_convert(const N32Value *value,N32Type type,N32Value *result);
int n32_binary(uint8_t op,const N32Value *left,const N32Value *right,N32Value *result);
int n32_math(uint8_t token,const N32Value *input,N32Value *result);
int n32_math_power(const N32Value *base,const N32Value *exponent,N32Value *result);
int n32_unary(uint8_t op,const N32Value *value,N32Value *result);
int n32_compare(const N32Value *left,const N32Value *right,int *order);
int n32_parse_number(const uint8_t *text,size_t length,size_t *used,N32Value *result);
int n32_format(const N32Value *value,char *text,size_t capacity,size_t *length);
int n32_string(const uint8_t *bytes,size_t length,N32Value *result);
int n32_copy(const N32Value *value,N32Value *result);
void n32_clear(N32Value *value);
const char *n32_error(int status);
/* Byte-sized lexical tokens. These IDs are this native interpreter's format;
 * original linked-memory .basram images must not be interpreted as these. */
enum {T_PRINT=128,T_LET,T_END,T_STOP,T_RUN,T_LIST,T_NEW,T_CLEAR,T_FOR,T_NEXT,T_TO,T_STEP,
 T_GOTO,T_GOSUB,T_RETURN,T_IF,T_THEN,T_ELSE,T_DIM,T_INPUT,T_DEFINT,T_DEFSNG,T_DEFDBL,T_DEFSTR,
 T_REM,T_AND,T_OR,T_NOT,T_ABS,T_SGN,T_INT,T_FIX,T_CINT,T_CSNG,T_CDBL,T_LEN,T_ASC,T_CHR,
 T_STR,T_VAL,T_LEFT,T_RIGHT,T_MID,T_RND,T_DATA,T_READ,T_RESTORE,T_TRON,T_TROFF,T_SQR,T_SIN,T_COS,T_TAN,T_ATN,T_EXP,T_LOG};
int n32_tokenize(const char *source,uint8_t *tokens,size_t capacity,size_t *length);
int n32_list_tokens(const uint8_t *tokens,size_t length,char *text,size_t capacity,size_t *used);
const char *n32_keyword(uint8_t token);
/* Arrays are homogeneous and packed:4-byte INTEGER/SINGLE,8-byte DOUBLE,
 * independent owned string descriptors. Shape/index calculations use size_t
 * and are checked before any multiplication or allocation. */
#define N32_RANK 8
#define N32_DEPTH 32
#define N32_LINE_LIMIT 1024
#define N32_CONTROL_LIMIT 32

typedef struct N32Variable N32Variable;
typedef struct {char name[4];uint32_t indices[N32_RANK];uint8_t rank;N32Type type;} N32Reference;
typedef struct {uint32_t number;uint8_t *tokens;size_t length;} N32Line;
typedef struct {N32Line *lines;size_t count,capacity;} N32Program;
typedef struct {size_t line,offset;} N32Cursor;
typedef struct {N32Reference variable;N32Value limit,step;N32Cursor resume;} N32Loop;
typedef int (*N32Write)(void *,const uint8_t *,size_t);
typedef int (*N32Read)(void *,uint8_t *,size_t,size_t *);
typedef struct {
 N32Program program;N32Variable *variables;N32Type defaults[26];
 N32Write write;N32Read read;void *context;
 N32Cursor cursor,data,returns[N32_CONTROL_LIMIT];N32Loop loops[N32_CONTROL_LIMIT];
 size_t return_count,loop_count,steps,budget;uint32_t error_line,random;
 bool running,trace,input_pending,input_prompted,after_cr,input_invalid;
 N32Reference input_target;N32Cursor input_resume;
 char input_line[N32_LINE_LIMIT+1];size_t input_length;
} N32Machine;
void n32_init(N32Machine *machine,N32Write write,N32Read read,void *context);
void n32_destroy(N32Machine *machine);
void n32_variables_clear(N32Machine *machine);
int n32_reference(N32Machine *machine,const uint8_t *text,size_t length,size_t *used,N32Reference *result);
int n32_get(N32Machine *machine,const N32Reference *reference,N32Value *result);
int n32_set(N32Machine *machine,const N32Reference *reference,const N32Value *value);
int n32_dimension(N32Machine *machine,const N32Reference *bounds);
int n32_expression(N32Machine *machine,const uint8_t *tokens,size_t length,size_t *used,N32Value *result);
int n32_program_set(N32Program *program,uint32_t number,const uint8_t *tokens,size_t length);
int n32_command(N32Machine *machine,const char *line);
int n32_source(N32Machine *machine,const char *text,size_t length);
int n32_start(N32Machine *machine,size_t statement_budget);
int n32_step(N32Machine *machine,size_t statements);
/* Board/direct-WASM session: submission never recursively executes RUN. A
 * cooperative statement machine lets INPUT suspend and preserves loop frames. */
int n32_feed(N32Machine *machine,uint8_t byte);
#endif
