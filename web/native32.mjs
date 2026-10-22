// Execute the functional C interpreter directly in WASM. This is not the RV64
// instruction emulator; its separate page remains available at index.html.
const output=document.querySelector('#output');
let instance, scheduled=false;
const decoder=new TextDecoder();
function write(pointer,length){output.textContent+=decoder.decode(new Uint8Array(instance.exports.memory.buffer,pointer,length));output.scrollTop=output.scrollHeight;}
function pump(){scheduled=false;if(instance.exports.n32_web_run(1000)===0){scheduled=true;requestAnimationFrame(pump);}}
function send(text){for(const ch of text){if(ch.charCodeAt(0)>127)throw Error('ASCII source required');if(!instance.exports.n32_web_enqueue(ch.charCodeAt(0)))throw Error('Keyboard queue full');}if(!scheduled){scheduled=true;requestAnimationFrame(pump);}}
try{
 const response=await fetch('native32.wasm');if(!response.ok)throw Error(`WASM download: ${response.status}`);
 ({instance}=await WebAssembly.instantiate(await response.arrayBuffer(),{env:{native_write:write}}));
 instance.exports._initialize?.();instance.exports.n32_web_reset();
 document.querySelector('#run').onclick=()=>{output.textContent='';instance.exports.n32_web_reset();send(document.querySelector('#source').value+'\nRUN\n');};
 document.querySelector('#input').onsubmit=e=>{e.preventDefault();const field=document.querySelector('#command');send(field.value+'\n');field.value='';};
 document.querySelector('#reset').onclick=()=>{output.textContent='';instance.exports.n32_web_reset();};
}catch(error){output.textContent=String(error)+'\nBuild with make native32-wasm.';}
