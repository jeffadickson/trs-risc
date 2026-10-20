/** Compare an ordinary linker/QEMU executable with the original ET_REL object
 * linked at runtime. WASM checks use the same public ABI as the browser host. */
import {readFile} from 'node:fs/promises';
import {spawnSync} from 'node:child_process';
import assert from 'node:assert/strict';
import {createRiscvMachine} from '../web/riscv.mjs';
const expected='ET_REL linking OK\n';
const oracle=spawnSync(process.env.QEMU||'.toolchain/qemu/usr/bin/qemu-riscv64',['build/rel-oracle'],{encoding:'utf8',timeout:30000});
assert.equal(oracle.status,0,oracle.stderr);assert.equal(oracle.stdout,expected);
for(const base of ['0x400000','0xc00000']){
  const run=spawnSync('build/rv-run',['--base',base,'build/rel-guest.o'],{encoding:'utf8',timeout:30000});
  assert.equal(run.status,0,run.stderr);assert.equal(run.stdout,oracle.stdout);
}
console.log('PASS: runtime-linked ET_REL at two bases matches ordinary linker/QEMU output');
if(process.argv[2]==='wasm'){
  const wasm=await readFile('web/rv64.wasm'),object=await readFile('build/rel-guest.o');
  for(const base of [0x400000,0xc00000]){
    let output='';const machine=await createRiscvMachine(wasm,object,text=>{output+=text;},{base});
    machine.reset();assert.equal(machine.pc(),BigInt(base));
    assert.equal(machine.registers()[2]%16n,0n);
    assert.equal(machine.runSlice(1000),2);assert.equal(output,oracle.stdout);
    // A second reset must zero the BSS that the preceding run wrote with 85.
    output='';machine.reset();assert.equal(machine.runSlice(1000),2);assert.equal(output,oracle.stdout);
    assert.throws(()=>machine.save(),/no known BASIC memory symbol/);
  }
  // Failures go through the compiled loader, not a JavaScript ELF validator.
  const table=Number(object.readBigUInt64LE(40)),stride=object.readUInt16LE(58),count=object.readUInt16LE(60);
  const malformed=[];
  const truncated=object.subarray(0,table+stride*count-1);malformed.push([truncated,/section table/]);
  const abi=Buffer.from(object);abi.writeUInt32LE(1,48);malformed.push([abi,/ISA\/ABI/]);
  const symbol=Buffer.from(object);
  let symtab,strings,absoluteRecord,absoluteName;
  for(let i=1;i<count;i++)if(object.readUInt32LE(table+i*stride+4)===2)symtab=table+i*stride;
  strings=table+object.readUInt32LE(symtab+40)*stride;
  const symbolStart=Number(object.readBigUInt64LE(symtab+24)),stringStart=Number(object.readBigUInt64LE(strings+24));
  for(let i=1;i<Number(object.readBigUInt64LE(symtab+32))/24;i++){
    const name=stringStart+object.readUInt32LE(symbolStart+i*24);
    if(object.subarray(name,object.indexOf(0,name)).toString()==='message')symbol.writeUInt16LE(0,symbolStart+i*24+6);
    if(object.subarray(name,object.indexOf(0,name)).toString()==='absolute_value'){absoluteRecord=symbolStart+i*24;absoluteName=name;}
  }
  // Even an object advertising a 64 KB symbol named mem must not expose BASIC
  // controls using the executable/PIE address calculation. Its section layout
  // has to come from the linker, not this adapter's static symbol reader.
  const namedMemory=Buffer.from(object);namedMemory.write('mem\0',absoluteName);
  namedMemory[absoluteRecord+4]=0x11;namedMemory.writeBigUInt64LE(65536n,absoluteRecord+16);
  const withoutImages=await createRiscvMachine(wasm,namedMemory);withoutImages.reset();
  assert.throws(()=>withoutImages.save(),/no known BASIC memory symbol/);
  malformed.push([symbol,/unresolved/]);
  const unsupported=Buffer.from(object);
  for(let i=1;i<count;i++)if(object.readUInt32LE(table+i*stride+4)===4){unsupported.writeBigUInt64LE(51n,Number(object.readBigUInt64LE(table+i*stride+24))+8);break;}
  malformed.push([unsupported,/only NONE\/64/]);
  for(const [bytes,error] of malformed){const machine=await createRiscvMachine(wasm,bytes);assert.throws(()=>machine.reset(),error);}
  console.log('PASS: WASM ET_REL runs/resets at two bases and rejects malformed/unsupported objects');
}
