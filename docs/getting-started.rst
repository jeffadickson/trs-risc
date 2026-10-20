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

Windows with WSL2
-----------------

Use an x86-64 Linux distribution in WSL2 (Ubuntu 24.04 was used for local
validation). Run the Make commands inside Linux, not Windows PowerShell. Clone
with LF line endings so shell scripts remain executable by a POSIX shell:

.. code-block:: sh

   git -c core.autocrlf=false clone https://github.com/jeffadickson/trs-risc.git
   cd trs-risc
   git config core.autocrlf false
   make bootstrap
   make bootstrap-riscv
   make test
   make test-riscv
   make test-wasm
   make test-riscv-wasm
   make docs
   CHROMIUM=/path/to/linux/chrome make test-browser
   make serve

Open http://localhost:8080/ in the Windows browser. WSL localhost forwarding
must be enabled. The default runs the RV64 PIE through the WASM CPU; use
``?base=0xc00000`` for 12 MB, ``?elf=exec`` for fixed-address ELF, and
``?engine=direct`` for the separate direct WASM comparison.

The prerequisites above still apply inside WSL. Install Node 22 and Linux
Chromium (or Chrome for Testing) for the Node and browser checks; a Windows
browser executable is not a replacement for the Linux browser used by the
current test harness. Set ``CHROMIUM`` to its Linux executable path. No npm
packages are required. Browser shared libraries must also be available.

An existing ``dotnet`` command does not guarantee that the required .NET 8 SDK
and runtime are installed. Check ``dotnet --list-sdks`` and
``dotnet --list-runtimes``; .NET 10 alone was insufficient. The current bootstrap
checks command presence, so provide .NET 8 explicitly in this case. A Linux SDK
installed in ``.toolchain/dotnet`` is automatically selected by Make. If using
checkout-local Node, add ``.toolchain/node/bin`` to PATH before tests.

Extracting SDKs, Python packages and Zig's many small files on a Windows-mounted
``/mnt/c`` checkout can be slow. A checkout on the WSL Linux filesystem avoids
that overhead. Generated files and local toolchains remain ignored either way;
do not run ``git clean`` or reset an existing checkout to change platforms.

If Zig's first libc build stalls on Windows-mounted cache files, select a cache
on the Linux filesystem explicitly (a Make command-line variable overrides the
checkout-local default):

.. code-block:: sh

   make riscv-wasm ZIG_GLOBAL_CACHE_DIR=/var/tmp/trs-risc-zig-cache

Use the same override for subsequent cross/WASM Make commands. Host validation
can use ``make test CFLAGS=-O0``, matching CI and avoiding expensive GCC
optimization of the large generated workbench translation. These choices do not
change the ROM, BASIC program or target ISA.
