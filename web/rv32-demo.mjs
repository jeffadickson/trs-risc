// The same module supports HTTP assets and a generated self-contained HTML.
const embedded=document.querySelector('#embedded-images');
const payload=embedded?JSON.parse(embedded.textContent):null;
async function asset(name){if(payload){const raw=atob(payload[name]);return Uint8Array.from(raw,c=>c.charCodeAt(0));}const response=await fetch(name);if(!response.ok)throw Error(`${name}: HTTP ${response.status}`);return new Uint8Array(await response.arrayBuffer());}
const output=document.querySelector('#output'),status=document.querySelector('#status');
let cpu,images,text='',active=false,scheduled=false,profile='compact';
function show(){output.textContent=text;output.scrollTop=output.scrollHeight;}
function wake(){active=true;document.querySelector('#pause').textContent='Pause CPU';if(!scheduled){scheduled=true;requestAnimationFrame(tick);}}
function tick(){scheduled=false;if(!active)return;const fault=cpu.rv32_run(200000);show();
 status.textContent=`${profile==='compact'?'RV32IMAC + LTO':'RV32IMA'} · ${Number(cpu.rv32_cycles()).toLocaleString()} instructions · PC 0x${cpu.rv32_pc().toString(16)}`;
 if(fault){active=false;status.textContent+=` · TRAP ${fault}, instruction 0x${cpu.rv32_instruction().toString(16)}`;return;}
 if(!cpu.rv32_pending()&&text.endsWith('READY\n>')){active=false;status.textContent+=' · READY';}
 if(active){scheduled=true;requestAnimationFrame(tick);}
}
function send(source){if([...source].some(c=>c.charCodeAt(0)>127))throw Error('ASCII source required');for(const char of source)if(!cpu.rv32_enqueue(char.charCodeAt(0)))throw Error('UART input queue is full');wake();}
function romBytes(bytes){const view=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength);let sum=0;for(let i=0;i<view.getUint16(44,true);i++){const at=view.getUint32(28,true)+i*view.getUint16(42,true);if(view.getUint32(at,true)===1)sum+=view.getUint32(at+16,true);}return sum;}
function reset(run){profile=document.querySelector('#profile').value;const bytes=images[profile];new Uint8Array(cpu.memory.buffer,cpu.rv32_image(),bytes.length).set(bytes);if(cpu.rv32_load(bytes.length))throw Error('ROM ELF was rejected');text='';show();
 const size=romBytes(bytes);document.querySelector('#sizes').textContent=`Selected ROM: ${size.toLocaleString()} bytes · original Z80 Level II: 12,288 bytes · ${(size/12288).toFixed(2)}× the original. This is still a smaller feature subset.`;
 if(run)send(document.querySelector('#source').value+'\nRUN\n');else wake();
}
try{const wasm=await asset('rv32-board.wasm');images={baseline:await asset('native32-rom.elf'),compact:await asset('native32-rom-compact.elf')};
 const {instance}=await WebAssembly.instantiate(wasm,{env:{uart_write:byte=>{if(byte!==13)text+=String.fromCharCode(byte);if(text.length>150000)text=text.slice(-120000);}}});cpu=instance.exports;cpu._initialize?.();
 for(const id of ['run','pause','send'])document.querySelector('#'+id).disabled=false;
 document.querySelector('#run').onclick=()=>{try{reset(true);}catch(error){status.textContent=String(error);status.textContent+=' — Run make serve-native32-simulator and open http://localhost:8080/. For offline use, open build/native32-simulator.html made by make native32-demo.';}};
 document.querySelector('#profile').onchange=()=>reset(true);
 document.querySelector('#pause').onclick=()=>{if(active){active=false;document.querySelector('#pause').textContent='Resume CPU';}else wake();};
 document.querySelector('#input').onsubmit=event=>{event.preventDefault();const input=document.querySelector('#command');try{send(input.value+'\n');input.value='';}catch(error){status.textContent=String(error);status.textContent+=' — Run make serve-native32-simulator and open http://localhost:8080/. For offline use, open build/native32-simulator.html made by make native32-demo.';}};
 reset(true);
}catch(error){status.textContent=String(error);status.textContent+=' — Run make serve-native32-simulator and open http://localhost:8080/. For offline use, open build/native32-simulator.html made by make native32-demo.';}
