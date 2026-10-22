ROM-style arithmetic on RV32 integer registers
==============================================

The RV32 builds now provide their own floating arithmetic implementation in
``src/native32/wordfloat``. They do not depend on the compiler's generic
floating-point helpers and do not require F or D hardware instructions.
The operation is still software floating point, just as it was in Level II:
sign/exponent/significand arithmetic implemented with integer instructions.

``wordfloat.c`` separates IEEE values into these parts, aligns significands,
performs arithmetic and normalizes and rounds the result. SINGLE uses a 24-bit
significand; DOUBLE uses 53 bits in a pair of RV32 words. The shared engine uses
integer C operations, with no float expressions or host math library calls.
The original ROM used byte carry/borrow chains and shifts. The native engine
uses word addition/subtraction, four 32-by-32 products for a full DOUBLE
product, and restoring division over word pairs. Mixed comments identify the
original algorithm families and explain the wider representation.

Three low bits retain guard, round and sticky information. One pack routine
rounds to nearest with ties to even, including gradual underflow. Right shifts
jam discarded nonzero information into a sticky bit. A 106-bit product is
formed explicitly from native word products, so no general 128-bit type or
library is needed. Signed zero, infinities and NaNs are handled; NaN payloads
are not preserved. Dynamic rounding modes and floating exception flags are
not implemented. BASIC continues to report nonfinite results as errors.
This is the project's chosen IEEE policy, not the old MBF bit layout.

``abi.c`` supplies strong compiler ABI entry points such as ``__adddf3`` and
``__mulsf3``. Those names remain visible in an ELF, but their implementations
now belong to this project and forward raw bits to the integer engine. They
are not generic compiler-rt float routines. A few integer compiler support
routines, such as 64-bit integer division, may remain; those are not floating
arithmetic. The ordinary hosted build and direct WASM build continue using
their native floating instructions. RV32 Linux tests and freestanding firmware
both use the custom engine.

Validation
----------

.. code-block:: sh

   make test-native32-wordfloat
   make test-native32-rv32 test-native32-math-rv32 test-native32-exp-rv32
   make native32-size

The differential test compares arithmetic and conversions against separate host
IEEE operations, requiring exact bits except for NaN payloads. Its deterministic
random inputs are supplemented by signed zeros, extreme normal/subnormal values,
infinities, NaNs, close exponents, cancellation and underflow cases. This is
substantial sampling, not an exhaustive proof. RV32 tests exercise the actual
ABI bridge and the BASIC/math paths, including the high-precision EXP vectors.

Size result and remaining gap
-----------------------------

With the refined DOUBLE EXP included, the generic-helper baseline measured
44,908 bytes of linked code/constants for RV32IMA and 33,362 bytes with C
instructions plus LTO. The custom integer engine measures 43,516 and 31,638
bytes respectively: reductions of 1,392 and 1,724 bytes. These measurements
exclude ELF metadata, BSS, heap and stack. Adding debug symbols for inspection
does not count them as ROM.

Thus generic floating helpers contributed overhead, but replacing them does
not make the image close to the original 12,288-byte Level II ROM. The custom
engine still has to implement floating arithmetic. The remaining image also
contains the parser, statement executor, variable/array/string storage,
number conversion, memory management and I/O adapter. The current implementation
is still a functional subset; its larger size is not evidence of feature parity.
Further size work needs measurements of these components and careful design
changes. The baseline ELF now retains symbols for that inspection; for example
``nm -S --size-sort build/native32-rom.elf`` lists function sizes. Compressed
instructions and LTO remain the smallest supplied ROM profile.
