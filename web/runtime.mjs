/**
 * Minimal browser/Node host for the WASI-linked C module. This code queues keys
 * and forwards characters; BASIC and Z80 instruction semantics remain in WASM.
 * Execution slices are controlled by the caller so the browser can repaint.
 */
export async function createMachine(bytes, writeCharacter = () => {}) {
  let instance;
  const decoder = new TextDecoder();
  const wasi = {
    // Existing C diagnostics use fprintf(stderr,...). Decode WASI iovecs rather
    // than assuming printf writes a single contiguous string.
    fd_write(fd, vectors, count, written) {
      const memory = instance.exports.memory;
      const view = new DataView(memory.buffer);
      let total = 0;
      for (let index = 0; index < count; index++) {
        const pointer = view.getUint32(vectors + index * 8, true);
        const length = view.getUint32(vectors + index * 8 + 4, true);
        writeCharacter(decoder.decode(new Uint8Array(memory.buffer, pointer, length)));
        total += length;
      }
      view.setUint32(written, total, true);
      return 0;
    },
    args_get() { return 0; },
    args_sizes_get(count, size) {
      const view = new DataView(instance.exports.memory.buffer);
      view.setUint32(count, 0, true); view.setUint32(size, 0, true); return 0;
    },
    fd_close() { return 0; },
    fd_seek() { return 29; }, // ESPIPE: browser terminal has no seek position.
    proc_exit(code) { throw new Error(`BASIC runtime exited with status ${code}`); },
    environ_sizes_get(count, size) {
      const view = new DataView(instance.exports.memory.buffer);
      view.setUint32(count, 0, true); view.setUint32(size, 0, true); return 0;
    },
    environ_get() { return 0; },
  };
  const result = await WebAssembly.instantiate(bytes, {
    env: {
      console_write(value) {
        if (value === 13) writeCharacter('\n');
        else if (value >= 32 && value < 127) writeCharacter(String.fromCharCode(value));
      },
    },
    wasi_snapshot_preview1: wasi,
  });
  instance = result.instance;
  if (instance.exports._initialize) instance.exports._initialize();
  let pending = '';
  return {
    /** Reset all writable C/Z80 state; the original ROM performs BASIC boot. */
    reset() { pending = ''; instance.exports.web_reset(); },
    /** Queue ASCII keystrokes. They are consumed by CALL 002Bh, not parsed here. */
    input(text) {
      if (/[^\x00-\x7f]/.test(text)) throw new Error('BASIC input must be ASCII.');
      pending += text;
    },
    /** Run at most budget native-translated instruction/console events. */
    runSlice(budget = 20000) {
      let consumed = 0;
      while (consumed < pending.length && instance.exports.web_enqueue(pending.charCodeAt(consumed))) consumed++;
      pending = pending.slice(consumed);
      const state = instance.exports.web_run(budget);
      return state === 1 && pending.length ? 0 : state;
    },
    steps() { return instance.exports.web_step_count(); },
    /** Download only BASIC linked-list program bytes, excluding ROM/workspace. */
    save() {
      const start = instance.exports.web_program_pointer();
      const length = instance.exports.web_program_length();
      if (length < 2 || length > 0xad17) throw new Error('Invalid BASIC program extent. Reset BASIC first.');
      return new Uint8Array(instance.exports.memory.buffer, start, length).slice();
    },
    /** Upload raw 42E9h-based program bytes at a fresh, waiting BASIC prompt. */
    load(image) {
      if (image.length < 2 || image.length > 0xad17) throw new Error('Invalid program image size.');
      const start = instance.exports.web_upload_pointer();
      new Uint8Array(instance.exports.memory.buffer, start, image.length).set(image);
      if (!instance.exports.web_upload(image.length)) throw new Error('Invalid image links, line numbers, or BASIC is not waiting for input.');
    },
  };
}
