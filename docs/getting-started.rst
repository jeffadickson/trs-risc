Build, dependencies and commands
================================

Host prerequisites
------------------

Use Linux x86-64 with a C compiler, Make, curl, tar, jq, sha512sum and Python 3
with venv support, plus GNU or LLVM objcopy (binutils). The C# utilities target **.NET 8** and use only framework
libraries: there are no NuGet package dependencies. Sphinx is the sole use of
Python in the project; ``docs/conf.py`` is its configuration, not a build tool.

.. code-block:: sh

   make bootstrap       # local .NET SDK and Sphinx installation
   make host            # C# tools, verified ROM, generated C, C executables
   make test            # differential CPU and end-to-end BASIC tests
   make demo            # Demo.cs drives the host executable
   make interactive     # press Enter at MEMORY SIZE?, then type BASIC

``make host`` automatically obtains .NET if absent, builds the tools, fetches the
ROM if absent, translates it, and compiles the executables. ``make docs`` obtains
Sphinx if absent. Downloads require network access, including any sandbox approval.
Nothing is installed into system directories. ``.toolchain`` is ignored by Git.
Microsoft's release manifest provides the SDK archive's checked SHA-512 digest.

An existing ``dotnet`` on PATH is used in preference to downloading the SDK.
The pinned fallback is SDK 8.0.413. The bootstrap script currently supports
Linux x86-64; on other systems, install suitable dependencies and override the
Make variables. The generated C requires a little-endian target.

RISC-V
------

.. code-block:: sh

   make bootstrap-riscv
   make riscv
   make test-riscv
   make size-report

System ``riscv64-linux-gnu-gcc`` and ``qemu-riscv64`` can be used. Otherwise,
bootstrap downloads Zig 0.14.1 and locally extracts Debian QEMU user mode.
This workspace may already have a Zig compiler under ``.toolchain/ziglang``;
the Makefile detects that too. The local Debian QEMU binary needs a compatible
Linux host. This is a Linux userspace target, not bare-metal firmware.

Override tools explicitly when needed:

.. code-block:: sh

   make riscv RISCV_CC=riscv64-linux-gnu-gcc RISCV_FLAGS='-O1 -static'
   make test-riscv QEMU=qemu-riscv64
   make docs

``riscv`` re-enters Make after bootstrap so newly installed compiler paths are
found. ``CC`` and ``CFLAGS`` control host compilation. ``ROM`` selects an existing
image; the known writable RAM stubs and console ABI are specific to the pinned
Model I ROM, so a different ROM is not automatically compatible.

Interactive options
-------------------

.. code-block:: text

   trs80-host [--input TEXT] [--steps N] [--screen] [--raw] [--trace] [--interactive]

Without arguments it submits two Enter keys and ``PRINT 2+2``. Interactive input
uses the host terminal's line buffering. EOF exits successfully. The default
budget is 20 million steps; use ``--steps`` for longer sessions. A budget exit
returns 3, unsupported code returns 2, normal EOF/HALT returns 0.

``--raw`` disables console traps and uses the original ROM display/keyboard
paths. Keyboard-matrix reads return no key, so raw mode can boot to ``MEMORY
SIZE?`` but cannot receive host keystrokes. ``--screen`` dumps the 64-by-16 video
buffer to stderr. ``--trace`` emits original Z80 addresses/register values.

Building this manual
--------------------

.. code-block:: sh

   make docs
   # open build/docs/html/index.html

The build runs Sphinx with ``-n -W --keep-going``: unresolved references and other
warnings fail the build. The size page is generated from the actual RV64 ELF.

RISC-V-in-WASM target
---------------------

.. code-block:: sh

   make riscv-wasm
   make test-riscv-wasm
   make serve

This path specifically requires Zig to build a soft-float RV64IMA Linux guest;
the ordinary hard-float glibc cross compiler is not assumed to have compatible
multilibs. ``ensure-zig`` obtains it before recursive Make discovers the path.
The C# server opens http://localhost:8080/; the default page uses the actual
RV64 ELF inside the WASM CPU. Use ``?engine=direct`` for direct C-to-WASM.

The RV64 browser guest targets keep an unstripped ``.full`` file in build/ and
remove DWARF payload from the served ELF while preserving symbols. This keeps
compiler-dependent debug size out of the emulator upload budget. The fixed-size
Z80 ROM and all loadable BASIC code/data remain present.
