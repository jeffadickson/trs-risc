/* First ET_REL fixture: no CRT, linker relaxation or instruction relocations.
   AUIPC + fixed literal offsets deliberately avoid PCREL_HI/LO relocations.
   The literal pool itself contains genuine assembler-produced R_RISCV_64
   references to separately placed readonly data and BSS. */
.option norvc
.option norelax
.section .text
.balign 8
.globl _start
.type _start,@function
_start:
    auipc t0,0
    ld a1,64(t0)             /* relocated message pointer */
    ld t1,72(t0)             /* relocated BSS pointer */
    ld s0,0(t1)              /* zero-filled BSS becomes exit status */
    li t2,85
    sd t2,0(t1)              /* prove BSS is writable during execution */
    li a0,1
    li a2,18
    li a7,64
    ecall
    mv a0,s0
    li a7,93
    ecall
    .org _start+64
    .quad message
    .quad scratch
.size _start,.-_start

.section .rodata
.balign 16
.globl message
.type message,@object
message:
local_message:
    .ascii "ET_REL linking OK\n"
.size message,.-message

.section .data
.balign 8
.globl data_pointer
.type data_pointer,@object
data_pointer:
    .quad message-1          /* signed negative addend, still a valid value */
    .quad 0
    .reloc data_pointer+8,R_RISCV_64,absolute_value /* SHN_ABS is not biased */
    .quad local_message     /* assembler's local/section-symbol resolution */
.size data_pointer,.-data_pointer
.globl absolute_value
.set absolute_value,0x1234

.section .bss
.balign 8192
.globl scratch
.type scratch,@object
scratch:
    .zero 16
.size scratch,.-scratch
