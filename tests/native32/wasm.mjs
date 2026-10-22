import {readFile} from 'node:fs/promises';
import assert from 'node:assert/strict';
let instance,output='';
({instance}=await WebAssembly.instantiate(await readFile('web/native32.wasm'),{env:{native_write:(p,n)=>{output+=new TextDecoder().decode(new Uint8Array(instance.exports.memory.buffer,p,n));}}}));
instance.exports._initialize?.();instance.exports.n32_web_reset();
function send(text){for(const c of text)assert.equal(instance.exports.n32_web_enqueue(c.charCodeAt(0)),1);for(let i=0;i<100;i++)if(instance.exports.n32_web_run(1000)===1)return;throw Error('Execution failed to yield');}
send('10 DEFINT I\n20 DIM A%(65536)\n30 A%(65536)=2147483647\n40 PRINT A%(65536)\n50 FOR I=40000 TO 40002\n60 PRINT I\n70 NEXT I\n80 INPUT N%\n90 PRINT N%\nRUN\n');
assert.match(output,/2147483647 *\n/);assert.match(output,/40000 *\n 40001 *\n 40002 *\n/);
send('2147483647\n');assert.match(output,/2147483647 *\nREADY/);
output='';send('PRINT "MATH";SIN(5.5);LOG(2);SQR(2);9^.5\n');
assert.match(output,/\nMATH-0\.705540478 +0\.693147182 +1\.41421354 +3 *\n/);
// An overlong editor line must be discarded, never executed as a valid prefix.
instance.exports.n32_web_reset();output='';send('PRINT "BAD"'+ ' '.repeat(1100)+'\n');assert.match(output,/LIMIT Error/);assert.ok(!output.includes('\nBAD\n'));
console.log('PASS native32 WASM: typed array, wide loop, suspended INPUT, ROM math, oversized line');
