Commented RV32 math assembly
============================

The listing below contains all 19 functions emitted from ``math.c``, including
the SINGLE and DOUBLE kernels, exponent/fraction helpers, checked dispatcher
and power entry. Every function header includes documented Z80 code excerpts
or call sites, ROM addresses, and an explanation of the native algorithm.
Nearby C source lines and comments on every RV32 instruction explain stack
frames, integer word operations, control flow, and software IEEE arithmetic.

This is actual Zig 0.14.1 compiler output for **RV32IMA, ILP32, soft float**,
compiled with ``-Oz -fno-inline -ffp-contract=off``. Inlining is disabled to
retain the teaching boundaries; this is not an address listing of the linked
baseline or compact/LTO firmware. No F/D hardware floating instructions are
used. The compiler's ``__*sf*`` and ``__*df*`` call sites use the firmware's
integer word-float ABI implementations when linked.

The Z80 snippets are excerpts already documented during algorithm recovery,
not complete ROM disassemblies or a claim of one-to-one instruction mapping.
Native IEEE helpers have no identical MBF implementation: their headers give
the corresponding Z80 caller or exponent operations and explain that boundary.
DOUBLE EXP additionally uses split ln(2) and 18 coefficients, rather than the
ROM's eight-coefficient SINGLE approximation. See :doc:`native32-math`.

Reproduce and verify
--------------------

From a Linux/WSL checkout with .NET 10 and the pinned Zig toolchain:

.. code-block:: sh

   make native32-math-listing
   make test-native32-math-listing

The C# annotator rejects unknown functions/opcodes and checks that instruction
order and function declarations are unchanged. Tests cover malformed input,
missing headers, modified instructions, and 256 seeded randomized assembly
streams (seed 20261024). Reassembling raw and commented files and comparing
their text and read-only data sections independently checks the annotations.
Source-level comments do not establish transcendental accuracy; the math and
simulator tests described in :doc:`native32-math` provide behavioral evidence.

Download the :download:`commented assembly <listings/native32-math-rv32.s>`.
On GitHub, open the `assembly source listing <https://github.com/jeffadickson/trs-risc/blob/native-rv32-rebuild/docs/listings/native32-math-rv32.s>`_
directly.

.. literalinclude:: listings/native32-math-rv32.s
   :language: asm
   :linenos:
