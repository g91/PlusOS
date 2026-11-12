// index.js - machine factory.
import { MACHINES, MACHINE_ORDER, identifyRom, romChecksum } from './table.js';
import { CompactMac } from './compact.js';
import { CPU_MODELS } from '../cpu/m68k.js';

export { MACHINES, MACHINE_ORDER, identifyRom, romChecksum, CPU_MODELS };

// opts: { machine: 'plus'|'se'|'classic', rom: Uint8Array, ramMB, cpu, pvDisk, pram }
export function createMachine(opts) {
  const def = MACHINES[opts.machine || 'plus'];
  if (!def) throw new Error('unknown machine: ' + opts.machine);
  if (def.family !== 'compact' || def.status === 'planned') throw new Error(def.name + ' is not emulated yet (status: ' + def.status + ')');
  if (opts.cpu && !CPU_MODELS[opts.cpu]) throw new Error('unknown CPU model: ' + opts.cpu);
  return new CompactMac(def, opts);
}

// Convert the 1-bit screen to RGBA (black on white). `out` must hold width*height*4 bytes.
export function screenToRGBA(screen, out, width = 512, height = 342) {
  const out32 = new Uint32Array(out.buffer, out.byteOffset, width * height);
  const BLACK = 0xFF000000, WHITE = 0xFFFFFFFF;
  let p = 0;
  const rowBytes = width >> 3;
  for (let i = 0, n = rowBytes * height; i < n; i++) {
    const b = screen[i];
    out32[p++] = b & 0x80 ? BLACK : WHITE; out32[p++] = b & 0x40 ? BLACK : WHITE;
    out32[p++] = b & 0x20 ? BLACK : WHITE; out32[p++] = b & 0x10 ? BLACK : WHITE;
    out32[p++] = b & 0x08 ? BLACK : WHITE; out32[p++] = b & 0x04 ? BLACK : WHITE;
    out32[p++] = b & 0x02 ? BLACK : WHITE; out32[p++] = b & 0x01 ? BLACK : WHITE;
  }
}
