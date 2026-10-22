Native32 functional rebuild
===========================

This branch starts from main commit
``d2f89e066759c94717068f45761137aec17994c0``. It does not recover the lost,
unpushed functional interpreter. ``src/native32`` is a new interpreter; the
historical ROM translator and RV64 emulator remain separate reference paths.
No Z80 CPU state or emulated 64 KB address space participates in this core.

Build and run
-------------

.. code-block:: sh

   make test-native32
   build/native32 console
   build/native32 run examples/hello.bas
   make test-native32-rv32
   make test-native32-wasm
   make native32-rom
   make docs-native32

The Makefile bootstraps Zig and QEMU through the existing tool installer.
The native host build needs a C compiler; WASM tests need Node. Open
``web/native32.html`` through a static HTTP server rooted at ``web`` after
building WASM. The existing C# server is available with ``make serve-native32``. The page is a direct C-to-WASM implementation, not an RV32 emulator.
Actual RV32 instructions are exercised under QEMU by the RV32 test target.
The existing RV64 simulator cannot execute the new RV32 image.

Architecture and numeric contract
---------------------------------

``number.c`` implements checked typed operations; ``token.c`` handles lexical
source; ``expression.c`` parses expressions; ``variables.c`` owns scalar and
packed array storage; ``program.c`` owns lines, execution cursors and control
stacks. CLI, WASM and UART adapters use the same callback-driven core.
Statement budgets and a WAIT result let INPUT suspend without replaying RUN.

* INTEGER is signed 32 bits, including logical operators and loop counters.
  Overflow promotes to DOUBLE. INTEGER division produces DOUBLE. CINT truncates
  toward zero and checks range before a C conversion.
* Unsuffixed variables still default to SINGLE. Use ``DEFINT`` or ``%`` to retain
  all 32 integer bits. SINGLE is IEEE binary32 with 24 significand bits; DOUBLE
  is IEEE binary64 with 53 bits, versus the original MBF DOUBLE's 56 bits.
* SINGLE operations round through float; DOUBLE operates through double.
  DOUBLE storage uses two words to avoid unnecessary alignment padding in
  tagged values. Packed arrays use 4-byte INTEGER/SINGLE and 8-byte DOUBLE
  cells. Strings use byte data and owned descriptors, not 32-bit characters.
* Indices, bounds and line numbers use native widths. Array index 65536 and
  loops above 32767 are tested. Rank is limited to eight, control stacks to
  32, strings to 255 bytes, source lines to 1024 bytes and an array to 16 MiB.
  Firmware allocation is additionally bounded by its 1 MiB heap.
* Nonfinite results produce an overflow error; subnormal values are allowed.
  Decimal parsing and 9/17 significant digit printing are experimental, not
  guaranteed correctly rounded or exact round trips for every IEEE value.
* SQR uses a new Newton iteration. RND uses xorshift32. These are deliberate
  new algorithms, not claims of original Level II numeric output compatibility.

This embraces RV32 for values, indices and arithmetic rather than translating
8-bit register operations. Soft-float still requires compiler support routines
for floating arithmetic on the chosen RV32IMA processor. There is no hardware
FPU profile yet. Integer-only power is supported; fractional exponents are not.

Implemented subset and gaps
---------------------------

Implemented: numbered source editing, RUN/LIST/NEW/CLEAR, assignments, numeric
and string expressions, comparisons and logical operators, PRINT, single-target
INPUT, IF/THEN/ELSE, GOTO/GOSUB/RETURN, typed FOR/NEXT, DIM, typed declarations,
DATA/READ/RESTORE, tracing, ABS/SGN/INT/FIX, conversions, LEN/ASC/CHR$, STR$/VAL,
LEFT$/RIGHT$/MID$, SQR and RND.

Missing: SIN/COS/TAN/ATN/EXP/LOG (recognized and explicitly UNSUPPORTED), original
math algorithms, fractional powers, nested IF, multi-variable NEXT and INPUT,
CONT, list ranges, PRINT zones/formatting compatibility, user-defined functions,
string assignment forms such as MID$ statements, machine-memory and hardware
functions, cassette/file operations, graphics, full error compatibility and
original binary program image loading. DATA consumes its line's remainder;
commands after DATA on that line are not supported. Keyword recognition requires
word boundaries; it does not reproduce all original no-space syntax. STOP ends
execution; it does not retain a resumable continuation. This list describes a
subset, not a full Level II replacement or recovered feature count.

Source is tokenized to new byte IDs. It is not an original ROM memory image.
Load numbered text with the CLI or browser editor; old ``.basram`` files belong
to the historical translator. Source replacement is transactional. An invalid
source import preserves the previous program. Overlong interactive lines are
rejected rather than executing a truncated prefix.

ROM measurement and instruction density
---------------------------------------

The original Model I Level II ROM is 12,288 bytes. This rebuild has fewer
features, so its size is not a feature-equivalent final comparison.
``native32-rom.elf`` is a freestanding RV32IMA soft-float image;
``native32-rom-compact.elf`` enables compressed instructions and LTO. The board
linker places code at 0x20000000 and RAM at 0x80000000. It initializes data/BSS
and uses a 16550 UART at 0x10000000. This is a buildable board contract, not a
verified boot on a specific physical board. The Linux CLI binary includes
libc/startup and must not be reported as ROM size. Heap, stack and BSS consume
RAM; ELF metadata is not ROM payload.

.. include:: _generated/native32-size.rst

The Z80 can encode ``INC A`` as one byte (3C), ``ADD A,B`` as one byte (80), and
``LD A,(HL)`` as one byte (7E). Their RV32I counterparts typically occupy four
bytes: ``addi a0,a0,1``, ``add a0,a0,a1`` and ``lbu a0,0(a1)``. Compressed
``c.addi`` and ``c.add`` can take two bytes when register and immediate
constraints fit. The base compressed set has no equivalent compressed byte
load. RV32's larger registers can process a 32-bit quantity in one arithmetic
instruction where Z80 needs multiple byte operations; instruction byte density
and work per instruction are different comparisons. A wider datapath alone
does not guarantee a smaller interpreter.

Native source comments explain policies and ownership, but do not contain a
verified Z80 listing for every routine. The new algorithms do not map one-to-one
to ROM entry points. Original source references remain in the historical
pipeline documentation; deriving a verified per-function correspondence and
original math port remains future work.

Validation
----------

The C suite has 50 numbered native-design checks in five groups of ten. It
covers integer edges, IEEE bit patterns, strings, typed array bounds, wide loop
counters, transactional loading and cooperative INPUT. These are requirements
for this implementation, not a claim of 50 recovered Level II routines.
Host, real RV32-under-QEMU and direct WASM share the core. The WASM integration
test runs an array at index 65536, a wide loop and suspended INPUT, and rejects
an oversized editor line. Firmware targets are link checked only.
