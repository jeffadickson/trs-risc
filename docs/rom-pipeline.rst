Where BASIC comes from
======================

There is no separate C implementation of the BASIC language here. The interpreter
is the original 12,288-byte Model I Level II ROM image.

1. ``BuildTools.FetchRom`` downloads the image from a pinned revision of
   ``lkesteloot/trs80``. It verifies length and SHA-256 **before writing it**.
2. ``Translator.Decode`` reads the Z80 instructions at build time and produces
   native C operations for every possible ROM byte-offset entry point.
3. ``Translator.Generate`` also emits ``static const uint8_t rom_image[]`` with
   all original bytes. BASIC still reads keyword spellings, error messages,
   floating-point constants and other tables using Z80 addresses.
4. The C compiler includes ``build/translated.inc`` into ``src/runtime.c`` or
   ``src/basic_tool.c`` and emits a host or RV64 Linux executable.
5. At startup, ``memcpy`` copies ``rom_image`` into emulated addresses
   ``0000h..2FFFh``. Original PC zero starts the native-translated reset sequence.
6. The original BASIC tokenizer reads submitted numbered lines and stores
   tokenized BASIC in RAM. ``RUN`` executes those tokenized lines through the
   original interpreter's translated routines. No C# runtime is needed by the
   resulting native executable.

Pinned source
-------------

.. code-block:: text

   https://raw.githubusercontent.com/lkesteloot/trs80/
     b8ea902112de747764adcd4483eb48044f785bb7/
     packages/trs80-emulator/roms/model1-level2.rom

   bytes:   12288
   SHA-256: 3cd3824d2ac2743750d2179951ed812794302b885ccb4f98aa2579b352c4b00e

The authoritative URL and digest live in ``BuildTools.cs``. A matching image can
also be supplied locally. The fetcher preserves an existing file if download
validation fails. The historical ROM has third-party rights; the project's MIT
license covers new tools/runtime code, not the ROM or derived translations.
The repository/source archive excludes the ROM and generated translations.

Why retain bytes and native code?
---------------------------------

Original ROM bytes are needed as **data**, even though the executable ROM
instructions have been translated. For example, the keyword table at ``1650h``
starts with:

.. code-block:: text

   C5 4E 44 C6 4F 52 ...
   E  N  D  F  O  R ...   (bit 7 marks the first letter of each keyword)
   token 80h = END; token 81h = FOR; token B2h = PRINT

Removing the ROM bytes without relocating every data reference would break
BASIC. Keeping 12 KB of data is a deliberate compatibility choice, not evidence
of a hidden runtime opcode interpreter.

The original reset instructions
-------------------------------

.. code-block:: asm

   ; original ROM addresses and bytes
   ; 0000: F3
   DI
   ; 0001: AF
   XOR A
   ; 0002: C3 74 06
   JP 0x0674

Their generated cases update ``iff1/iff2``, ``A/F`` and ``pc`` directly. Each
emitted case includes a mixed comment with the ROM address, exact instruction
bytes, and Z80 mnemonic. Since every byte offset is translated, comments at data
offsets are possible instruction interpretations, not claims of real functions.
