/** Launch actual Chromium against the local HTML/WASM without network access.
 * The file-access flag is for this isolated local test only; normal use is HTTP.
 */
import {mkdtemp,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {spawn} from 'node:child_process';
import {resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
import assert from 'node:assert/strict';
const url=pathToFileURL(resolve('web/index.html'));url.search='?test=demo';
const profile=await mkdtemp(join(tmpdir(),'trs-risc-browser-'));
const browser=spawn(process.env.CHROMIUM||'chromium',[`--user-data-dir=${profile}`,'--headless','--no-sandbox','--allow-file-access-from-files','--disable-dev-shm-usage','--disable-gpu','--virtual-time-budget=15000','--dump-dom',url.href]);
let output='',error='';browser.stdout.on('data',chunk=>output+=chunk);browser.stderr.on('data',chunk=>error+=chunk);
const timer=setTimeout(()=>browser.kill('SIGKILL'),30000);
const code=await new Promise((resolve,reject)=>{browser.on('close',resolve);browser.on('error',reject);});clearTimeout(timer);
await rm(profile,{recursive:true,force:true});
assert.equal(code,0,error);assert.match(output,/data-booted="true"/);assert.match(output,/data-demo-tested="true"/);assert.match(output,/HELLO FROM RISC-V/);
console.log('PASS: real Chromium boots HTML/WASM and computes demo output');
