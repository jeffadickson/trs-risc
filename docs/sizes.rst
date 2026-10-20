Z80 versus native code size
==========================

Run ``make size-report`` to measure the current RV64 ELF directly. ``make docs``
does this automatically. The C# measurement utility reads ELF64 section headers
and symbol sizes; it does not estimate from source text.

.. include:: _generated/size.rst

Interpretation
--------------

The original 12,288-byte ROM includes instructions, tables, constants and strings.
The prototype translates every byte offset into a native entry, including data
and overlapping instruction streams. Each entry needs explicit Z80 byte/flag
semantics and dispatch. This deliberately conservative design expands strongly.
It is not a like-for-like Z80-versus-RISC-V instruction density benchmark.

Report the translated instruction-function size separately from all of ``.text``
(runtime/startup/libc), ``.rodata`` (ROM and dispatch data), initialized ``.data``,
zero-fill RAM ``.bss``, and the entire ELF including symbols/debug metadata.
The embedded ``rom_image`` is already part of ``.rodata``: do not sum those rows.
``.bss`` occupies RAM but adds no payload bytes to the file.

A baseline RV64 build measured 654,290 bytes for ``translated_step`` versus
12,288 ROM bytes, about **53.25 times**. Entire text was 680,496 bytes. Those
numbers describe that particular unstripped Zig build and must be regenerated
when compiler/flags/source change. Symbols/debug information can dominate the
unstripped file size even when the application uses ``-g0``. WASM file size is
also not directly comparable to Z80 ROM size; it includes module structure and
WASI-linked support. The browser target strips debug metadata by default.

A future smaller port could analyze reachable code, form basic blocks, reduce
PC dispatch and specialize flag liveness. None of these ratios measures speed.
