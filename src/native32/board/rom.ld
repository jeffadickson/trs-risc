OUTPUT_ARCH(riscv)
ENTRY(_start)
MEMORY { ROM (rx) : ORIGIN = 0x20000000, LENGTH = 256K
         RAM (rw) : ORIGIN = 0x80000000, LENGTH = 2M }
SECTIONS {
 .text : { KEEP(*(.text.start)) *(.text .text.*) } > ROM
 .rodata : ALIGN(4) { *(.rodata .rodata.* .srodata .srodata.*) } > ROM
 .data : ALIGN(16) { __data_start = .; __global_pointer$ = . + 0x800;
   *(.data .data.* .sdata .sdata.*) __data_end = .; } > RAM AT> ROM
 __data_load = LOADADDR(.data);
 .bss (NOLOAD) : ALIGN(16) { __bss_start = .; *(.bss .bss.* .sbss .sbss.*) *(COMMON) __bss_end = .; } > RAM
 __stack_top = ORIGIN(RAM) + LENGTH(RAM);
 __stack_bottom = __stack_top - 64K;
 ASSERT(__bss_end <= __stack_bottom, "native BASIC overlaps reserved stack")
 /DISCARD/ : { *(.comment) *(.note .note.*) *(.eh_frame .eh_frame.*) }
}
