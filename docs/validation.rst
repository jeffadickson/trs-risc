Validation and provenance
=========================

``make test`` compares C translation semantics against the independent test-only
``superzazu/z80`` core, pinned to revision
``d64fe10a2274e5e40019b1086bf7d8990cbc5f23`` with its original MIT license.
The core is not linked into production binaries.

The earlier baseline matched **628,344 instruction/console events** in lockstep,
including primary/alternate registers, flags, index and stack registers, refresh,
interrupt mode and complete memory. Console adapters are shared, so this checks
CPU translation rather than physical keyboard fidelity.

The synthetic baseline covers **25,616** single-instruction cases across seven
opcode families and 16 seeded register states. It checks documented flags;
undefined ED encodings and incomplete reference-core block-I/O flags are excluded.
Hidden WZ behavior, real interrupts and cycle timing are outside these tests.

.. code-block:: sh

   make test         # C CPU differential tests and C# behavioral tests
   make test-riscv   # native RV64 ELF behavior through QEMU
   make test-wasm    # compiled WASM through the exact browser host adapter
   make test-elf-loader # static PIE relocations and malformed ELF tests
   make test-riscv-wasm # full BASIC PIE at two bases, plus QEMU comparison
   make test-browser # optional real Chromium UI test
   make docs         # strict Sphinx warning/reference checks

Behavior tests cover boot, arithmetic, loops, strings, floating point and bounded
execution. Image tests cover authentic 68-byte storage, LIST/re-tokenization,
load/RUN, four simultaneous source errors, corrupt links, DATA/REM and quoted
colons. WASM tests exercise the same compiled C through browser imports, including
input suspension, image save/load, floating point and reset during an infinite loop.
A Node WASM test is not itself a browser UI test; Chromium is checked separately.

Only new C/C#/HTML/JS and documentation sources, test-only reference code and
examples belong in the repository. Downloaded ROMs, generated translations,
native/WASM executables, derived BASIC images and local dependencies are excluded.
The MIT license does not cover historical ROM rights or generated derivatives.
``VALIDATION.txt`` records the initial prototype; rerun current targets after changes.
