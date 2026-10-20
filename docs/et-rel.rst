Single-object ET_REL linking
============================

The emulator now has a first-stage **freestanding, single-object** ELF64 ET_REL
linker. It places sections and resolves symbols before starting the CPU. This is
separate from the existing ET_EXEC program-header loader and static PIE's
symbol-free dynamic relocator. It does not yet combine objects or accept
arbitrary C compiler output.

.. code-block:: sh

   make bootstrap-riscv
   make test-et-rel-wasm
   build/rv-run --base 0x400000 build/rel-guest.o
   build/rv-run --base 0xc00000 build/rel-guest.o

The fixture prints ``ET_REL linking OK``. Its input is the original assembler
object, not an executable secretly linked by the build. A separate conventional
linker/QEMU executable is an independent output oracle for the same object.

Accepted object contract
------------------------

Use little-endian ELF64, RISC-V machine 243, version 1, flags zero, RV64IMA/LP64
code, no program headers and no preassigned entry/section virtual addresses.
The ELF header and section entries must be the normal 64-byte forms. Extended
section numbering is rejected. Flags and table structure are checked; this is
not a full ISA-attribute verifier. Unsupported instruction bytes still fault
when decoded by the CPU.

Allocatable sections must be ordinary PROGBITS or NOBITS with only the WRITE,
ALLOC and EXECINSTR flags. Writable executable sections and executable BSS are
rejected. Nonallocatable PROGBITS, string/symbol tables, RELA tables and RISC-V
attributes are accepted as metadata; RELA may carry SHF_INFO_LINK. Relocations
targeting debug/nonallocatable sections are rejected, so build fixtures with
``-g0 -mno-relax`` and use ``.option norvc`` and ``.option norelax`` in assembly.
Groups, merge/string sections, compressed sections, TLS and dynamic metadata are
outside this subset.

Sections are placed in input section-table order, with each nonempty allocated
section beginning on a separate page and honoring any larger power-of-two
alignment. Default base is 4 MB; an explicit base must be nonzero, page aligned
and below 32 MB. Zero selects the default, as with PIE. The whole layout must fit
below 32 MB, preserving the emulator's mmap arena and stack. NOBITS and padding
start zero. No input program-header table is manufactured.

Section flags describe distinct executable, readonly and writable extents used
by loader validation. The CPU still has no MMU or runtime page protection.

Symbols and entry
-----------------

Exactly one SYMTAB with a linked STRTAB is required. The linker validates names,
local/global partitioning, symbol types/visibility, section indices and extents.
Defined local and global NOTYPE/OBJECT/FUNC/SECTION/FILE symbols are supported,
including section-relative values and SHN_ABS. Nonallocatable symbols can be
validated but cannot supply runtime relocation values. Duplicate global names
are rejected. Undefined symbols other than the mandatory null symbol, COMMON,
weak symbols and nondefault visibility are rejected even if unreferenced.

The entry must be a defined global ``_start`` FUNC symbol, with at least four
bytes inside an executable section and a four-byte-aligned address. Registers
start zero except for the 16-byte-aligned stack pointer. The initial stack is:

.. code-block:: text

   argc, argv[0..argc-1], NULL, envp NULL,
   AT_PAGESZ, 4096, AT_ENTRY, resolved _start, AT_NULL, 0

Arguments are NUL-terminated strings in guest RAM. This freestanding contract
provides no PHDR, libc startup, TLS, constructors or shared-library initialization.
The existing bounded Linux syscall shims are available to the fixture's ECALLs.
ET_EXEC/PIE retain their previous fuller startup metadata, including AT_PHDR and
the deterministic demo AT_RANDOM bytes.

Checked relocations and limits
------------------------------

Only SHT_RELA is supported. ``sh_link`` must identify the object's SYMTAB and
``sh_info`` must identify an allocated destination section. The implemented
relocation formulas follow the `RISC-V ELF psABI, version 2.0-rc0,
October 14, 2026 <https://riscv-non-isa.github.io/riscv-elf-psabi-doc/>`_:

* ``R_RISCV_NONE``: no write. This subset requires zero symbol/addend and an
  offset no greater than the destination section's size.
* ``R_RISCV_64``: write ``S + signed A`` to an aligned eight-byte field. ``S`` is
  the resolved symbol value; symbol zero means zero, as used by the assembler
  for folded absolute constants. SHN_ABS values are not biased. The full unsigned
  64-bit result range is permitted, with underflow/overflow rejected. This is
  not a guarantee that the value is a valid guest pointer.

Eight-byte literal pools inside executable sections are allowed; instruction
bitfield relocations are not implemented. Overlapping relocation writes fail.
All sections, symbols, tables and patches are validated before any section is
copied or any patch is written. A bad final record cannot install earlier valid
patches. Failure resets the previous guest and leaves it faulted; it does not
preserve a previously running image. File metadata stays outside guest RAM.

The existing 8 MB upload and 96 MB RAM bounds remain. Additional fixed caps are
128 section headers (including null), 512 symbols (including null), 4,096 total
relocation records, and 255 bytes per name. Alignment is bounded by the 32 MB
image arena. These deliberately small limits bound native and WASM parsing work.

Rejected relocation families include PC-relative/absolute instruction pairs,
calls/branches, RELAX/ALIGN, GOT/PLT, TLS, compressed and debug arithmetic types.
There is no archive search, external host-symbol lookup, shared-library linking,
veneer generation or instruction relaxation. Multiple objects remain a later
stage described in :doc:`et-rel-plan`.

Validation and WASM use
-----------------------

``tests/rel_guest.S`` uses fixed-offset PC-relative literal loads, genuine
assembler-produced data RELA records, local/global symbols, signed addends,
absolute constants, and separately aligned BSS. It reads initially zero BSS,
writes it, prints through ECALL and exits with the initial BSS value.

``tests/et_rel.c`` checks resolved words, changed section alignment, flags,
startup arguments/stack, BSS, full-width values and rejection before publication.
It corrupts header/section bounds, names, symbol definitions, links, relocations,
resource counts and signed arithmetic. ``tests/et-rel.mjs`` compares native
execution at two bases against the ordinary linker/QEMU oracle, and executes
and resets the original object through the WASM CPU at both bases. Malformed
objects are also rejected by the compiled WASM loader. CI runs these checks.

The existing upload and ``rv_boot_at`` ABI accepts this single object without a
new multi-file protocol. The generic JavaScript host can run the fixture, but
the BASIC page continues to select its existing executable/PIE artifacts.
ET_REL BASIC image controls are not supported: section-relative symbols cannot
use the page's executable-symbol-plus-load-bias lookup. No BASIC port to ET_REL
or general browser object-linking UI is claimed.

.. literalinclude:: ../src/rv/et_rel.inc
   :language: c
   :linenos:
