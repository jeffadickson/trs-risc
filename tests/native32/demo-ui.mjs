import {readFile} from 'node:fs/promises';
import assert from 'node:assert/strict';
const elements=new Map();for(const id of ['output','status','pause','source','profile','sizes','run','send','input','command'])elements.set('#'+id,{textContent:'',value:'',disabled:true});
elements.get('#profile').value='compact';elements.get('#source').value='10 PRINT "HELLO FROM THE RV32 ROM"\n20 PRINT EXP(1#)\n30 END';
globalThis.document={querySelector:selector=>elements.get(selector)??null};const frames=[];globalThis.requestAnimationFrame=fn=>frames.push(fn);
globalThis.fetch=async name=>({ok:true,arrayBuffer:async()=>{const data=await readFile(new URL('../../web/'+name,import.meta.url));return data.buffer.slice(data.byteOffset,data.byteOffset+data.byteLength);}});
await import('../../web/rv32-demo.mjs');for(let i=0;i<500&&frames.length;i++)frames.shift()();
assert.match(elements.get('#output').textContent,/\nHELLO FROM THE RV32 ROM\n/);assert.match(elements.get('#status').textContent,/READY/);

console.log("PASS simulator page: fetched assets, automatic startup, UART output and READY");

globalThis.fetch=async()=>({ok:false,status:404});
await import('../../web/rv32-demo.mjs?missing-assets');
assert.match(elements.get('#status').textContent,/HTTP 404/);
assert.match(elements.get('#status').textContent,/make serve-native32-simulator/);
console.log('PASS simulator page: missing assets show actionable build instructions');
