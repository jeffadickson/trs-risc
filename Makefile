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
WASM_FLAGS ?= -O1 -Wl,--strip-all
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
RV_GUEST_FLAGS = -mcpu=generic_rv64+m+a-c-d-f -mabi=lp64 -O1 -static
RV_EXPORTS = rv_image_pointer rv_image_limit rv_boot rv_enqueue rv_run rv_steps rv_exit_status rv_fault_pointer rv_memory_pointer rv_memory_size rv_program_counter rv_register
.PHONY: riscv-wasm test-riscv-wasm
build/trs80-rv64im: src/runtime.c build/translated.inc
	$(RV_GUEST_CC) $(RV_GUEST_FLAGS) -Ibuild $< -o $@
build/rv-run: src/rv/rv64.c
	$(CC) -O2 $< -o $@
web/rv64.wasm: src/rv/rv64.c
	$(WASM_CC) -O2 -DRV_NO_MAIN -Wl,--strip-all $< -Wl,--no-entry $(foreach symbol,$(RV_EXPORTS),-Wl,--export=$(symbol)) -o $@
web/trs80-rv64im.elf: build/trs80-rv64im
	cp $< $@
riscv-wasm: ensure-zig
	$(MAKE) web/rv64.wasm web/trs80-rv64im.elf web/demo.bas
serve: riscv-wasm
test-riscv-wasm: riscv-wasm build/rv-run
	node tests/riscv-wasm.mjs

.PHONY: ensure-zig test-rv-cpu
ensure-zig:
	@test -n '$(ZIG)' || sh tools/bootstrap.sh riscv
build/rv-cpu-test: tests/rv_cpu.c src/rv/rv64.c
	$(CC) -O2 $< -o $@
test-rv-cpu: build/rv-cpu-test
	build/rv-cpu-test
test-riscv-wasm: test-rv-cpu
