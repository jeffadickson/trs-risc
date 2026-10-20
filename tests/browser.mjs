/** Real Chromium E2E over HTTP, matching normal usage. No Playwright package,
 * Python server, file:// exemptions or virtual-clock completion assumptions.
 * Chrome DevTools Protocol waits for the actual WASM/BASIC demo to finish.
 */
import {spawn} from 'node:child_process';
import {mkdtemp,readFile,rm,writeFile} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join,resolve,extname} from 'node:path';
import http from 'node:http';
import assert from 'node:assert/strict';
const sleep=milliseconds=>new Promise(done=>setTimeout(done,milliseconds));
const root=resolve('web'),profile=await mkdtemp(join(tmpdir(),'trs-risc-browser-'));
const server=http.createServer(async(request,response)=>{
  try{
    const pathname=decodeURIComponent(new URL(request.url,'http://localhost').pathname);
    const path=resolve(root,'.'+(pathname==='/'?'/index.html':pathname));
    if(!path.startsWith(root+'/'))throw new Error('outside web root');
    const bytes=await readFile(path);
    response.writeHead(200,{'Content-Type':({'.html':'text/html','.mjs':'text/javascript','.css':'text/css','.wasm':'application/wasm'})[extname(path)]||'application/octet-stream'});
    response.end(bytes);
  }catch{response.writeHead(404);response.end();}
});
let browser,socket,closed=false,error='',sequence=0;
const pending=new Map(),exceptions=[];
try{
  await new Promise((done,fail)=>{server.once('error',fail);server.listen(0,'127.0.0.1',done);});
  const url=`http://127.0.0.1:${server.address().port}/index.html?test=demo`;
  browser=spawn(process.env.CHROMIUM||'chromium',[`--user-data-dir=${profile}`,'--headless','--no-sandbox','--disable-dev-shm-usage','--disable-gpu','--remote-debugging-port=0','about:blank']);
  browser.on('close',()=>{closed=true;});browser.stderr.on('data',chunk=>{error+=chunk;});
  browser.on('error',failure=>{error+=failure.message;closed=true;});
  const deadline=Date.now()+45000;let port;
  while(Date.now()<deadline&&!port){
    if(closed)throw new Error('Chromium exited: '+error);
    try{port=Number((await readFile(join(profile,'DevToolsActivePort'),'utf8')).split('\n')[0]);}catch{await sleep(50);}
  }
  assert.ok(port,'Chromium debugging port unavailable: '+error);
  const pages=await (await fetch(`http://127.0.0.1:${port}/json/list`)).json();
  socket=new WebSocket(pages.find(page=>page.type==='page').webSocketDebuggerUrl);
  socket.addEventListener('close',()=>{for(const waiter of pending.values())waiter.fail(new Error('Chrome debugging connection closed'));pending.clear();});
  socket.addEventListener('message',event=>{
    const message=JSON.parse(event.data);
    if(message.id&&pending.has(message.id)){
      const {done,fail}=pending.get(message.id);pending.delete(message.id);
      if(message.error)fail(new Error(JSON.stringify(message.error)));else done(message.result);
    }
    if(message.method==='Runtime.exceptionThrown')exceptions.push(message.params.exceptionDetails.exception?.description||message.params.exceptionDetails.text);
  });
  await new Promise((done,fail)=>{socket.addEventListener('open',done,{once:true});socket.addEventListener('error',fail,{once:true});});
  const command=(method,params={})=>new Promise((resolve,reject)=>{const id=++sequence;const timer=setTimeout(()=>{pending.delete(id);reject(new Error('Chrome command timed out: '+method));},10000);pending.set(id,{done:value=>{clearTimeout(timer);resolve(value);},fail:failure=>{clearTimeout(timer);reject(failure);}});socket.send(JSON.stringify({id,method,params}));});
  await command('Runtime.enable');await command('Page.enable');await command('Page.navigate',{url});
  let snapshot={};
  while(Date.now()<deadline){
    const result=await command('Runtime.evaluate',{expression:'JSON.stringify({booted:document.documentElement.dataset.booted,done:document.documentElement.dataset.demoTested,status:document.getElementById("status")?.textContent,output:document.getElementById("terminal")?.textContent})',returnByValue:true});
    if(result.result.value)snapshot=JSON.parse(result.result.value);
    if(snapshot.done==='true')break;
    if(exceptions.length)throw new Error(exceptions.join('\n'));
    await sleep(50);
  }
  assert.equal(snapshot.booted,'true',JSON.stringify(snapshot));
  assert.equal(snapshot.done,'true',JSON.stringify(snapshot));
  assert.match(snapshot.output,/HELLO FROM RISC-V\n 1  1 \n 2  4 \n 3  9 \n 4  16 \n 5  25/);
  const screenshot=await command('Page.captureScreenshot',{format:'png'});
  await writeFile('build/browser.png',Buffer.from(screenshot.data,'base64'));
  console.log('PASS: Chromium over HTTP boots RV64 PIE/WASM, executes the real demo and renders results');
}finally{
  socket?.close();
  if(browser&&!closed){browser.kill('SIGTERM');for(let tick=0;tick<20&&!closed;tick++)await sleep(50);if(!closed)browser.kill('SIGKILL');}
  server.closeAllConnections();server.close();
  await rm(profile,{recursive:true,force:true,maxRetries:3,retryDelay:100});
}
