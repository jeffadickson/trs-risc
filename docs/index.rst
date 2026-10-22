trs-risc: Level II BASIC on RISC-V
==================================

A new functional RV32 rebuild is described in :doc:`native32`. The historical
reference path below remains available.

This is a historical programming experiment: run the **original TRS-80 Model I
Level II BASIC ROM** as ahead-of-time translated RV64 native code, then use that
same ROM as a tokenizer, detokenizer and program runner.

The BASIC interpreter was **not rewritten in C**. The C# build tool translates
Z80 instructions to C statements; a C compiler turns those statements into RISC-V
instructions. BASIC's own parser, keyword matching and floating-point routines
are still the original ROM algorithms. The program retains Z80 registers and
its 64 KB memory layout. It is a console-first RV64 Linux executable.

Repository: https://github.com/jeffadickson/trs-risc

.. toctree::
   :maxdepth: 2

   native32
   native32-handoff
   getting-started
   rom-pipeline
   architecture
   basic-workbench
   browser
   riscv-emulator
   et-rel
   et-rel-plan
   source-reference
   sizes
   validation

Quick example
-------------

.. code-block:: sh

   make host
   make demo
   build/basic-tool tokenize examples/hello.bas build/hello.basram
   build/basic-tool list build/hello.basram
   build/basic-tool run build/hello.basram

.. literalinclude:: ../examples/hello.bas
   :language: basic

The ``Demo.cs`` driver submits this source plus ``RUN`` as virtual keystrokes.
The ROM produces ``HELLO FROM RISC-V`` and the squares of 1 through 5; the driver
does not substitute or precompute that output.
