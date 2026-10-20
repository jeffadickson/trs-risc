ET_REL linking: proposed next stages
====================================

Status and boundary
-------------------

This is a plan, not implemented functionality. The current loader in
``src/rv/rv64.c`` accepts ET_EXEC and constrained static ET_DYN. It consumes
program headers and applies symbol-free dynamic RELA entries. ET_REL needs a
separate section-based path; loosening the existing PIE checks would not provide
object linking. Keep the current executable loader and its tests intact.

The first milestone should load one freestanding RV64IMA/LP64 object containing
``_start``, initialized data, BSS and a relocated data pointer, then run it in the
native CPU and the WASM CPU at two bases. No libc, archives, shared objects, TLS,
compressed instructions, floating-point ABI, GOT/PLT construction or relaxation
in this milestone. A later milestone may combine an explicitly supplied bounded
list of objects. Do not promise that arbitrary compiler output will link.

1. Validate and place sections
------------------------------

Add bounded ELF64 section-header parsing with checked arithmetic for table
sizes, file extents, string terminators, links and indices. Reject unsupported
extended numbering initially with an explicit diagnostic. Validate machine,
endianness, ISA/ABI flags and allocatable section types before creating a layout.
Use deterministic input/section ordering, honor power-of-two ``sh_addralign``,
copy allocatable PROGBITS and zero allocatable NOBITS. Keep executable, read-only
and writable ranges distinct; stay within the current image arena below 32 MB,
leaving the mmap arena and stack intact. Bound total input bytes and counts.

Retain an input-section-to-guest-address map and permissions. ET_REL normally
has no loadable program headers, so do not require or invent an input PHDR.
Define a freestanding entry contract: resolve executable, four-byte-aligned
``_start``, set an aligned stack and document its contents. Factor common CPU
reset/stack construction out of ``boot_elf_at`` only when this path needs it;
do not present fake Linux auxiliary-vector PHDR metadata for a section image.

2. Resolve symbols
------------------

Validate SYMTAB and its linked STRTAB, including entry sizes, name bounds,
section indices and symbol extents. Section-defined symbol addresses are the
placed section base plus ``st_value``; local and section symbols remain scoped
to their input. Support defined globals and SHN_ABS in the first milestone;
reject unresolved references and COMMON explicitly.

For multiple objects, specify deterministic resolution: reject duplicate strong
definitions, let a strong definition override a weak one, choose the first weak
definition in input order and resolve undefined weak symbols to zero. Diagnose
unresolved strong symbols with object/section/relocation context. Add COMMON
allocation and COMDAT/group selection only in a separately tested extension.
Keep archive discovery and host symbol lookup outside this loader.

3. Plan and apply relocations
-----------------------------

Read SHT_RELA using ``sh_info`` for the destination section and ``sh_link`` for
the symbol table. Validate every relocation and compute a patch plan before
writing guest memory. Check symbol indices, destination width, alignment,
signed addends, result range and instruction opcode shape. Reject conflicting
patches, metadata overlap and unknown relocation types. Instruction relocations
may patch planned executable sections during linking; PIE's rule forbidding
text relocation stays unchanged. Publish/start the linked image only on success.

Use the `RISC-V ELF psABI
<https://riscv-non-isa.github.io/riscv-elf-psabi-doc/>`_ as the relocation reference.
Record the specification revision used when implementation begins. Proposed
support order (all names below have the ``R_RISCV_`` prefix):

* First milestone: ``NONE`` and ``64`` for a checked absolute data pointer.
* Next: ``32``, ``HI20``, ``LO12_I`` and ``LO12_S`` for absolute addresses;
  ``PCREL_HI20``, ``PCREL_LO12_I`` and ``PCREL_LO12_S`` for address pairs.
  A PC-relative low relocation identifies the high relocation's label; its
  displacement must use the matching high instruction's address, not the low
  instruction's PC. Test nonadjacent pairs and multiple lows per high.
* Then: ``CALL_PLT`` (and legacy ``CALL``) for AUIPC/JALR pairs, plus ``JAL``
  and ``BRANCH``. Check signed reach and encoded alignment; do not generate
  veneers or silently truncate an out-of-range displacement.

Compile controlled fixtures with relaxation disabled, no small-data dependence,
no PIC/GOT requirement and no unwind/debug payload. Inspect actual relocation
records with readelf rather than assuming compiler flags guarantee a subset.
Reject ``RELAX`` and ``ALIGN`` until their handling is explicitly designed;
accepting a marker must not accidentally promise instruction relaxation or
section shrinking. Defer GOT, TLS, compressed, arithmetic/debug relocations and
all other types with clear errors.

4. Fixtures and acceptance
--------------------------

Use C/assembly fixtures built by Make with the pinned toolchain; any new project
fixture/inspection utility belongs in C#, not Python. Start with a tiny assembly
object using a local data-pointer relocation and ECALL write/exit. Add BSS and
alignment cases, then two objects with cross-object data and function references.
A conventional linker-produced executable run under QEMU supplies an independent
behavior oracle; the runtime-linking path must consume the original objects.

Test native and WASM execution at 4 MB and 12 MB, exact output, relocated words,
zeroed BSS and entry/stack alignment. Add negative cases for truncated tables,
invalid indices/strings, huge alignment/size, bad symbol definitions, unresolved
names, duplicate strong symbols, unsupported flags/types, overflow, wrong
instruction encodings, broken PC-relative pairs and overlapping patches. Verify
failure applies no partial relocation plan and that existing ET_EXEC/PIE tests
still pass. Include positive/negative range boundaries for every instruction
relocation and fixtures that change layout between runs.

Expose the native path and tests before changing the WASM upload ABI. Multiple
objects require an explicit bounded upload protocol, not concatenation disguised
as ELF. If BASIC image controls are later supported, expose the resolved ``mem``
symbol through the linker result; the browser's current ELF-symbol-plus-PIE-bias
lookup is insufficient. Document the supported subset, entry contract and all
rejected features in this manual and CI before advertising ET_REL support.
