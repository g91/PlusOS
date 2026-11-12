// pvdisk.js - emulator side of the paravirtual disk driver (rom_patches/pvdisk.s).
//
// patchSonyDriver() replaces the ROM's .Sony DRVR with PVDISK_DRIVER. The 68k
// driver forwards Open/Prime/Control/Status to PVDisk through a memory-mapped
// port (see the register list in pvdisk.s); PVDisk reads the parameter block
// and the DCE from guest memory and moves the data.

// Assembled from rom_patches/pvdisk.s (m68k-linux-gnu-as -m68000). 196 bytes.
const PVDISK_HEX =
  '6f00001e00000000001c007e00820086007a052e536f6e7900f4000048e71830247afff62649248825490004700125400008203c00000310' +
  'a71e21c8013441e8011843fa003421490006a058760025430010282a001467187018a71e25480018588820044840302b0018a04e528360de' +
  '4cdf0c1870004e754e7570e84e757002600670036002700448e700e0247aff8a24882549000425400008202a00246708307c0007a02f60f2' +
  '202a000c4aaa001c4cdf0700660c08280001000666042f3808fc4e75';
export const PVDISK_DRIVER = new Uint8Array(PVDISK_HEX.match(/../g).map((h) => parseInt(h, 16)));
const PVBASE_OFFSET = 24;

// Mac OS error codes
const noErr = 0, controlErr = -17, statusErr = -18, wPrErr = -44, paramErr = -50, nsDrvErr = -56, offLinErr = -65;

// Find the .Sony DRVR in a ROM image: an 18-byte driver header followed by the name.
export function findSonyDriver(rom) {
  const name = [5, 0x2E, 0x53, 0x6F, 0x6E, 0x79];
  outer:
  for (let i = 18; i < rom.length - 0x400; i += 2) {
    for (let k = 0; k < 6; k++) if (rom[i + k] !== name[k]) continue outer;
    const h = i - 18;
    const w = (o) => (rom[h + o] << 8) | rom[h + o + 1];
    const flags = w(0);
    if ((flags & 0x0F00) !== 0x0F00 || (flags & 0x00FF) !== 0) continue;       // read/write/control/status enabled
    let okay = true, maxOff = 0;
    for (let o = 8; o < 18; o += 2) { const v = w(o); if (v < 24 || v > 0x4000 || (v & 1)) okay = false; maxOff = Math.max(maxOff, v); }
    if (okay && maxOff >= PVDISK_DRIVER.length) return h;
  }
  return -1;
}

export function fixRomChecksum(rom) {
  let sum = 0;
  for (let i = 4; i < rom.length; i += 2) sum = (sum + ((rom[i] << 8) | rom[i + 1])) >>> 0;
  rom[0] = sum >>> 24; rom[1] = sum >>> 16; rom[2] = sum >>> 8; rom[3] = sum;
}

// Returns the ROM offset of the patched driver, or -1 if the ROM has no recognisable .Sony driver.
export function patchSonyDriver(rom, portAddr) {
  const h = findSonyDriver(rom);
  if (h < 0) return -1;
  rom.set(PVDISK_DRIVER, h);
  const p = h + PVBASE_OFFSET;
  rom[p] = portAddr >>> 24; rom[p + 1] = portAddr >>> 16; rom[p + 2] = portAddr >>> 8; rom[p + 3] = portAddr;
  fixRomChecksum(rom);
  return h;
}

export class PVDisk {
  // mem: { r8, r16, r32, w8, w16, w32, copyIn(addr, Uint8Array), copyOut(addr, len) -> Uint8Array }
  constructor(mem) {
    this.mem = mem;
    this.drives = [];           // { num, data: Uint8Array (volume bytes), readOnly, present, dqe, image }
    this.pb = 0; this.dce = 0; this.result = 0; this.index = 0; this.rtsFlag = 0;
    this.events = [];
    this.opened = false;
    this.emptyDQE = 0;
    this.onActivity = null;     // (driveNum, isWrite) => void
    this.stats = { reads: 0, writes: 0, bytesRead: 0, bytesWritten: 0 };
  }

  // Attach a volume: `data` is the byte range of the file system (a subarray of the image).
  addDrive(data, opts = {}) {
    const d = { num: this.drives.length + 1, data, readOnly: !!opts.readOnly, present: true, dqe: 0, image: opts.image || null, name: opts.name || '' };
    this.drives.push(d);
    return d;
  }
  clearDrives() { this.drives.length = 0; }
  driveByNum(n) { return this.drives.find((d) => d.num === n) || null; }
  reset() { this.opened = false; this.events.length = 0; this.emptyDQE = 0; for (const d of this.drives) d.dqe = 0; }

