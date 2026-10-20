/** Actual RV64 ELF running inside the C RISC-V CPU compiled to WebAssembly. */
function symbol(elf, wanted) {
  const view = new DataView(elf.buffer, elf.byteOffset, elf.byteLength), decoder = new TextDecoder();
  const integer = offset => Number(view.getBigUint64(offset, true));
  const table = integer(40), stride = view.getUint16(58, true), count = view.getUint16(60, true);
  for (let section = 0; section < count; section++) {
    const header = table + section * stride;
    if (view.getUint32(header + 4, true) !== 2) continue;
    const start = integer(header + 24), size = integer(header + 32), entry = integer(header + 56);
    const stringHeader = table + view.getUint32(header + 40, true) * stride;
    const strings = integer(stringHeader + 24), stringSize = integer(stringHeader + 32);
    if (entry < 24 || start + size > elf.length || strings + stringSize > elf.length) throw new Error('Invalid ELF symbol table.');
    for (let position = start; position + 24 <= start + size; position += entry) {
      const nameStart = strings + view.getUint32(position, true);
      if (nameStart >= strings + stringSize) throw new Error('Invalid ELF symbol name.');
      const nameEnd = elf.indexOf(0, nameStart);
      if (nameEnd < 0 || nameEnd >= strings + stringSize) throw new Error('Invalid ELF symbol string.');
      if (decoder.decode(elf.subarray(nameStart, nameEnd)) === wanted) return {address:integer(position + 8), size:integer(position + 16)};
    }
  }
  return null;
}
export async function createRiscvMachine(wasmBytes, elfBytes, write = () => {}, options = {}) {
  if (!Number.isInteger(options.base ?? 0) || (options.base ?? 0) < 0 || (options.base ?? 0) >= 0x2000000 || (options.base ?? 0) % 4096) throw new Error('Invalid PIE load base.');
  let instance, pending = '';
  const decoder = new TextDecoder(), elf = new Uint8Array(elfBytes);
  // ET_REL symbols are section-relative, not virtual addresses plus a bias.
  // The generic CPU can run an object, but BASIC image controls need a future
  // resolved-symbol API. Never guess an object's mem address using PIE rules.
  const isObject = elf[16] === 1 && elf[17] === 0;
  const z80 = isObject ? null : symbol(elf, 'mem');
  const wasi = {
    args_get(){return 0;},args_sizes_get(count,size){const view=new DataView(instance.exports.memory.buffer);view.setUint32(count,0,true);view.setUint32(size,0,true);return 0;},
    fd_close(){return 0;},fd_seek(){return 29;},
    proc_exit(code){throw new Error(`Emulator exited with status ${code}`);},
    fd_write(fd,vectors,count,written){const view=new DataView(instance.exports.memory.buffer);let total=0;for(let index=0;index<count;index++){const pointer=view.getUint32(vectors+index*8,true),length=view.getUint32(vectors+index*8+4,true);write(decoder.decode(new Uint8Array(view.buffer,pointer,length)));total+=length;}view.setUint32(written,total,true);return 0;},
  };
  instance=(await WebAssembly.instantiate(wasmBytes,{
    env:{write_bytes(fd,pointer,length){write(decoder.decode(new Uint8Array(instance.exports.memory.buffer,pointer,length)));}},
    wasi_snapshot_preview1:wasi,
  })).instance;
  const api=instance.exports;
  if (api._initialize) api._initialize();
  function fault(){const bytes=new Uint8Array(api.memory.buffer),start=api.rv_fault_pointer();return new Error(decoder.decode(bytes.subarray(start,bytes.indexOf(0,start))));}
  function memory(){
    if(!z80||z80.size!==65536||z80.address+api.rv_load_bias()+65536>api.rv_memory_size())throw new Error('This ELF has no known BASIC memory symbol.');
    return new DataView(api.memory.buffer,api.rv_memory_pointer()+api.rv_load_bias()+z80.address,65536);
  }
  function validate(image){
    if(image.length<2||image.length>0xad17)throw new Error('Invalid BASIC image length.');
    let offset=0,previous=-1;
    for(;;){
      if(offset+2>image.length)throw new Error('Truncated program link.');
      const next=image[offset]|image[offset+1]<<8;
      if(!next){if(offset+2!==image.length)throw new Error('Bytes after final sentinel.');return;}
      const end=next-0x42e9,line=image[offset+2]|image[offset+3]<<8;
      if(offset+5>image.length||end<offset+5||end>=image.length||image[end-1]||line<=previous||line>65529)throw new Error('Invalid BASIC links or line numbers.');
      for(let pos=offset+4;pos+1<end;pos++)if(!image[pos])throw new Error('Embedded line terminator.');
      previous=line;offset=end;
    }
  }
  let lastState=0;
  return {
    reset(){pending='';if(elf.length>api.rv_image_limit())throw new Error('ELF exceeds emulator upload limit.');new Uint8Array(api.memory.buffer,api.rv_image_pointer(),elf.length).set(elf);if(!api.rv_boot_at(elf.length, options.base ?? 0))throw fault();lastState=0;},
    input(text){if(/[^\x00-\x7f]/.test(text))throw new Error('Input must be ASCII.');pending+=text;},
    runSlice(budget=100000){let count=0;while(count<pending.length&&api.rv_enqueue(pending.charCodeAt(count)))count++;pending=pending.slice(count);lastState=api.rv_run(budget);if(lastState===3)throw fault();if(lastState===2&&api.rv_exit_status())throw new Error(`RISC-V guest exited with status ${api.rv_exit_status()}`);return lastState===1&&pending.length?0:lastState;},
    steps(){return api.rv_steps();},
    registers(){return Array.from({length:32},(_,index)=>api.rv_register(index));},
    pc(){return api.rv_program_counter();},
    save(){const view=memory(),length=view.getUint16(0x40f9,true)-0x42e9;if(length<2||length>0xad17)throw new Error('Invalid BASIC program extent.');return new Uint8Array(view.buffer,view.byteOffset+0x42e9,length).slice();},
    load(image){if(lastState!==1)throw new Error('Wait for BASIC input before uploading.');validate(image);const view=memory();if(view.getUint16(0x40a4,true)!==0x42e9)throw new Error('BASIC has not initialized.');new Uint8Array(view.buffer,view.byteOffset+0x42e9,image.length).set(image);for(const pointer of [0x40f9,0x40fb,0x40fd])view.setUint16(pointer,0x42e9+image.length,true);},
  };
}
