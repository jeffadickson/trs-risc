Mixed Z80 / native source reference
===================================

Each runtime helper is annotated with the Z80 instruction families it implements
and flag rules. Shared helpers implement many instructions and do not have a
single original BASIC routine address. Verified console entries and known copied
RAM stubs have exact addresses and bytes in the comments.

The C# generator also emits a mixed comment for every generated case:

.. code-block:: c

   /* Z80 0000: F3  DI */
   case 0: refresh=(refresh&128)|((refresh+1)&127);
           iff1=iff2=0;pc=1;return;

``build/translated.inc`` contains the full local listing. Since every byte offset
is an entry, listings inside tables are possible instruction interpretations,
not recovered function boundaries. That generated file includes original ROM
data and is not committed. Assembly and C come from the same decoded operands.

C runtime and flag semantics
----------------------------

.. literalinclude:: ../src/runtime.c
   :language: c
   :linenos:

C BASIC image driver
--------------------

.. literalinclude:: ../src/basic_tool.c
   :language: c
   :linenos:

C static syntax pass
--------------------

.. literalinclude:: ../src/basic_syntax.inc
   :language: c
   :linenos:

C# instruction translator
-------------------------

.. literalinclude:: ../tools/TrsRisc.Tools/Translator.cs
   :language: csharp
   :linenos:

C# demo
-------

.. literalinclude:: ../tools/TrsRisc.Tools/Demo.cs
   :language: csharp
   :linenos:

Browser C interface
-------------------

.. literalinclude:: ../src/web.c
   :language: c
   :linenos:

C# tool commands
----------------

Run ``dotnet build/tools/TrsRisc.Tools.dll COMMAND ...`` (the Makefile handles this).

.. code-block:: text

   translate ROM OUTPUT          mixed-comment native C
   fetch-rom PATH                pinned, verified ROM download
   fixtures DIRECTORY            synthetic instruction test C
   demo COMMAND [ARGS...]        real BASIC demo on host or QEMU
   smoke COMMAND [ARGS...]       runtime behavior tests
   basic-smoke COMMAND [ARGS...] token image / errors / corruption tests
   size-report ROM ELF OUTPUT    ELF section/symbol measurements as Sphinx text
   serve DIRECTORY PORT          localhost static HTML/WASM server

These tools use .NET framework libraries only. No Python project utilities or
NuGet packages are needed. Python remains solely for the external Sphinx package
and its configuration file.
