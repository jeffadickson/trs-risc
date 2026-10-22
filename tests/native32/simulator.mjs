import {readFile} from 'node:fs/promises';
import assert from 'node:assert/strict';
const wasm=await readFile('web/rv32-board.wasm');
for(const profile of ['native32-rom','native32-rom-compact']){
 let output='';const {instance}=await WebAssembly.instantiate(wasm,{env:{uart_write:byte=>{if(byte!==13)output+=String.fromCharCode(byte);}}});
 const cpu=instance.exports;cpu._initialize?.();
 const image=await readFile(`web/${profile}.elf`);new Uint8Array(cpu.memory.buffer,cpu.rv32_image(),image.length).set(image);
 assert.equal(cpu.rv32_load(image.length),0);
 function until(marker){for(let i=0;i<1000;i++){const fault=cpu.rv32_run(100000);if(fault)throw Error(`${profile}: trap ${fault} PC=${cpu.rv32_pc().toString(16)} instruction=${cpu.rv32_instruction().toString(16)}\n${output}`);if(output.includes(marker))return;}throw Error(`${profile}: timeout\n${output}`);}
 until('READY\n>');output='';
 const source='10 DEFINT I\n20 PRINT "HELLO FROM RV32"\n30 FOR I=40000 TO 40002\n40 PRINT I\n50 NEXT I\n60 PRINT EXP(1#)\n70 INPUT N%\n80 PRINT N%\n90 PRINT "SIMULATION COMPLETE"\nRUN\n';
 for(const byte of source)assert.equal(cpu.rv32_enqueue(byte.charCodeAt(0)),1);
 until('2.718281828459');assert.match(output,/\nHELLO FROM RV32\n/);assert.match(output,/40000 *\n 40001 *\n 40002 *\n/);
 for(const byte of '2147483647\n')assert.equal(cpu.rv32_enqueue(byte.charCodeAt(0)),1);
 until('\nSIMULATION COMPLETE\nREADY\n>');assert.match(output,/2147483647 *\nSIMULATION COMPLETE/);
 console.log(`PASS actual ${profile} ELF: UART boot, wide loop, DOUBLE EXP, suspended INPUT; ${cpu.rv32_cycles()} RV32 instructions`);
 // Reject malformed images without destroying the loaded guest.
 const before=cpu.rv32_pc();new Uint8Array(cpu.memory.buffer,cpu.rv32_image(),1)[0]=0;assert.equal(cpu.rv32_load(image.length),1);assert.equal(cpu.rv32_pc(),before);
}