  // The drive list presented to the driver: at least one (empty) drive so the ROM shows the "?" disk.
  list() { return this.drives.length ? this.drives : [{ num: 1, data: null, present: false, readOnly: false, dqe: 0 }]; }

  readReg(off) {
    switch (off) {
      case 12: return this.result >>> 0;
      case 20: { const d = this.list()[this.index]; return d ? d.num : 0; }
      case 28: return this.rtsFlag;
      case 32: return 0x5056444B;
      case 36: return this.events.length ? this.events.shift() : 0;
      default: return 0;
    }
  }
  writeReg(off, v) {
    switch (off) {
      case 0: this.pb = v >>> 0; break;
      case 4: this.dce = v >>> 0; break;
      case 8: this.rtsFlag = 0; this.result = this.exec(v); break;
      case 16: this.index = v >>> 0; break;
      case 24: this.fillDQE(v >>> 0); break;
    }
  }

  fillDQE(addr) {
    const d = this.list()[this.index], m = this.mem;
    if (!d) return;
    if (this.drives.length === 0) this.emptyDQE = addr; else d.dqe = addr;
    const blocks = d.data ? Math.floor(d.data.length / 512) : 0;
    m.w8(addr + 0, d.readOnly ? 0x80 : 0);          // writeProt
    m.w8(addr + 1, d.present ? 8 : 0);              // diskInPlace: 8 = non-ejectable disk
    m.w8(addr + 2, 1);                              // installed
    m.w8(addr + 3, 0);                              // sides
    m.w16(addr + 4 + 4, 1);                         // qType 1: dQDrvSz2 holds the high word of the size
    m.w16(addr + 4 + 12, blocks & 0xFFFF);          // dQDrvSz
    m.w16(addr + 4 + 14, (blocks >>> 16) & 0xFFFF); // dQDrvSz2
  }

  exec(cmd) {
    const m = this.mem, pb = this.pb, dce = this.dce;
    switch (cmd) {
      case 1: this.opened = true; return noErr;
      case 2: {
        const trap = m.r16(pb + 6) & 0xFF, d = this.driveByNum(m.r16(pb + 22));
        m.w32(pb + 40, 0);
        if (!d) return this.drives.length ? nsDrvErr : offLinErr;
        if (!d.present) return offLinErr;
        const pos = m.r32(dce + 16), cnt = m.r32(pb + 36), buf = m.r32(pb + 32);
        if (pos + cnt > d.data.length) return paramErr;
        if (trap === 2) {
          if (!(m.r16(pb + 44) & 0x40)) m.copyIn(buf, d.data.subarray(pos, pos + cnt));        // bit 6 = read-verify
          this.stats.reads++; this.stats.bytesRead += cnt;
        } else if (trap === 3) {
          if (d.readOnly) return wPrErr;
          d.data.set(m.copyOut(buf, cnt), pos);
          if (d.image) d.image.dirty = true;
          this.stats.writes++; this.stats.bytesWritten += cnt;
        } else return paramErr;
        if (this.onActivity) this.onActivity(d.num, trap === 3);
        m.w32(pb + 40, cnt);                         // ioActCount
        m.w32(dce + 16, pos + cnt);                  // dCtlPosition
        return noErr;
      }
      case 3: {
        const cs = m.r16(pb + 26);
        switch (cs) {
          case 1: this.rtsFlag = 1; return noErr;    // KillIO
          case 5: case 6: case 8: case 9: case 65: return noErr;     // verify, format, tag buffer, track cache, accRun
          case 7: return noErr;                      // eject: volumes are fixed disks, nothing to do
          default: return controlErr;
        }
      }
      case 4: {
        if (m.r16(pb + 26) !== 8) return statusErr;  // drvStsCode
        const d = this.driveByNum(m.r16(pb + 22));
        const dqe = d ? d.dqe : this.emptyDQE;
        if (!dqe) return nsDrvErr;
        const out = pb + 28;
        m.w16(out, 0);                               // current track
        for (let i = 0; i < 16; i++) m.w8(out + 2 + i, m.r8(dqe + i));     // flags + qLink..dQFSID
        m.w32(out + 18, 0);                          // twoSideFmt, needsFlush, diskErrs
        return noErr;
      }
      default: return paramErr;
    }
  }
}
