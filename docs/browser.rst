HTML and WebAssembly
====================

Two paths are available. The default runs an actual RV64IMA Linux ELF in the
new C RISC-V CPU compiled to WASM. The alternate ``?engine=direct`` path compiles
the same translated BASIC C directly to wasm32-wasi. There is no JavaScript Z80
emulator or separate BASIC implementation. See :doc:`riscv-emulator` for the CPU
subset and Linux syscall limits.

.. code-block:: sh

   make wasm               # direct C-to-WASM path
   make test-wasm
   make riscv-wasm         # actual RV64 ELF inside a WASM CPU
   make test-riscv-wasm
   make serve
   # open http://localhost:8080/

``make serve`` uses the C# localhost server. It serves only static files, including
``application/wasm``; no server-side BASIC execution occurs. Do not normally open
the page with ``file://``: browser fetch restrictions require an HTTP server.
The entire ``web`` directory can be served by another static HTTP server too.
No publication or external hosting is required.

Controls
--------

The terminal accepts direct commands and INPUT responses. Run demo submits the
numbered sample and RUN. The source editor resets BASIC, submits numbered lines,
and runs them. Pause stops scheduling execution slices; Reset clears writable
state and starts the original ROM boot path. Save downloads the raw program
memory image. Load starts a fresh session, validates/uploads an image, and LISTs
it. Type RUN to execute the loaded program.

The browser source editor uses the ROM tokenizer; it does **not** invoke the C
static syntax checker. Invalid source may be stored and later report an error
when RUN reaches it. Use the C workbench for the documented all-lines check.
The source editor preflights ASCII and numbering but does not promise full syntax
validation. Images are compatible with the C workbench's 42E9h-based format.

How the module yields
---------------------

``web_run(budget)`` executes bounded instruction slices and returns running,
waiting-for-key, or halted. On a blocking CALL 002Bh with an empty queue, it leaves
PC/SP unchanged and yields. The next supplied key resumes that call. Nonblocking
polls return A=0. Character output comes from the CALL 0033h adapter. The UI uses
short timers between slices so infinite BASIC loops do not freeze input or Reset.
The loop is cooperative, not timing-accurate hardware emulation.

``web/runtime.mjs`` supplies the small WASI import surface needed by libc error
reporting and forwards normal console characters. JavaScript does not implement
BASIC arithmetic, branches, tokenization or Z80 semantics. ``web.c`` provides
reset, keyboard queue, bounded run, step count, and program image save/upload APIs.
Images are staged and validated before copying over the program area.

Verification
------------

``make test-wasm`` runs the actual WASM binary using the exact browser import
adapter under Node. ``make test-browser`` additionally launches Chromium against
the HTML UI. The optional browser test needs a Chromium executable and a sandbox
that permits its local sockets. The test's file-access flag is for isolated local
testing only, not required when normally serving over HTTP.
