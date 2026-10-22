# All project translation, fixture, demo and measurement tools are C#.
# Python is used only by the external Sphinx documentation package.
CC ?= cc
CFLAGS ?= -O1
ROM ?= vendor/model1-level2.rom
DOTNET = $(if $(wildcard .toolchain/dotnet/dotnet),$(CURDIR)/.toolchain/dotnet/dotnet,dotnet)
TOOLS_DLL = build/tools/TrsRisc.Tools.dll
TOOLS = DOTNET_CLI_HOME=$(CURDIR)/.toolchain/dotnet-home DOTNET_CLI_TELEMETRY_OPTOUT=1 $(DOTNET) $(TOOLS_DLL)
ZIG = $(firstword $(wildcard .toolchain/zig/zig .toolchain/ziglang/zig))
RISCV_CC ?= $(if $(ZIG),$(CURDIR)/$(ZIG) cc -target riscv64-linux-musl,riscv64-linux-gnu-gcc)
RISCV_FLAGS ?= -O1 -static
QEMU ?= $(if $(wildcard .toolchain/qemu/usr/bin/qemu-riscv64),$(CURDIR)/.toolchain/qemu/usr/bin/qemu-riscv64,qemu-riscv64)
SPHINX = $(CURDIR)/.toolchain/docs/bin/sphinx-build
export ZIG_GLOBAL_CACHE_DIR := $(CURDIR)/.toolchain/zig-cache
export DOTNET_CLI_HOME := $(CURDIR)/.toolchain/dotnet-home
export DOTNET_CLI_TELEMETRY_OPTOUT := 1

.PHONY: all host riscv tools bootstrap bootstrap-riscv ensure-dotnet ensure-docs ensure-riscv fetch-rom test test-riscv demo interactive docs size-report clean
all: host
# make brings in tools automatically; bootstrap preinstalls .NET + Sphinx.
bootstrap:
	sh tools/bootstrap.sh all
bootstrap-riscv:
	sh tools/bootstrap.sh riscv
ensure-dotnet:
	@command -v dotnet >/dev/null 2>&1 || test -x .toolchain/dotnet/dotnet || sh tools/bootstrap.sh dotnet
ensure-docs:
	@test -x $(SPHINX) || sh tools/bootstrap.sh docs
ensure-riscv:
	@command -v riscv64-linux-gnu-gcc >/dev/null 2>&1 || test -n '$(ZIG)' || sh tools/bootstrap.sh riscv
