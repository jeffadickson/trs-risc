Decoding Level II math for RV32
===============================

The native interpreter now ports the **algorithms**, rather than simulating
Z80 arithmetic instructions or substituting a different math library. The
reference is the pinned 12 KB Model I Level II ROM, SHA-256
``3cd3824d2ac2743750d2179951ed812794302b885ccb4f98aa2579b352c4b00e``.
The old translator is used only as a test oracle. It is not linked into the
functional interpreter or its firmware.

Recovered entry points
----------------------

=========== ========== ======================================================
Function    ROM entry  Functional algorithm
=========== ========== ======================================================
LOG         0809h      Normalize mantissa; rational transform; odd polynomial
EXP         1439h      Binary exponent reduction; polynomial; power-of-two scale
SQR         13E7h      Power by one half through LOG and EXP
COS         1541h      Add pi/2, then use SIN
SIN         1547h      Reduce to turns; fold range; odd polynomial
TAN         15A8h      Compute SIN and COS, then divide
ATN         15BDh      Sign and reciprocal reduction; odd polynomial
Horner      14A9h      Multiply accumulated polynomial by x, add coefficient
Odd series  149Ah      Evaluate x times a polynomial in x squared
=========== ========== ======================================================

``math_kernel.inc`` contains mixed Z80 snippets, addresses and functional C
beside each of these algorithms. ``math.c`` contains native word-based exponent
and fraction operations, type dispatch and checked result publication. The
kernel is instantiated for float and double, so SINGLE arithmetic rounds at
float operation boundaries. Fused multiply-add contraction is disabled.
There are no host libm calls in the production core.

LOG decoding
------------

The ROM changes its accumulator exponent to 80h and keeps the old exponent
in the CPU stack. In functional terms, write a positive input as
``x = m * 2**k``, with ``0.5 <= m < 1``. Its sequence of adds, divides and
subtractions is then:

.. code-block:: text

   z = 1 - sqrt(2) / (m + sqrt(0.5))
   p = z * ((c0*z*z + c1)*z*z + c2)
   result = (k - 0.5 + p) * ln(2)

The constants are the ROM's stored approximations, not newly evaluated ideal
constants. The native implementation extracts the IEEE exponent with a word
mask and a shift, and sets the mantissa exponent with another mask. It scales
subnormal inputs first. The exponent is a native integer, so it can represent
binary64's -1074..1024 range rather than the original MBF exponent byte.

For example, Z80 ``LD A,(4124h); SUB 80h; LD (HL),B`` manipulates the single
accumulator's exponent byte. A native SINGLE normalization reads one 32-bit
word, extracts ``(bits >> 23) & 255``, and builds the normalized value with
``(bits & 0x7fffff) | 0x3f000000``. It operates on the real IEEE word layout;
it does not retain the 4121h..4124h workspace or synthesize Z80 flags.
DOUBLE storage uses two native words, because a 64-bit value does not fit in
one RV32 register.

EXP and SQR decoding
--------------------

The ROM first multiplies by its approximation to log2(e), floors that result,
and chooses ``k = floor(x*log2(e)) + 1``. It then constructs
``r = k*ln(2) - x``. For SINGLE, an eight-coefficient Horner polynomial approximates
``exp(-r)``; scaling by ``2**k`` restores the result. The native version keeps
``k`` in a 32-bit integer and scales across the full supported IEEE range,
including subnormal outputs. It checks broad bounds before converting an
exponent to an integer and checks nonfinite results before publishing them.

SQR no longer uses the replacement Newton algorithm. The ROM loads the
constant one half at 1380h and enters power evaluation. The native version
follows that same LOG/multiply/EXP structure. Negative roots report FC, and
zero is handled before LOG. Fractional powers now use LOG and EXP too; the
native integer-power fast path retains exponentiation by squaring. Negative
bases require integral exponents. Zero to a negative power reports division
by zero.

Trigonometric reduction
-----------------------

SIN divides by the ROM's 2*pi approximation, subtracts the floor, and folds
that fractional turn around quarter and half turns. The subtraction helper
at 0710h means **table value minus accumulator**: the fold begins with
``0.25 - fraction``. Reading that call backwards produces a plausible but
incorrect implementation. Direct accumulator comparisons caught this during
the port. The five-coefficient odd polynomial is then evaluated in native
register arithmetic.

COS adds the ROM's pi/2 approximation and calls the SIN algorithm. TAN divides
its independently computed sine and cosine. ATN takes the magnitude, maps
values at least one to their reciprocal, evaluates a nine-coefficient odd
polynomial, and reverses the reciprocal transform with pi/2 minus the result.
It restores the argument's sign last.

Coefficient storage and precision
---------------------------------

The ROM tables are MBF SINGLE values, stored low mantissa byte first, sign in
bit 7 of the third byte, and exponent in the fourth. A nonzero constant is:

.. code-block:: text

   sign * (1 + (mantissa & 0x7fffff) / 2**23) * 2**(exponent - 129)

