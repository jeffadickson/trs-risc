import { createMachine } from './runtime.mjs';
import { createRiscvMachine } from './riscv.mjs';
const settings = new URLSearchParams(location.search);
const direct = settings.get('engine') === 'direct';
const pie = settings.get('elf') !== 'exec';
const base = Number(settings.get('base') ?? '0x400000');
const $ = id => document.getElementById(id);
let machine, running = false, scheduled = false, generation = 0, transcript = '', output = '';
let waiters = [];
function flush() {
  if (output) { transcript = (transcript + output).slice(-120000); $('terminal').textContent = transcript; output = ''; $('terminal').scrollTop = $('terminal').scrollHeight; }
  $('steps').textContent = `${machine.steps().toLocaleString()} steps`;
}
function status(text) { $('status').textContent = text; }
function pump() {
  if (!running || scheduled) return;
  scheduled = true;
  const epoch = generation;
  setTimeout(() => {
    scheduled = false;
    if (epoch !== generation || !running) { if (running) pump(); return; }
    try {
      const state = machine.runSlice(direct ? 16000 : 300000); flush();
      if (state === 1) {
        running = false; status('Waiting for BASIC input');
        const callbacks = waiters; waiters = []; callbacks.forEach(resolve => resolve());
      } else if (state === 2) { running = false; status('CPU halted — reset to start again'); }
      else { status('Running BASIC'); pump(); }
    } catch (error) { running = false; flush(); status(error.message); }
  }, 0);
}
function send(text) { machine.input(text); running = true; $('pause').textContent = 'Pause'; pump(); }
function boot() {
  generation++; machine.reset(); transcript = output = ''; $('terminal').textContent = '';
  const waiting = new Promise(resolve => waiters.push(resolve));
  send('\n\n'); return waiting;
}
function fail(error) { status(error.message); }
$('command-form').addEventListener('submit', event => {
  event.preventDefault(); try { send($('command').value + '\n'); $('command').value = ''; } catch (error) { fail(error); }
});
$('pause').addEventListener('click', () => {
  running = !running; $('pause').textContent = running ? 'Pause' : 'Resume';
  if (running) pump(); else status('Paused — resume or reset');
});
$('reset').addEventListener('click', () => { $('pause').textContent = 'Pause'; boot(); });
async function runSource() {
  try {
    const source = $('source').value.trim();
    if (!source || source.split(/\r?\n/).some(line => line.trim() && !/^\s*\d+/.test(line))) throw new Error('Use numbered ASCII BASIC lines.');
    if (/[^\x20-\x7e\r\n]/.test(source)) throw new Error('BASIC source must use printable ASCII.');
    const epoch = generation + 1;
    await boot(); if (generation !== epoch) return; send(source + '\nRUN\n');
  } catch (error) { fail(error); }
}
$('demo').addEventListener('click', async () => { $('source').value = await (await fetch('demo.bas')).text(); runSource(); });
$('run-source').addEventListener('click', runSource);
$('save').addEventListener('click', () => {
  try {
    const url = URL.createObjectURL(new Blob([machine.save()], {type:'application/octet-stream'}));
    const link = document.createElement('a'); link.href = url; link.download = 'program.basram'; link.click(); setTimeout(() => URL.revokeObjectURL(url), 1000);
  } catch (error) { fail(error); }
});
$('load').addEventListener('change', async event => {
  try {
    const file = event.target.files[0]; if (!file) return;
    const bytes = new Uint8Array(await file.arrayBuffer());
    await boot(); machine.load(bytes); send('LIST\n');
  } catch (error) { fail(error); }
  event.target.value = '';
});
try {
  const response = await fetch(direct ? 'trs-risc.wasm' : 'rv64.wasm');
  if (!response.ok) throw new Error('WASM missing. Run make riscv-wasm and make serve.');
  const module = await response.arrayBuffer();
  if (direct) machine = await createMachine(module, text => { output += text; });
  else {
    if (!Number.isInteger(base) || base < 0 || base >= 0x2000000 || base % 4096) throw new Error('PIE base must be page aligned and below 32 MB.');
    const guest = await fetch(pie ? 'trs80-pie.elf' : 'trs80-rv64im.elf'); if (!guest.ok) throw new Error('RV64 guest ELF missing. Run make riscv-wasm.');
    machine = await createRiscvMachine(module, await guest.arrayBuffer(), text => { output += text; }, {base:pie ? base : 0});
  }
  $('engine').textContent = direct ? 'Direct C → WebAssembly' : `RV64 ${pie ? 'PIE @ 0x'+base.toString(16) : 'ELF'} → RISC-V CPU → WebAssembly`;
  $('source').value = await (await fetch('demo.bas')).text();
  for (const id of ['command','send','demo','pause','reset','save','load','run-source']) $(id).disabled = false;
  await boot(); $('command').focus(); document.documentElement.dataset.booted = 'true';
  // Opt-in test route uses the same UI handlers; it never supplies output.
  if (new URLSearchParams(location.search).get('test') === 'demo') {
    await runSource();
    await new Promise(resolve => waiters.push(resolve));
    if (transcript.includes('HELLO FROM RISC-V\n 1  1 \n 2  4 \n 3  9 \n 4  16 \n 5  25')) document.documentElement.dataset.demoTested = 'true';
  }
} catch (error) { fail(error); }
