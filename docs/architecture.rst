Translation and machine model
=============================

Build-time decoding
-------------------

The C# decoder uses Z80 opcode fields ``x=op>>6``, ``y=(op>>3)&7``, ``z=op&7``.
It supports base, CB, ED, DD, FD, DD-CB-d and FD-CB-d families. Last DD/FD prefix
wins. ED cancels index substitution. Indexed memory operations read a signed
displacement once. ``LD H,(IX+d)`` still uses ordinary H, not IXH.

Each decoded instruction carries generated C, assembly, length and M1-fetch
count. The generated instruction updates the Z80 refresh register's low seven
bits before executing semantics. Prefix bytes count according to M1 fetches;
DD/FD-CB final operation bytes do not add a fetch.

A branch updates the original 16-bit ``pc``. A call pushes the original return
address, and a return pops it. ``translated_step`` dispatches on ``pc``. This
retains original address semantics and computed jumps while executing native
statements. It is not an idiomatic hand rewrite of the interpreter.

Memory map
----------

.. list-table:: Emulated addresses
   :header-rows: 1

   * - Range
     - Meaning in this prototype
   * - 0000h..2FFFh
     - Original 12 KB ROM, read-only
   * - 3000h..37FFh
     - Reserved/peripheral area, no writes modeled
   * - 3800h..3BFFh
     - Keyboard matrix, reads return zero
   * - 3C00h..3FFFh
     - 64-by-16 character display RAM
   * - 4000h..FFFFh
     - Writable RAM: vectors, BASIC workspace/program/variables/stack

Registers alias AF, BC, DE, HL as word/byte pairs on a little-endian machine.
Arithmetic helpers explicitly derive Z80 flags; native RISC-V condition flags
are not assumed. Carry/half-carry and signed overflow are different quantities.
P/V means parity for logic/CB operations and overflow for arithmetic.

Console ABI
-----------

Output replaces ``CALL 0033h`` with a host character write and a simulated RET.
Input replaces ``CALL 002Bh``. To distinguish blocking line input from BREAK
polls, inspect the emulated stack's return address. The ROM wrapper is:

.. code-block:: asm

   ; 0049 CD 2B 00
   CALL 0x002B
   ; 004C B7
   OR A
   ; 004D C0
   RET NZ
   ; 004E 18 F9
   JR 0x0049

A caller return address of ``004Ch`` consumes queued input; other callers get
A=0. This allows BASIC loops to poll without eating the following command.
Terminal adapters are also used in reference lockstep tests, so those tests
verify CPU translation, not the fidelity of the physical keyboard hardware.

Writable executable RAM
-----------------------

ROM initialization copies a tiny code template from ``18F7h`` to ``4080h``.
BASIC patches immediate operands to perform multi-byte floating-point
subtraction. Translating only immutable ROM would fail on calls to this routine.

.. code-block:: asm

   ; address: bytes       instruction
   ; 4080: D6 nn
   SUB n                 ; operand at 4081h is patched
   ; 4082: 6F
   LD L,A
   ; 4083: 7C
   LD A,H
   ; 4084: DE nn
   SBC A,n               ; operand at 4085h is patched
   ; 4086: 67
   LD H,A
   ; 4087: 78
   LD A,B
   ; 4088: DE nn
   SBC A,n               ; operand at 4089h is patched
   ; 408A: 47
   LD B,A
   ; 408B: 3E nn
   LD A,n                ; operand at 408Ch is patched
   ; 408D: C9
   RET

``ram_hook`` implements those known sites as explicit native statements and
checks the fixed opcode bytes. It also handles known port/indirect-jump stubs
and initialized JP/RET vectors. It does not interpret arbitrary RAM code.

Limits
------

Disk, cassette, printers, physical keyboard timing and external interrupts are
not implemented. Port reads return zero and writes do nothing. Execution is not
cycle accurate; hidden WZ-latch behavior and all undocumented flags are not fully
modeled. Arbitrary USR or POKE-installed machine-code programs fail explicitly.
The tool is suitable for ordinary console BASIC experiments, not claims of full
TRS-80 software compatibility.