$(TOOLS_DLL): $(wildcard tools/TrsRisc.Tools/*.cs) tools/TrsRisc.Tools/TrsRisc.Tools.csproj NuGet.Config | ensure-dotnet
	$(DOTNET) build tools/TrsRisc.Tools -c Release -o $(CURDIR)/build/tools --nologo
tools: $(TOOLS_DLL)
$(ROM): $(TOOLS_DLL)
	$(TOOLS) fetch-rom $@
fetch-rom: $(TOOLS_DLL)
	$(TOOLS) fetch-rom $(ROM)
build/translated.inc: $(TOOLS_DLL) $(ROM)
	mkdir -p build
	$(TOOLS) translate $(ROM) $@
build/trs80-host: src/runtime.c build/translated.inc
	$(CC) $(CFLAGS) -Ibuild $< -o $@
build/basic-tool: src/basic_tool.c src/basic_syntax.inc src/runtime.c build/translated.inc
	$(CC) $(CFLAGS) -Ibuild $< -o $@
host: build/trs80-host build/basic-tool
# Re-enter make after bootstrap so local compiler paths are discovered afresh.
riscv: ensure-riscv
	$(MAKE) build/trs80-riscv64 build/basic-tool-riscv64
build/trs80-riscv64: src/runtime.c build/translated.inc
	$(RISCV_CC) $(RISCV_FLAGS) -Ibuild $< -o $@
build/basic-tool-riscv64: src/basic_tool.c src/basic_syntax.inc src/runtime.c build/translated.inc
	$(RISCV_CC) $(RISCV_FLAGS) -Ibuild $< -o $@
build/lockstep: tests/lockstep.c tests/reference/z80.c src/runtime.c build/translated.inc
	$(CC) $(CFLAGS) -Ibuild tests/lockstep.c tests/reference/z80.c -o $@
build/opcodes/translated.inc: $(TOOLS_DLL)
	$(TOOLS) fixtures build/opcodes
build/opcode-test: tests/opcodes.c tests/reference/z80.c src/runtime.c build/opcodes/translated.inc
	$(CC) $(CFLAGS) -Ibuild/opcodes tests/opcodes.c tests/reference/z80.c -o $@
test: host build/lockstep build/opcode-test
	build/lockstep
	build/opcode-test
	$(TOOLS) smoke build/trs80-host
	$(TOOLS) basic-smoke build/basic-tool
test-riscv: riscv
	$(TOOLS) smoke $(QEMU) build/trs80-riscv64
	$(TOOLS) basic-smoke $(QEMU) build/basic-tool-riscv64
demo: host
	$(TOOLS) demo build/trs80-host
interactive: host
	build/trs80-host --interactive
size-report: riscv
	$(TOOLS) size-report $(ROM) build/trs80-riscv64 docs/_generated/size.rst
docs: size-report ensure-docs
	$(SPHINX) -n -W --keep-going -b html docs build/docs/html
clean:
	rm -rf build

# The browser target uses the same generated ROM C and a bounded execution ABI.
WASM_CC ?= $(if $(ZIG),$(CURDIR)/$(ZIG) cc -target wasm32-wasi,zig cc -target wasm32-wasi)
WASM_FLAGS ?= -O1 -mexec-model=reactor -Wl,--strip-all
WASM_EXPORTS = web_reset web_enqueue web_run web_program_pointer web_program_length web_step_count web_upload_pointer web_upload
.PHONY: wasm test-wasm serve
web/trs-risc.wasm: src/web.c src/runtime.c build/translated.inc
	$(WASM_CC) $(WASM_FLAGS) -Ibuild src/web.c -Wl,--no-entry $(foreach symbol,$(WASM_EXPORTS),-Wl,--export=$(symbol)) -o $@
web/demo.bas: examples/hello.bas
	cp $< $@
wasm: ensure-zig
	$(MAKE) web/trs-risc.wasm web/demo.bas
test-wasm: wasm
	node tests/wasm.mjs
serve: wasm tools
	$(TOOLS) serve web 8080

.PHONY: test-browser
test-browser: wasm
	node tests/browser.mjs

# Actual RV64 guest running inside a second, independent WASM CPU emulator.
RV_GUEST_CC ?= $(if $(ZIG),$(CURDIR)/$(ZIG) cc -target riscv64-linux-musl,zig cc -target riscv64-linux-musl)
RV_GUEST_FLAGS = -mcpu=generic_rv64+m+a-c-d-f -mabi=lp64 -O1 -g0 -static
# LLVM/cross objcopy understands RV64 directly. Host GNU binutils can copy the
# generic ELF64 representation; set EM_RISCV=243 explicitly (these guests use
# e_flags=0). Keep the symbol table for browser BASIC-image memory lookup.
RV_OBJCOPY ?= $(if $(shell command -v llvm-objcopy 2>/dev/null),llvm-objcopy,$(if $(shell command -v riscv64-linux-gnu-objcopy 2>/dev/null),riscv64-linux-gnu-objcopy,objcopy -I elf64-little -O elf64-little --alt-machine-code=243))
RV_EXPORTS = rv_boot_at rv_load_bias rv_image_pointer rv_image_limit rv_boot rv_enqueue rv_run rv_steps rv_exit_status rv_fault_pointer rv_memory_pointer rv_memory_size rv_program_counter rv_register
.PHONY: riscv-wasm test-riscv-wasm
build/trs80-rv64im: src/runtime.c build/translated.inc Makefile
	$(RV_GUEST_CC) $(RV_GUEST_FLAGS) -Ibuild $< -o $@.full
	$(RV_OBJCOPY) --strip-debug $@.full $@
build/rv-run: src/rv/rv64.c src/rv/et_rel.inc
	mkdir -p build
	$(CC) -O2 $< -o $@
web/rv64.wasm: src/rv/rv64.c src/rv/et_rel.inc
	$(WASM_CC) -O2 -mexec-model=reactor -DRV_NO_MAIN -Wl,--strip-all $< -Wl,--no-entry $(foreach symbol,$(RV_EXPORTS),-Wl,--export=$(symbol)) -o $@
web/trs80-rv64im.elf: build/trs80-rv64im
	cp $< $@
riscv-wasm: ensure-zig
	$(MAKE) web/rv64.wasm web/trs80-rv64im.elf web/trs80-pie.elf web/demo.bas
serve: riscv-wasm
test-riscv-wasm: riscv-wasm build/rv-run
	QEMU="$(QEMU)" node tests/riscv-wasm.mjs

.PHONY: ensure-zig test-rv-cpu
ensure-zig:
	@test -n '$(ZIG)' || sh tools/bootstrap.sh riscv
build/rv-cpu-test: tests/rv_cpu.c src/rv/rv64.c src/rv/et_rel.inc
	$(CC) -O2 $< -o $@
test-rv-cpu: build/rv-cpu-test
	build/rv-cpu-test
test-riscv-wasm: test-rv-cpu

# Static PIE relocation tests: the small fixture has no self-relocating CRT.
.PHONY: test-elf-loader
build/pie-guest: tests/pie_start.S tests/pie_guest.c
	$(RV_GUEST_CC) -mcpu=generic_rv64+m+a-c-d-f -mabi=lp64 -fPIE -pie -nostdlib -static -Wl,-e,_start -O1 $^ -o $@
build/elf-loader-test: tests/elf_loader.c src/rv/rv64.c src/rv/et_rel.inc
	$(CC) -O2 $< -o $@
test-elf-loader: build/pie-guest build/elf-loader-test
	build/elf-loader-test
build/trs80-pie: src/runtime.c build/translated.inc Makefile
	$(RV_GUEST_CC) -mcpu=generic_rv64+m+a-c-d-f -mabi=lp64 -fPIE -pie -static -O1 -g0 -Ibuild $< -o $@.full
	$(RV_OBJCOPY) --strip-debug $@.full $@
web/trs80-pie.elf: build/trs80-pie
	cp $< $@
riscv-wasm: test-elf-loader

test-browser: riscv-wasm

# Section-based single-object linker. The ordinary linker/QEMU path is an
# independent output oracle; our runtime consumes the unlinked .o instead.
.PHONY: test-et-rel test-et-rel-wasm
build/rel-guest.o: tests/rel_guest.S | ensure-zig
	mkdir -p build
	$(RV_GUEST_CC) -mcpu=generic_rv64+m+a-c-d-f -mabi=lp64 -mno-relax -g0 -c $< -o $@
build/rel-oracle: build/rel-guest.o
	$(RV_GUEST_CC) -mcpu=generic_rv64+m+a-c-d-f -mabi=lp64 -mno-relax -nostdlib -static -Wl,-e,_start $< -o $@
build/et-rel-test: tests/et_rel.c src/rv/rv64.c src/rv/et_rel.inc
	mkdir -p build
	$(CC) -O2 $< -o $@
test-et-rel: build/rel-guest.o build/rel-oracle build/et-rel-test build/rv-run
	build/et-rel-test
	QEMU="$(QEMU)" node tests/et-rel.mjs native
test-et-rel-wasm: test-et-rel
	$(MAKE) web/rv64.wasm
	QEMU="$(QEMU)" node tests/et-rel.mjs wasm

# Rebuilt functional interpreter: native32 numeric model, independent of the
# historical ROM translator and its .NET/download pipeline.
N32_CORE = $(filter-out src/native32/cli.c src/native32/web.c,$(wildcard src/native32/*.c))
N32_WARN = -std=c11 -Wall -Wextra -Werror -Wno-misleading-indentation -ffp-contract=off
.PHONY: native32 test-native32
build/native32: $(N32_CORE) src/native32/cli.c src/native32/native.h
	mkdir -p build
	$(CC) $(N32_WARN) -Os -Isrc/native32 $(N32_CORE) src/native32/cli.c -o $@
native32: build/native32
build/native32-test: $(N32_CORE) tests/native32/test.c src/native32/native.h
	mkdir -p build
	$(CC) $(N32_WARN) -O2 -Isrc/native32 $(N32_CORE) tests/native32/test.c -o $@
build/native32-sanitize: $(N32_CORE) tests/native32/test.c src/native32/native.h
	mkdir -p build
	$(CC) $(N32_WARN) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc/native32 $(N32_CORE) tests/native32/test.c -o $@
test-native32: native32 build/native32-test
	build/native32-test

N32_RV32_CC = $(CURDIR)/$(ZIG) cc -target riscv32-linux-musl
N32_RV32_FLAGS = -mcpu=generic_rv32+m+a-c-d-f -mabi=ilp32 -Oz -static -ffunction-sections -fdata-sections -Wl,--gc-sections
.PHONY: native32-rv32 test-native32-rv32 native32-wasm
native32-rv32: ensure-zig
	$(MAKE) build/native32-rv32 build/native32-test-rv32
build/native32-rv32: $(N32_CORE) src/native32/cli.c src/native32/native.h
	mkdir -p build
	$(N32_RV32_CC) $(N32_WARN) $(N32_RV32_FLAGS) -Isrc/native32 $(N32_CORE) src/native32/cli.c -o $@
build/native32-test-rv32: $(N32_CORE) tests/native32/test.c src/native32/native.h
	mkdir -p build
	$(N32_RV32_CC) $(N32_WARN) $(N32_RV32_FLAGS) -Isrc/native32 $(N32_CORE) tests/native32/test.c -o $@
test-native32-rv32: native32-rv32
	$(CURDIR)/.toolchain/qemu/usr/bin/qemu-riscv32 build/native32-test-rv32
web/native32.wasm: $(N32_CORE) src/native32/web.c src/native32/native.h
	$(WASM_CC) $(N32_WARN) -Oz -mexec-model=reactor -Wl,--strip-all -Isrc/native32 $(N32_CORE) src/native32/web.c -Wl,--no-entry -Wl,--export=n32_web_reset -Wl,--export=n32_web_enqueue -Wl,--export=n32_web_run -Wl,--export=n32_web_steps -o $@
native32-wasm: ensure-zig
	$(MAKE) web/native32.wasm

N32_ROM_CC = $(CURDIR)/$(ZIG) cc -target riscv32-freestanding
N32_BOARD = src/native32/board/start.S src/native32/board/runtime.c src/native32/board/uart.c
N32_ROM_FLAGS = $(N32_WARN) -Oz -ffreestanding -fno-builtin -fno-strict-aliasing -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -nostdlib -Isrc/native32/board/include -Isrc/native32 -Wl,--gc-sections -Wl,-T,src/native32/board/rom.ld
.PHONY: native32-rom
native32-rom: ensure-zig
	$(MAKE) build/native32-rom.elf build/native32-rom-compact.elf
build/native32-rom.elf: $(N32_CORE) $(N32_BOARD) src/native32/native.h src/native32/board/rom.ld
	mkdir -p build
	$(N32_ROM_CC) -mcpu=generic_rv32+m+a-c-d-f -mabi=ilp32 $(N32_ROM_FLAGS) $(N32_CORE) $(N32_BOARD) -o $@
build/native32-rom-compact.elf: $(N32_CORE) $(N32_BOARD) src/native32/native.h src/native32/board/rom.ld
	mkdir -p build
	$(N32_ROM_CC) -mcpu=generic_rv32+m+a+c-d-f -mabi=ilp32 $(N32_ROM_FLAGS) -flto $(N32_CORE) $(N32_BOARD) -o $@

.PHONY: test-native32-wasm
test-native32-wasm: native32-wasm
	node tests/native32/wasm.mjs

.PHONY: native32-size docs-native32
native32-size: native32-rom
	mkdir -p docs/_generated
	@size build/native32-rom.elf build/native32-rom-compact.elf
	@{ echo 'Measured linked sections (bytes, text + initialized data; BSS excluded)::'; echo; size build/native32-rom.elf build/native32-rom-compact.elf | sed 's/^/   /'; } > docs/_generated/native32-size.rst
docs-native32: native32-size ensure-docs
	@test -f docs/_generated/size.rst || echo 'Historical translator size report not generated; run make size-report.' > docs/_generated/size.rst
	$(SPHINX) -n -W --keep-going -b html docs build/docs/html

.PHONY: serve-native32
serve-native32: native32-wasm tools
	$(TOOLS) serve web 8080