The port expresses those exact numbers as hexadecimal C floating constants.
There is no runtime MBF decoder or byte-wise coefficient arithmetic. Tables
at 07FCh, 1479h, 1593h and 15E3h contain respectively 3, 8, 5 and 9
coefficients. The first byte is a **count**, not a degree. Thus the native
SINGLE tables occupy 12, 32, 20 and 36 bytes respectively. They are polynomial
coefficients, not a large input/output lookup table.

This is an algorithm port with a deliberate IEEE numeric policy. It is **not
bit-exact MBF compatibility**: IEEE intermediate rounding and exponent ranges
change some answers. DOUBLE EXP has been refined as described below. The other DOUBLE functions
still use inherited coefficients with roughly SINGLE approximation quality.
Widening their storage does not create 53-bit transcendental accuracy; refining
those remaining functions is future work.
Large angle range reduction also loses information once the input format can
no longer resolve fractional turns. No claim of correctly rounded math is made.

Validation and reproducibility
------------------------------

.. code-block:: sh

   make test-native32-math
   make test-native32-math-rv32
   make test-native32-math-rom
   make test-native32-wasm

The ROM test boots the historical interpreter to install its arithmetic RAM
stubs, snapshots memory, and invokes each recovered routine directly with
exact MBF SINGLE inputs. It decodes the accumulator into a double for comparison;
BASIC's printed decimal rounding is not used as evidence. Seven functions are
sampled at 193 points each. LOG and SQR sample positive inputs; the other
functions sample -6..6 in increments of 1/16. Error is measured as
``abs(native-ROM) / max(1, abs(ROM))``, with a limit of 0.00002. The test prints
exact-match counts and observed maximum errors; passing that limit does not
establish bit equality or full-domain accuracy.

The standalone native math tests also check exact decoded ROM reference
vectors, domain errors, output preservation on failure, fractional powers and
IEEE ranges outside MBF, including DOUBLE's smallest subnormal input. They run
as real RV32 code under QEMU. WASM integration exercises SIN, LOG, SQR and
fractional power through the actual BASIC parser. Both firmware profiles boot in the browser RV32 board simulator and run wide
loops, DOUBLE EXP and suspended INPUT in integration tests. Physical-board boot
is not yet verified.

Observed comparison run
-----------------------

On the current host run, **1,092 of 1,351 outputs matched the ROM exactly**.
The largest scaled difference was 0.00000976087157, in TAN. Maximum scaled
errors by function were LOG 1.4243e-7, EXP 1.1921e-7, SQR 2.1597e-7,
SIN 2.3842e-7, COS 3.7998e-7 and ATN 1.1509e-7. TAN magnifies small cosine
rounding changes near its poles. These figures describe the specified sample
grid only, not an accuracy guarantee for all representable inputs.

Refined DOUBLE EXP
------------------

DOUBLE EXP preserves the ROM's exponent reduction, Horner polynomial and
power-of-two scaling structure, with three numerical refinements:

* Reduction uses a full binary64 log2(e), and ln(2) split into high and low
  parts. It computes ``(k*ln2_hi - x) + k*ln2_lo`` to avoid cancellation loss
  from a single rounded ln(2), especially when x is large.
* The polynomial for exp(-r) has 18 coefficients (degree 17), replacing the
  eight ROM coefficients for this DOUBLE path. They are exact-rounded
  alternating reciprocal factorials. The Taylor remainder over the reduced
  interval is less than 3e-19. This is a bounded Taylor construction, not a
  claim of a minimax fit or universally correctly rounded EXP.
* Scaling constructs exact IEEE powers of two and rounds into the subnormal
  range only on the final multiplication. It avoids repeated subnormal
  halving, which can introduce double rounding.

The DOUBLE coefficient table occupies 144 bytes, versus the previous 64-byte
DOUBLE copy of the eight ROM coefficients. SINGLE EXP retains the old table
and behavior. DOUBLE SQR and fractional power benefit from the refined EXP,
but their inherited LOG approximation still limits their overall accuracy.
No input/output lookup table was added.

``ExpAccuracy.cs`` generates the coefficients using exact BigInteger rational
rounding. It independently generates expected EXP bits with 400 decimal places
of integer fixed-point arithmetic, a Taylor series at x/1024 and ten squarings.
It does not use host Math.Exp or the implementation under test. Inputs are
exact binary64 values; expected results are rounded once to binary64. The
checked-in vectors cover a broad -746..710 grid and adjacent representable
values near reduction, overflow and subnormal boundaries.

.. code-block:: sh

   make regenerate-native32-exp
   make test-native32-exp test-native32-exp-rv32

All 4,725 vectors passed: 4,693 finite results and 32 overflow cases. The largest
observed difference was **two ULPs**, both on the host and real RV32 under QEMU,
including the custom integer arithmetic engine. The regression limit is four
ULPs. This sampled result is not a full-domain error proof. A ULP is one spacing
between neighboring representable values at the result's magnitude.

The higher-accuracy EXP alone changed linked size from 44,780 to 44,908 bytes
for RV32IMA and 33,218 to 33,362 bytes for compressed instructions plus LTO.
The subsequent custom arithmetic replacement reduced those totals; see
:doc:`native32-wordfloat`. The README description and this document describe
both changes explicitly.
