RISC-V emulator in WebAssembly
==============================

The default HTML path now executes the **actual RISC-V Linux executable** inside
an emulator written in C and compiled to WASM. It is separate from the direct
C-to-WASM BASIC build. Both paths run the original ROM algorithms.

.. code-block:: text

   historical Z80 ROM -> C# translator -> generated C -> RV64IMA ELF
                                                        |
                                 C RV64 CPU -> WASM -----+
                                                        |
                                             HTML terminal

.. code-block:: sh

   make riscv-wasm
   make test-riscv-wasm
   make serve

CPU and executable subset
-------------------------

``src/rv/rv64.c`` implements RV64 integer base instructions, the M multiply/divide
extension, and A atomics for a single hart. Word operations sign-extend their
32-bit results. Division handles zero divisors and signed MIN/-1 without invoking
host C undefined behavior. LR/SC reservations and AMO word/doubleword behavior are
explicit. x0 remains zero. Data loads may be unaligned; instructions must align
to four bytes. FENCE is a no-op in this single-hart, sequential model.

There is no C compressed extension or F/D native floating point. Build the guest
with Zig ``-mcpu=generic_rv64+m+a-c-d-f -mabi=lp64``. The original BASIC floating
point routines still work: they are software algorithms implemented in Z80 and
translated into RV64 integer operations. The normal RV64GC QEMU build is a
separate artifact and is intentionally rejected by this CPU's loader.

The loader accepts little-endian ELF64 ET_EXEC and self-contained static PIE
ET_DYN for machine 243 (RISC-V).
It checks file/segment bounds, load addresses, entry alignment and ABI flags,
rejects PT_INTERP/dynamic executables, zeros guest RAM and maps PT_LOAD segments.
It constructs a Linux entry stack containing argc, argv, environment terminator
and auxiliary vector (PHDR, page size, entry, IDs and deterministic AT_RANDOM).
It applies the constrained static PIE relocations described below; it does not
resolve shared-library symbols or emulate a privileged board/MMU.

Sandboxed Linux calls
---------------------

The guest has 96 MB of bounded RAM and an 8 MB ELF upload limit. No guest address
is used as an unchecked host pointer. The syscall layer supplies terminal
read/readv/write/writev, exit, a bounded brk/anonymous mmap arena, minimal process
startup and terminal inquiries. Unsupported services return ENOSYS. There is no
host file access, thread creation, real signals or device emulation. mmap,
munmap/mprotect and signal behavior are simplified rather than a general Linux
implementation. Clock and entropy are deterministic demo shims, not secure
services or a real-time clock.

An empty stdin queue suspends an ECALL before its PC advances; supplying a key
resumes the same call. Each ``rv_run`` executes a bounded instruction batch and
yields, so an infinite BASIC loop remains interruptible by the UI's Reset.
These are emulator instruction counts, not translated-Z80 step counts.

Browser integration
-------------------

``web/riscv.mjs`` supplies host imports and reads the BASIC guest's ``mem`` symbol
from the unstripped ELF to locate its emulated Z80 memory. This keeps BASIC
knowledge outside the generic CPU core. It validates and uploads program images
at a waiting prompt, or downloads only the tokenized program area. Stripping the
guest's symbol table removes those optional image controls; retain symbols for
this demo. WASM emulator debug metadata itself can be stripped.

The HTML default uses this path; ``?engine=direct`` selects direct C-to-WASM.
The browser needs both ``rv64.wasm`` and ``trs80-rv64im.elf`` served over HTTP.
Neither artifact contains a JavaScript BASIC or Z80 implementation.

Tests
-----

``tests/rv_cpu.c`` covers zero register, sign/word extension, shift boundaries,
M edge cases, LR/SC/AMO, waiting ECALL resumption, bounds and invalid ELF/opcodes.
``tests/riscv-wasm.mjs`` executes the guest through the real WASM CPU, including
boot, the 68-byte demo image, LIST/RUN, image round trips, software math and INPUT.
It compares native CPU stdout against QEMU for the identical ELF and arguments.
The demo executes about 42.9 million RISC-V instructions; that is not a speed
claim or an exhaustive ISA-conformance result.

.. literalinclude:: ../src/rv/rv64.c
   :language: c
   :linenos:

Static PIE relocation
---------------------

The loader additionally accepts self-contained ET_DYN images with no interpreter
or shared-library dependencies. A page-aligned load bias defaults to 4 MB and can
be selected explicitly:

.. code-block:: sh

   build/rv-run --base 0x400000 build/trs80-pie
   build/rv-run --base 0xc00000 build/trs80-pie
   make test-elf-loader

The bias is added to PT_LOAD virtual addresses, entry point, dynamic-table
pointers, program-header and auxiliary-vector addresses, and heap boundaries.
ET_EXEC remains fixed-address and rejects a requested nonzero bias.

PT_DYNAMIC is parsed for DT_RELA/DT_RELASZ/DT_RELAENT. Only symbol-free
``R_RISCV_RELATIVE`` (3) and empty ``R_RISCV_NONE`` entries are supported:

.. code-block:: text

   *(load_bias + r_offset) = load_bias + signed_r_addend

Relocation tables must be mapped, correctly sized, and terminated metadata must
be present. Destination words must be aligned writable data, not text or the
relocation table itself. Signed addends, segment addresses and table extents are
checked before arithmetic can escape guest RAM. The complete table is validated
before applying any write. DT_NEEDED, PT_INTERP/TLS, REL/RELR, PLT/symbol and text
relocations fail explicitly. This does not link ET_REL object files, resolve
symbols across libraries, or implement a dynamic linker.

The browser defaults to the BASIC PIE at 4 MB. ``?base=0xc00000`` moves it to
12 MB; ``?elf=exec`` selects the old fixed-address ELF. The JS memory-image
adapter adds the load bias to the BASIC memory symbol, so saved Z80 program
images are identical at either native load address. ``rv_boot_at`` and
``rv_load_bias`` expose this to WASM hosts; ``rv_boot`` keeps default behavior.

The small ``pie-guest`` fixture is linked from C/assembly without libc or a
self-relocating CRT. It exercises genuine linker-produced data/function RELA
entries, BSS and startup pointers, proving relocation is performed by the
loader. Tests also run the complete BASIC PIE at two bases and compare saved
program images byte-for-byte.
