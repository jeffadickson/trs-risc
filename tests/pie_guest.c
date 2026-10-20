/* A real linker-produced static PIE with both data and function relocations.
   Volatile pointer objects prevent the compiler from replacing them with
   PC-relative constants and accidentally bypassing the loader being tested. */
#include <stdint.h>
static const char text[]="PIE relocation OK\n";
const char *volatile message=text;
static int twice(int value){return value*2;}
int (*volatile function_pointer)(int)=twice;
static volatile unsigned long zero_fill[8];
extern void _start(void);
static long syscall3(long number,long first,long second,long third){
    register long syscall_number __asm__("a7")=number;
    register long argument0 __asm__("a0")=first;
    register long argument1 __asm__("a1")=second;
    register long argument2 __asm__("a2")=third;
    __asm__ volatile("ecall":"+r"(argument0):"r"(syscall_number),"r"(argument1),"r"(argument2):"memory");
    return argument0;
}
__attribute__((noreturn)) void guest_main(uint64_t *stack){
    int valid=function_pointer(21)==42;
    for(unsigned index=0;index<8;index++)if(zero_fill[index])valid=0;
    uint64_t argc=*stack++,phdr=0,entry=0,count=0;
    if(argc<1||argc>2)valid=0;
    stack+=argc+1;while(*stack)stack++;stack++; /* argv NULL and envp NULL */
    while(stack[0]){if(stack[0]==3)phdr=stack[1];if(stack[0]==5)count=stack[1];if(stack[0]==9)entry=stack[1];stack+=2;}
    if(!phdr||!count||entry!=(uint64_t)(uintptr_t)_start)valid=0;
    if(valid)syscall3(64,1,(long)message,sizeof text-1);
    syscall3(93,valid?0:1,0,0);for(;;){}
}
