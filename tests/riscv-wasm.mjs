/** Execute the actual RV64 ELF through the new WASM CPU and compare QEMU. */
import {readFile} from 'node:fs/promises';
import {spawnSync} from 'node:child_process';
import assert from 'node:assert/strict';
import {createRiscvMachine} from '../web/riscv.mjs';
const wasm=await readFile('web/rv64.wasm'),elf=await readFile('web/trs80-rv64im.elf');
let output='';const machine=await createRiscvMachine(wasm,elf,text=>{output+=text;});
function settle(){for(let index=0;index<1500;index++)if(machine.runSlice(500000)===1)return;throw new Error('guest did not reach stdin wait within 750M RV64 instructions');}
function boot(){output='';machine.reset();machine.input('\n\n');settle();assert.match(output,/RADIO SHACK LEVEL II BASIC\nREADY/);}
boot();console.log('PASS: actual RV64 ELF boots BASIC inside WASM CPU');
const source=await readFile('examples/hello.bas','utf8');machine.input(source);settle();const image=machine.save();assert.equal(image.length,68);
machine.input('RUN\n');settle();assert.match(output,/HELLO FROM RISC-V\n 1  1 \n 2  4 \n 3  9 \n 4  16 \n 5  25 \nREADY/);
console.log(`PASS: RV64 guest tokenizer stores 68-byte image; demo executes (${machine.steps()} RV64 instructions)`);
boot();machine.load(image);machine.input('LIST\nRUN\n');settle();assert.match(output,/10 PRINT "HELLO FROM RISC-V"/);assert.match(output,/HELLO FROM RISC-V\n 1  1/);assert.deepEqual(machine.save(),image);
console.log('PASS: program images move through RV64 guest memory and remain byte-exact');
machine.input('PRINT SQR(81)\nPRINT SIN(0)\nPRINT 1/8\n');settle();assert.match(output,/\n 9 \n/);assert.match(output,/\n 0 \n/);assert.match(output,/\n .125 \n/);
console.log('PASS: software floating-point BASIC routines run using RV64IMA only');
// Native CPU and QEMU receive identical ELF bytes/argv. Compare actual stdout,
// not diagnostic instruction counts or an expected string supplied by a driver.
const script='\n\n'+source+'RUN\nPRINT SQR(81)\n';
const native=spawnSync('build/rv-run',['build/trs80-rv64im','--input',script],{encoding:'utf8',timeout:30000});
const qemu=spawnSync('.toolchain/qemu/usr/bin/qemu-riscv64',['build/trs80-rv64im','--input',script],{encoding:'utf8',timeout:30000});
assert.equal(native.status,0,native.stderr);assert.equal(qemu.status,0,qemu.stderr);assert.equal(native.stdout,qemu.stdout);
console.log('PASS: new native RV64 CPU output matches QEMU for identical guest ELF');
const unsupported=elf.slice();new DataView(unsupported.buffer).setUint32(48,1,true);
const invalid=await createRiscvMachine(wasm,unsupported);assert.throws(()=>invalid.reset(),/compressed or hard-float/);
console.log('PASS: unsupported ISA ELF rejected before guest execution');
boot();machine.input('10 INPUT A\n20 PRINT A\n30 END\nRUN\n');settle();machine.input('123\n');settle();assert.match(output,/\n 123 \n/);
console.log('PASS: Linux readv suspends/resumes interactive INPUT');
