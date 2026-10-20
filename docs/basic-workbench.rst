BASIC memory-image workbench
============================

``src/basic_tool.c`` is the first C version of the historical text/token tool.
It links the same translated interpreter as the runtime. The original ROM does
keyword tokenization and reverse conversion through LIST; the host driver does
file I/O and a separate conservative static syntax pass.

.. code-block:: sh

   make host
   build/basic-tool check examples/hello.bas
   build/basic-tool tokenize examples/hello.bas build/hello.basram
   build/basic-tool detokenize build/hello.basram build/hello-listed.bas
   build/basic-tool list build/hello.basram
   build/basic-tool run build/hello.basram

Numbered text is submitted as virtual keystrokes. After BASIC stores the program,
the host checks all tokenized lines. If there are errors, no image is written.
An existing output file is not overwritten by a failed check. Reverse conversion
loads the bytes and invokes the ROM's own LIST routine. LIST canonicalizes source
spacing: preserving original whitespace is not promised. The five-line demo
round-trips to identical tokenized bytes.

Raw program layout
------------------

A ``.basram`` file is exactly the program-area bytes to load at **42E9h** in this
pinned ROM configuration. It has no invented wrapper, cassette framing, checksum,
variable snapshot or relocation information. Each nonfinal node contains:

.. code-block:: text

   +0,+1  absolute address of next node, little endian
   +2,+3  BASIC line number, little endian
   +4...  ROM-tokenized statement body, ending with 00h

A final node is just ``00 00``. For ``10 PRINT 42`` followed by ``20 END``:

.. code-block:: text

   F2 42 0A 00 B2 20 34 32 00   ; at 42E9h: next=42F2h, line=10, PRINT 42
   F8 42 14 00 80 00            ; at 42F2h: next=42F8h, line=20, END
   00 00                        ; final sentinel at 42F8h

The full five-line demo uses **68 bytes**, including links, numbers and sentinel.
This program image is separate from the 12 KB interpreter ROM.

The loader validates contiguous forward links, increasing legal line numbers,
length bounds, NUL terminators and the unique sentinel before copying any data.
It requires text base ``(40A4h)=42E9h``. It sets variable/array/end boundaries
at ``40F9h``, ``40FBh``, ``40FDh`` to the end of the installed image. RUN clears
the interpreter's execution state. A different load base is not relocated.

Source restrictions
-------------------

Use printable ASCII and CR, LF or CRLF newlines, with conventional uppercase
keywords. Every nonblank line needs a number from 0 to 65529. Duplicate numbers,
bare numbered deletion lines, control characters and lines over a conservative
250-character input limit are rejected. Input lines may be out of order; BASIC
stores them in numerical order.

Checking all lines
------------------

Tokenizing a line does not prove it is syntactically valid. The original ROM can
store a bad line and discover the error only during RUN. Executing RUN also
misses untaken branches and may hang. Therefore ``src/basic_syntax.inc`` is a new
host-side structural parser over ROM-produced tokens. It never executes BASIC.
It visits every stored line, emits one error per failing statement, recovers at
a colon outside strings, and continues across later statements and lines.

.. code-block:: sh

   build/basic-tool check examples/errors.bas

.. code-block:: text

   BASIC line 10, token byte 6: expected expression
   BASIC line 20, token byte 7: FOR requires TO
   BASIC line 30, token byte 5: IF requires THEN or GOTO
   BASIC line 40, token byte 5: expected expression
   4 syntax/coverage error(s); no image written

Offsets refer to tokenized bytes, not original source columns. Covered syntax
includes expressions, assignments, FOR/NEXT, IF/THEN/ELSE, branches, ON branches,
PRINT, INPUT/LINE INPUT, READ/DATA/RESTORE, DIM, REM, DEF type ranges, DEF FN,
SET/RESET, POKE/OUT, CLEAR and common control statements. Function-call structure
is checked; types, builtin arity, numeric ranges, missing targets, runtime errors
and FOR/NEXT pairing are not proven. Disk/tape/editor syntax outside this subset
is explicitly rejected. A pass means structural syntax passed this documented
subset, not complete Level II language conformance. Exponentiation is spelled
``[`` (token D1h), not ASCII ``^``.
