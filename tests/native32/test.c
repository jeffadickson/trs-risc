/* Native-design requirements. These test the newly specified32-bit semantics;
 * they do not relabel the old translator as a functional interpreter. */
#include "native.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifndef N32_CHECKPOINT
#define N32_CHECKPOINT 5
#endif
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
#define TEST(id,label,...) do{__VA_ARGS__;printf("PASS %u %s\n",(unsigned)(id),label);}while(0)
typedef struct {char bytes[16384];size_t length;const char *response;unsigned reads;} Output;
static int write_bytes(void *context,const uint8_t *bytes,size_t length){Output *o=context;CHECK(length<sizeof o->bytes-o->length);memcpy(o->bytes+o->length,bytes,length);o->length+=length;o->bytes[o->length]=0;return N32_OK;}
static int read_line(void *context,uint8_t *bytes,size_t capacity,size_t *length){Output *o=context;o->reads++;if(!o->response)return N32_WAIT;*length=strlen(o->response);CHECK(*length<=capacity);memcpy(bytes,o->response,*length);o->response=NULL;return N32_OK;}
static N32Machine machine;static Output out;
static N32Value eval(const char *text){uint8_t tokens[1024];size_t length,used;N32Value value={0};CHECK(n32_tokenize(text,tokens,sizeof tokens,&length)==N32_OK);CHECK(n32_expression(&machine,tokens,length,&used,&value)==N32_OK);CHECK(used==length);return value;}
static void value(const char *text,double expected,N32Type type){N32Value actual=eval(text);CHECK(actual.type==type);CHECK(n32_real(&actual)==expected);n32_clear(&actual);}
static void string(const char *text,const char *expected){N32Value actual=eval(text);CHECK(actual.type==N32_STRING&&actual.as.string.length==strlen(expected));CHECK(!memcmp(actual.as.string.bytes,expected,strlen(expected)));n32_clear(&actual);}
static int program(const char *text){out.length=0;out.bytes[0]=0;CHECK(n32_source(&machine,text,strlen(text))==N32_OK);CHECK(n32_start(&machine,10000)==N32_OK);int status=0;while(machine.running&&!status)status=n32_step(&machine,50);return status;}
static int failed(const char *text){uint8_t tokens[1024];size_t length,used;N32Value value={0};int status=n32_tokenize(text,tokens,sizeof tokens,&length);if(!status)status=n32_expression(&machine,tokens,length,&used,&value);n32_clear(&value);return status;}
int main(void){n32_init(&machine,write_bytes,read_line,&out);
 TEST(1,"INTEGER maximum survives",value("2147483647",2147483647.0,N32_INTEGER));
 TEST(2,"CINT accepts full negative32-bit boundary",value("CINT(-2147483648#)",-2147483648.0,N32_INTEGER));
 TEST(3,"integer overflow promotes to DOUBLE",value("2147483647+1",2147483648.0,N32_DOUBLE));
 TEST(4,"integer multiplication uses wider intermediate",value("50000*50000",2500000000.0,N32_DOUBLE));
 TEST(5,"32-bit AND keeps upper bits",value("1073741824 AND 2147483647",1073741824.0,N32_INTEGER));
 TEST(6,"32-bit OR keeps upper bits",value("1073741824 OR 1",1073741825.0,N32_INTEGER));
 TEST(7,"NOT has32 significant bits",value("NOT 0",-1,N32_INTEGER);value("NOT 2147483647",-2147483648.0,N32_INTEGER));
 TEST(8,"SINGLE is IEEE binary32",N32Value v=eval(".1!");uint32_t bits;memcpy(&bits,&v.as.single,4);CHECK(bits==0x3dcccccd));
 TEST(9,"DOUBLE is IEEE binary64",N32Value v=eval(".1#");CHECK(v.as.wide.high==0x3fb99999&&v.as.wide.low==0x9999999a));
 TEST(10,"numeric errors preserve output",N32Value a=n32_double(2147483648.0),v=n32_integer(7);CHECK(n32_convert(&a,N32_INTEGER,&v)==N32_OVERFLOW&&v.as.integer==7));
#if N32_CHECKPOINT>=2
 TEST(11,"INTEGER division retains32-bit precision",value("2147483647/1",2147483647.0,N32_DOUBLE));
 TEST(12,"divide by zero is explicit",CHECK(failed("1/0")==N32_DIVZERO));
 TEST(13,"unary minus binds below power",value("-2^2",-4,N32_INTEGER));
 TEST(14,"power retains original left associativity",value("2^3^2",64,N32_INTEGER));
 TEST(15,"comparisons return32-bit BASIC true",value("100000001#>100000000#",-1,N32_INTEGER));
 TEST(16,"INT floor differs from FIX truncation",value("INT(-1.2#)",-2,N32_INTEGER);value("FIX(-1.2#)",-1,N32_INTEGER));
 TEST(17,"negative INTEGER extreme negates without UB",value("-CINT(-2147483648#)",2147483648.0,N32_DOUBLE));
 TEST(18,"native sqrt handles integer input",value("SQR(81)",9,N32_SINGLE));
 TEST(19,"string functions and typed lengths",string("MID$(\"ABCDE\",2,3)","BCD");value("LEN(\"ABC\")",3,N32_INTEGER));
 TEST(20,"embedded-zero strings preserve lengths",N32Value v=eval("\"A\"+CHR$(0)");CHECK(v.as.string.length==2&&v.as.string.bytes[1]==0);n32_clear(&v));
#endif
#if N32_CHECKPOINT>=3
 TEST(21,"INTEGER variables store full32 bits",CHECK(n32_command(&machine,"A%=2147483647")==N32_OK);value("A%",2147483647.0,N32_INTEGER));
 TEST(22,"DOUBLE variables avoid SINGLE rounding",CHECK(n32_command(&machine,"D#=100000001#")==N32_OK);value("D#-100000000#",1,N32_DOUBLE));
 TEST(23,"typed scalar namespaces remain distinct",CHECK(n32_command(&machine,"A!=2")==N32_OK);value("A!",2,N32_SINGLE);value("A%",2147483647.0,N32_INTEGER));
 TEST(24,"DEFINT changes source-level defaults",CHECK(n32_command(&machine,"DEFINT B-C:B=40000")==N32_OK);value("B",40000,N32_INTEGER));
 TEST(25,"DEFDBL selects DOUBLE storage",CHECK(n32_command(&machine,"DEFDBL E:E=.1#")==N32_OK);N32Value v=eval("E");CHECK(v.type==N32_DOUBLE));
 TEST(26,"large array subscripts exceed16 bits",CHECK(n32_command(&machine,"DIM X%(65536):X%(65536)=123456789")==N32_OK);value("X%(65536)",123456789.0,N32_INTEGER));
 TEST(27,"DOUBLE arrays preserve DOUBLE values",CHECK(n32_command(&machine,"DIM Y#(2):Y#(1)=100000001#")==N32_OK);value("Y#(1)",100000001.0,N32_DOUBLE));
 TEST(28,"array rank mismatch is rejected",CHECK(failed("X%(1,1)")==N32_SUBSCRIPT));
 TEST(29,"negative subscript is rejected",CHECK(failed("X%(-1)")==N32_SUBSCRIPT));
 TEST(30,"oversized allocation fails before allocation",CHECK(n32_command(&machine,"DIM Z%(2147483647)")==N32_MEMORY));
#endif
#if N32_CHECKPOINT>=4
 TEST(31,"ordinary BASIC square demo runs",CHECK(program("10 FOR I=1 TO 5\n20 PRINT I;I*I\n30 NEXT I\n")==N32_OK);CHECK(!strcmp(out.bytes," 1  1 \n 2  4 \n 3  9 \n 4  16 \n 5  25 \n")));
 TEST(32,"INTEGER loops exceed16-bit values",CHECK(program("10 FOR I%=40000 TO 40002\n20 PRINT I%;\n30 NEXT I%\n")==N32_OK);CHECK(!strcmp(out.bytes," 40000  40001  40002 ")));
 TEST(33,"DOUBLE FOR is a deliberate native extension",CHECK(program("10 FOR D#=1 TO 2\n20 PRINT D#;\n30 NEXT D#\n")==N32_OK);CHECK(!strcmp(out.bytes," 1  2 ")));
 TEST(34,"large line numbers branch",CHECK(program("10 GOTO 100000\n100000 PRINT 9\n")==N32_OK);CHECK(!strcmp(out.bytes," 9 \n")));
 TEST(35,"GOSUB and RETURN use native cursors",CHECK(program("10 GOSUB 100:PRINT 2:END\n100 PRINT 1:RETURN\n")==N32_OK);CHECK(!strcmp(out.bytes," 1 \n 2 \n")));
 TEST(36,"IF and ELSE choose branches",CHECK(program("10 IF 0 THEN PRINT 1 ELSE PRINT 2\n")==N32_OK);CHECK(!strcmp(out.bytes," 2 \n")));
 TEST(37,"DATA READ retain destination precision",CHECK(program("10 DATA 2147483647,.1\n20 READ A%,D#\n30 PRINT A%;D#-.1#\n")==N32_OK);CHECK(!strcmp(out.bytes," 2147483647  0 \n")));
 TEST(38,"INPUT suspends then resumes existing RUN",CHECK(program("10 INPUT A%\n20 PRINT A%\n")==N32_WAIT);CHECK(machine.running);out.response="2147483647";CHECK(n32_step(&machine,100)==N32_OK&&!machine.running);CHECK(!strcmp(out.bytes,"?  2147483647 \n")));
 TEST(39,"execution budget bounds infinite loops",CHECK(n32_source(&machine,"10 GOTO 10\n",11)==N32_OK);CHECK(n32_start(&machine,20)==N32_OK);CHECK(n32_step(&machine,100)==N32_LIMIT));
 TEST(40,"syntax failure preserves existing program",size_t before=machine.program.count;CHECK(n32_source(&machine,"THIS IS NOT NUMBERED",20)==N32_SYNTAX);CHECK(machine.program.count==before));
#endif
#if N32_CHECKPOINT>=5
 TEST(41,"floating overflow is explicit",CHECK(failed("3.4E38*2!")==N32_OVERFLOW));
 TEST(42,"IEEE subnormal SINGLE survives",N32Value v=eval("1E-40");CHECK(v.type==N32_SINGLE&&v.as.single>0&&v.as.single<1.17549435e-38f));
 TEST(43,"large INT retains floating storage",N32Value v=eval("INT(1D20)");CHECK(v.type==N32_DOUBLE&&n32_real(&v)==1e20));
 TEST(44,"unsigned-looking integer overflow is checked",CHECK(failed("CINT(4294967295#)")==N32_OVERFLOW));
 TEST(45,"ported sine handles zero",value("SIN(0)",0,N32_SINGLE));
 TEST(46,"integer format avoids signed minimum overflow",char text[48];size_t length;N32Value v=n32_integer(INT32_MIN);CHECK(n32_format(&v,text,sizeof text,&length)==N32_OK&&!strcmp(text,"-2147483648")));
 TEST(47,"promoted integer prints exactly",char text[48];size_t length;N32Value v=n32_double(2147483648.0);CHECK(n32_format(&v,text,sizeof text,&length)==N32_OK&&!strcmp(text,"2147483648")));
 TEST(48,"NEW frees arrays and typed state",CHECK(n32_command(&machine,"NEW")==N32_OK);CHECK(machine.program.count==0&&machine.variables==NULL));
 TEST(49,"token streams contain one-byte keyword IDs",uint8_t text[32];size_t length;CHECK(n32_tokenize("PRINT 1",text,sizeof text,&length)==N32_OK);CHECK(text[0]==T_PRINT&&length==3));
 TEST(50,"bounded recursive syntax rejects stack abuse",char source[120];memset(source,'(',50);source[50]='1';memset(source+51,')',50);source[101]=0;CHECK(failed(source)==N32_LIMIT));
#endif
 n32_destroy(&machine);return 0;
}
