# trs-risc — TRS-80 BASIC, RISC-V, and WebAssembly

The original **Model I Level II BASIC ROM**, translated ahead of time to native
code, with a C program-image workbench and a newly written **RV64IMA emulator
compiled to WebAssembly**.

Repository: https://github.com/jeffadickson/trs-risc

There are two browser execution paths:

1. **Default:** Z80 ROM → C → actual RV64 Linux ELF → our RISC-V CPU → WASM → HTML.
2. **Direct:** Z80 ROM → C → WASM → HTML, available as `?engine=direct`.

The default path runs real RISC-V instructions. The emulator is 35 KB of stripped
WASM and has a static ELF loader, bounded guest RAM and a small Linux syscall
layer. RV64I, M, and single-hart A are implemented; C/F/D, privileged hardware,
an MMU, threads, dynamic linking and general Linux compatibility are not.

```sh
make bootstrap              # local .NET 8 SDK + Sphinx
make host                   # C# tools, pinned/verified ROM, translated C, binaries
make demo                   # Demo.cs supplies source; the ROM computes the output
make test
make bootstrap-riscv        # local Zig compiler + QEMU if needed
make riscv-wasm             # actual RV64IMA guest ELF + WASM RISC-V CPU
make test-riscv-wasm         # CPU/WASM behavior and same-ELF QEMU comparison
make serve                  # C# server: http://localhost:8080/
make docs                   # strict Sphinx build: build/docs/html/index.html
```

The Makefile brings in missing tools. Bootstrap currently targets Linux x86-64;
see [build details](docs/getting-started.rst). Project tools are C#; Python is used
only by external Sphinx and its configuration. C# tools use no NuGet packages.
The native C and compiled WASM executables do not need .NET at runtime.

```basic
10 PRINT "HELLO FROM RISC-V"
20 FOR I=1 TO 5
30 PRINT I;I*I
40 NEXT I
50 END
```

`Demo.cs` submits this text and `RUN`. The ROM tokenizes and executes it; loop and
multiplication results are not injected by a demo driver. BASIC was not rewritten
in C: the C# translator emits C for the ROM's Z80 instructions. Generated cases
include original Z80 bytes, addresses and assembly comments, and runtime helpers
are heavily annotated with Z80 instruction/flag semantics.

The C workbench uses the same ROM tokenizer and LIST routine:

```sh
build/basic-tool tokenize examples/hello.bas build/hello.basram
build/basic-tool detokenize build/hello.basram build/hello-listed.bas
build/basic-tool list build/hello.basram
build/basic-tool run build/hello.basram
build/basic-tool check examples/errors.bas
```

The demo image is **68 bytes of authentic program memory at 42E9h**, including
linked line pointers. A separate conservative static parser visits all stored
lines and reports multiple errors without executing the program. Coverage limits
are explicit: it does not prove types, builtin arity, runtime errors or unsupported
hardware/editor syntax. Images load/save through both browser execution paths.

The interpreter comes from a pinned 12,288-byte ROM download, verified with
SHA-256. Original bytes remain embedded as data for tables and strings. The
native translation is much larger: `make size-report` measures the translated
function, ELF sections and whole-file overhead separately. This is not an ISA
speed or code-density benchmark.

Read the Sphinx sources: [ROM pipeline](docs/rom-pipeline.rst),
[architecture](docs/architecture.rst), [memory images](docs/basic-workbench.rst),
[browser](docs/browser.rst), [RISC-V CPU](docs/riscv-emulator.rst),
[annotated source](docs/source-reference.rst), and [sizes](docs/sizes.rst).

Console BASIC works; disk, tape, physical keyboard timing, external interrupts
and arbitrary Z80 RAM machine code remain outside this prototype. RISC-V is a
Linux userspace target, not bare-metal firmware.

New code is MIT licensed. The historical ROM and generated derivatives retain
third-party rights and are excluded from the source repository/archive. The Z80
reference is test-only, with its included MIT license.
