/** Test the actual compiled WASM through the exact browser import adapter. */
import { readFile } from 'node:fs/promises';
import assert from 'node:assert/strict';
import { createMachine } from '../web/runtime.mjs';
const bytes = await readFile(new URL('../web/trs-risc.wasm', import.meta.url));
let text = '';
const machine = await createMachine(bytes, chunk => { text += chunk; });
function settle() {
  for (let tick = 0; tick < 1000; tick++) if (machine.runSlice(20000) === 1) return;
  throw new Error('BASIC did not reach a waiting keyboard call within 20M steps');
}
function boot() { machine.reset(); text = ''; machine.input('\n\n'); settle(); assert.match(text, /RADIO SHACK LEVEL II BASIC\nREADY/); }
boot(); console.log('PASS: WASM boots original BASIC and suspends for input');
const source = await readFile(new URL('../examples/hello.bas', import.meta.url), 'utf8');
machine.input(source); settle();
const image = machine.save(); assert.equal(image.length, 68); assert.equal(image[4], 0xb2);
machine.input('RUN\n'); settle();
assert.match(text, /HELLO FROM RISC-V\n 1  1 \n 2  4 \n 3  9 \n 4  16 \n 5  25 \nREADY/);
console.log('PASS: WASM tokenizer stores 68-byte demo; ROM RUN computes squares');
boot(); machine.load(image); machine.input('LIST\nRUN\n'); settle();
assert.match(text, /10 PRINT "HELLO FROM RISC-V"/); assert.match(text, /HELLO FROM RISC-V\n 1  1/);
assert.deepEqual(machine.save(), image);
console.log('PASS: program image upload, LIST, RUN and save preserve exact bytes');
boot(); const corrupt = image.slice(); corrupt[0] = corrupt[1] = 0;
assert.throws(() => machine.load(corrupt), /Invalid image/);
console.log('PASS: corrupt program links rejected');
machine.input('PRINT SQR(81)\nPRINT 1/8\n'); settle();
assert.match(text, /\n 9 \n/); assert.match(text, /\n .125 \n/);
console.log('PASS: original floating-point routines execute inside WASM');
boot(); machine.input('10 GOTO 10\nRUN\n');
for(let tick=0;tick<100;tick++) machine.runSlice(10000);
assert.equal(machine.runSlice(1000),0);
boot(); machine.input('PRINT 2+2\n'); settle(); assert.match(text,/\n 4 \n/);
console.log('PASS: infinite program yields in slices and reset recovers');
