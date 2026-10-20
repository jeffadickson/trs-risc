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
const qemu=spawnSync(process.env.QEMU || '.toolchain/qemu/usr/bin/qemu-riscv64',['build/trs80-rv64im','--input',script],{encoding:'utf8',timeout:30000});
assert.equal(native.status,0,native.stderr);assert.equal(qemu.status,0,qemu.stderr);assert.equal(native.stdout,qemu.stdout);
console.log('PASS: new native RV64 CPU output matches QEMU for identical guest ELF');
const unsupported=elf.slice();new DataView(unsupported.buffer).setUint32(48,1,true);
const invalid=await createRiscvMachine(wasm,unsupported);assert.throws(()=>invalid.reset(),/compressed or hard-float/);
console.log('PASS: unsupported ISA ELF rejected before guest execution');
boot();machine.input('10 INPUT A\n20 PRINT A\n30 END\nRUN\n');settle();machine.input('123\n');settle();assert.match(output,/\n 123 \n/);
console.log('PASS: Linux readv suspends/resumes interactive INPUT');
// The tiny PIE has no CRT self-relocator, so it proves the WASM loader itself
// applies R_RISCV_RELATIVE. Check completion as well as its actual stdout.
const fixture=await readFile('build/pie-guest');
for(const base of [0x400000,0xc00000]){
 let result='';const guest=await createRiscvMachine(wasm,fixture,text=>{result+=text;},{base});guest.reset();
 let state=0;for(let slice=0;slice<100&&state===0;slice++)state=guest.runSlice(10000);
 assert.equal(state,2);assert.equal(result,'PIE relocation OK\n');
}
console.log('PASS: real static PIE function/data relocations execute in WASM at two bases');
const basicPie=await readFile('web/trs80-pie.elf');
for(const base of [0x400000,0xc00000]){
 let result='';const guest=await createRiscvMachine(wasm,basicPie,text=>{result+=text;},{base});guest.reset();
 const ready=()=>{for(let slice=0;slice<1500;slice++)if(guest.runSlice(500000)===1)return;throw new Error('relocated BASIC did not reach input');};
 guest.input('\n\n');ready();assert.match(result,/LEVEL II BASIC\nREADY/);
 guest.load(image);guest.input('RUN\n');ready();assert.match(result,/HELLO FROM RISC-V\n 1  1 \n 2  4 \n 3  9 \n 4  16 \n 5  25/);assert.deepEqual(guest.save(),image);
}
console.log('PASS: full BASIC PIE boots, loads/RUNs/saves the same program at two bases');
