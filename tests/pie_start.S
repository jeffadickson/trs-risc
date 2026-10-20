/* Preserve the Linux entry stack as guest_main's first argument. No libc/startup
   self-relocator runs: every dynamic relocation is the emulator loader's work. */
.option norvc
.text
.global _start
.type _start,@function
_start:
    mv a0, sp
    tail guest_main
.size _start, .-_start
