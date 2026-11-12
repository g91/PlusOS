// compact.js - the compact Macintosh family: Plus, SE, Classic.
//
// The machine object is also the CPU's bus. Everything runs in one thread and
// is free of DOM access; the front end (browser or Node harness) calls
// runFrame() and reads the frame buffer / sound samples.

import { M68K } from '../cpu/m68k.js';
import { VIA6522 } from '../devices/via.js';
import { RTC } from '../devices/rtc.js';
import { SCC8530 } from '../devices/scc.js';
import { IWM } from '../devices/iwm.js';
import { NCR5380, SCSIDisk } from '../devices/scsi.js';
import { PlusKeyboard } from '../devices/kbd_plus.js';
import { ADBTransceiver } from '../devices/adb.js';
import { PVDisk, patchSonyDriver } from '../devices/pvdisk.js';
import { parseImage } from '../disk/image.js';

export const LINES_PER_FRAME = 370;
export const CYCLES_PER_LINE = 352;            // 7.8336 MHz / 22.255 kHz
export const VISIBLE_LINES = 342;
const PV_PORT = 0xF40000;

export class CompactMac {
  // def: entry of MACHINES; opts: { rom: Uint8Array, ramMB, cpu, pvDisk (default true), pram: Uint8Array }
  constructor(def, opts) {
    this.def = def;
    this.isSE = def.input === 'adb';
    this.cyclesPerSecond = def.clock;
    this.cyclesPerFrame = LINES_PER_FRAME * CYCLES_PER_LINE;
    const ramMB = opts.ramMB || def.ramDefault;
    this.ramSize = Math.round(ramMB * 1024 * 1024);
    if (this.ramSize & (this.ramSize - 1)) throw new Error('RAM size must be a power of two');
    this.ramMask = this.ramSize - 1;
    this.ram = new Uint8Array(this.ramSize + 8);
    if (!opts.rom) throw new Error('no ROM image');
    let romLen = 1; while (romLen < opts.rom.length) romLen <<= 1;
    this.romMask = romLen - 1;
    // The ROM is decoded in $400000-$43FFFF (mirrored inside); above that the bus floats. The Plus ROM
    // relies on this: it detects SCSI hardware by comparing $420000 with $440000.
    this.romEnd = 0x400000 + Math.max(0x40000, romLen);
    this.rom = new Uint8Array(romLen + 8);
    this.rom.set(opts.rom);
    this.romPatched = -1;
    this.fastBoot = !!opts.fastBoot;          // skip the ROM's RAM test (Plus: mouse button held at power-on; SE: warm-start flag)
    this.usePV = opts.pvDisk !== false;
    if (this.usePV) this.romPatched = patchSonyDriver(this.rom.subarray(0, romLen), PV_PORT);

    this.overlay = true;
    this.events = [];
    this.frameStart = 0; this.line = 0; this.frames = 0; this.bootFrame = 0;
    this.nextSecond = this.cyclesPerSecond;
    this.irqVia = false; this.irqScc = false; this.nmiSwitch = false;
    this.portA = 0xFF; this.portB = 0xFF;
    this.mouseDX = 0; this.mouseDY = 0; this.mouseDown = false; this.x2 = 0; this.y2 = 0; this.mouseTimer = false; this.absWait = 0;
    this.sound = new Uint8Array(LINES_PER_FRAME);
    this.soundOn = false; this.soundVolume = 7;
    this.images = [];                 // attached disk images { bytes, name, info, mode, dirty }
    this.onPramWrite = null;

    this.via = new VIA6522({
      readA: () => this.viaReadA(), readB: () => this.viaReadB(),
      writeA: (v) => this.viaWriteA(v), writeB: (v) => this.viaWriteB(v),
      srWrite: (v) => this.input.srWrite(v),
      irq: (s) => { this.irqVia = s; this.updateIPL(); },
      timersChanged: () => { const t = this.via.nextEvent(); if (t < this.cpu.endCycles) this.cpu.endCycles = t; },
    });
    this.rtc = new RTC();
    if (opts.pram) this.rtc.pram.set(opts.pram.subarray(0, 256));
    this.rtc.onPramWrite = () => { if (this.onPramWrite) this.onPramWrite(this.rtc.pram); };
    this.scc = new SCC8530({ irq: (s) => { this.irqScc = s; this.updateIPL(); } });
    this.iwm = new IWM();
    this.scsi = new NCR5380({ irq: (s) => this.scsiIRQ(s) });
    this.pv = new PVDisk({
      r8: (a) => this.read8(a), r16: (a) => this.read16(a), r32: (a) => this.read32(a) >>> 0,
      w8: (a, v) => this.write8(a, v), w16: (a, v) => this.write16(a, v), w32: (a, v) => this.write32(a, v),
      copyIn: (a, bytes) => { a &= this.ramMask; if (a + bytes.length <= this.ramSize) this.ram.set(bytes, a); else for (let i = 0; i < bytes.length; i++) this.ram[(a + i) & this.ramMask] = bytes[i]; },
      copyOut: (a, n) => { a &= this.ramMask; if (a + n <= this.ramSize) return this.ram.slice(a, a + n); const o = new Uint8Array(n); for (let i = 0; i < n; i++) o[i] = this.ram[(a + i) & this.ramMask]; return o; },
    });
    this.cpu = new M68K(this, opts.cpu || def.cpu);
    this.input = this.isSE ? new ADBTransceiver(this, this.via) : new PlusKeyboard(this, this.via);
    this.reset();
  }

  // ---- disks -------------------------------------------------------------------------
  // mode: 'pv' (paravirtual driver, default), 'scsi' (NCR 5380 target, needs a driver on the image), 'auto'
  attachDisk(bytes, name = 'disk', mode = 'auto', readOnly = false) {
    const info = parseImage(bytes);
    if (mode === 'auto') mode = this.usePV && this.romPatched >= 0 ? 'pv' : 'scsi';
    const image = { bytes, name, info, mode, dirty: false, readOnly };
    this.images.push(image);
    if (mode === 'scsi') {
      const used = this.scsi.targets.filter(Boolean).length;
      const id = [0, 1, 2, 3, 4, 5, 6][used];
      if (id !== undefined) this.scsi.attach(id, new SCSIDisk(bytes, { readOnly, onWrite: () => { image.dirty = true; } }));
      image.scsiId = id;
    } else {
      for (const v of info.volumes) this.pv.addDrive(bytes.subarray(v.offset, v.offset + v.length), { readOnly, image, name: v.name });
    }
    return image;
  }
  detachDisks() { this.images.length = 0; this.pv.clearDrives(); this.scsi.targets.fill(null); }

  // ---- time / scheduling ------------------------------------------------------------------
  now() { return this.cpu.cycles; }
  schedule(delay, fn) {
    const t = this.cpu.cycles + delay, ev = this.events;
    let i = ev.length;
    while (i > 0 && ev[i - 1].t > t) i--;
    ev.splice(i, 0, { t, fn });
    if (t < this.cpu.endCycles) this.cpu.endCycles = t;
  }

  reset() {
    this.overlay = true;
    this.events.length = 0;
    this.via.reset(this.cpu.cycles);
    this.scc.reset(); this.iwm.reset(); this.scsi.reset(); this.pv.reset();
    this.rtc.resetSerial();
    this.input.reset();
    this.irqVia = this.irqScc = false; this.nmiSwitch = false;
    this.mouseDX = this.mouseDY = 0; this.mouseTimer = false;
    this.bootFrame = this.frames;
    if (this.fastBoot && this.isSE) this.ram.set([0x57, 0x4C, 0x53, 0x43], 0xCFC);      // WarmStart = 'WLSC'
    this.cpu.hardReset();
    this.updateIPL();
  }
  resetDevices() { this.via.reset(this.cpu.cycles); this.scc.reset(); this.iwm.reset(); this.scsi.reset(); }     // RESET instruction

  updateIPL() {
    // The interrupt encoder gives the SCC priority: VIA + SCC together read as level 2, not 3
    // (the ROM's level-3 handler is a bare RTE).
    this.cpu.setIPL((this.nmiSwitch ? 4 : 0) | (this.irqScc ? 2 : this.irqVia ? 1 : 0));
  }
  // Programmer's interrupt switch.
  interruptButton() { this.nmiSwitch = true; this.updateIPL(); this.schedule(20000, () => { this.nmiSwitch = false; this.updateIPL(); }); }

  scsiIRQ(s) {
    if (!this.isSE) return;                       // not connected on the Plus
    this.scsiIrqLine = s;
    if (s && !(this.portB & 0x40)) this.via.pulseCB2();      // SE: SCSI IRQ on CB2, enabled while PB6 is low
  }

  // Run one video frame (370 lines). Returns nothing; the frame buffer is read with screen().
  runFrame() {
    const cpu = this.cpu, via = this.via, ev = this.events;
    for (let line = 0; line < LINES_PER_FRAME; line++) {
      this.line = line;
      const lineEnd = this.frameStart + (line + 1) * CYCLES_PER_LINE;
      while (cpu.cycles < lineEnd) {
        let target = lineEnd;
        const ve = via.nextEvent();
        if (ve < target) target = ve;
        if (ev.length && ev[0].t < target) target = ev[0].t;
        if (target > cpu.cycles) cpu.run(target - cpu.cycles);
        const now = cpu.cycles;
        via.sync(now);
        while (ev.length && ev[0].t <= now) ev.shift().fn();
      }
      this.sound[line] = this.ram[this.soundBase() + line * 2];
      if (line === VISIBLE_LINES - 1) via.pulseCA1();               // vertical blanking interrupt
    }
    this.frameStart += this.cyclesPerFrame;
    this.frames++;
    while (cpu.cycles >= this.nextSecond) { this.nextSecond += this.cyclesPerSecond; this.rtc.tick(); via.pulseCA2(); }
  }

  // ---- video / sound ---------------------------------------------------------------------
  screenBase() { return this.ramSize - ((this.portA & 0x40) || this.isSE ? 0x5900 : 0x5900 + 0x8000); }
  soundBase() { return this.ramSize - ((this.portA & 0x08) ? 0x300 : 0x300 + 0x5C00); }
  // 1 bit per pixel, 64 bytes per row, 342 rows, MSB = leftmost, 1 = black
  screen() { const b = this.screenBase(); return this.ram.subarray(b, b + 64 * VISIBLE_LINES); }

  // ---- VIA wiring ---------------------------------------------------------------------------
  viaReadA() { return 0xFF; }                                        // PA7 = SCC W/REQ (idle high)
  viaReadB() {
    let v = 0xFF;
    if (!this.rtc.dataBit()) v &= ~0x01;
    if (this.isSE) v = this.input.readPB(v);
    else {
      if (this.mouseDown || (this.fastBoot && this.frames < this.bootFrame + 3)) v &= ~0x08;
      if (!this.x2) v &= ~0x10;
      if (!this.y2) v &= ~0x20;
      // PB6 = H4: low during the visible part of a scan line
      if ((this.cpu.cycles - this.frameStart) % CYCLES_PER_LINE < 256) v &= ~0x40;
    }
    return v;
  }
  viaWriteA(pins) {
    this.portA = pins;
    if (!this.isSE) this.overlay = (pins & 0x10) !== 0;
    this.soundVolume = pins & 7;
  }
  viaWriteB(pins) {
    const old = this.portB;
    this.portB = pins;
    this.rtc.update(pins);
    this.soundOn = !(pins & 0x80);
    if (this.isSE) this.input.writePB(pins, old);
  }

  // ---- input ---------------------------------------------------------------------------------
  keyDown(code) { this.input.keyDown(code); }
  keyUp(code) { this.input.keyUp(code); }
  mouseButton(down) { this.mouseDown = !!down; if (this.isSE) this.input.mouseButton(this.mouseDown); }
  mouseMove(dx, dy) {
    if (this.isSE) { this.input.mouseMove(dx, dy); return; }
    this.mouseDX += dx; this.mouseDY += dy;
    const LIM = 2000;
    this.mouseDX = Math.max(-LIM, Math.min(LIM, this.mouseDX));
    this.mouseDY = Math.max(-LIM, Math.min(LIM, this.mouseDY));
    this.kickMouse();
  }
  kickMouse() { if (!this.mouseTimer) { this.mouseTimer = true; this.schedule(600, () => this.mouseStep()); } }
  // Plus mouse: one quadrature edge per step. X1 on SCC channel A DCD, X2 on PB4; Y1 on channel B DCD, Y2 on PB5.
  mouseStep() {
    this.mouseTimer = false;
    const scc = this.scc;
    if (this.mouseDX !== 0 && scc.canChangeDCD(0)) {
      const x1 = scc.ch[0].dcd ^ 1, right = this.mouseDX > 0;
      this.x2 = right ? x1 ^ 1 : x1;               // ROM: X1 == X2 -> left
      scc.setDCD(0, x1);
      this.mouseDX += right ? -1 : 1;
    }
    if (this.mouseDY !== 0 && scc.canChangeDCD(1)) {
      const y1 = scc.ch[1].dcd ^ 1, down = this.mouseDY > 0;
      this.y2 = down ? y1 : y1 ^ 1;                // ROM: Y1 == Y2 -> down
      scc.setDCD(1, y1);
      this.mouseDY += down ? -1 : 1;
    }
    if (this.mouseDX !== 0 || this.mouseDY !== 0) this.kickMouse();
  }
  // Absolute pointing: steer the guest cursor (low-memory RawMouse/MTemp) towards a screen position.
  mouseTo(x, y) {
    // Absolute pointing cannot be expressed with a relative mouse, so this is the one place where
    // the emulator writes guest memory: MTemp ($828) and RawMouse ($82C) get the new position and
    // CrsrNew ($8CE) is set from CrsrCouple ($8CF), which is what the ROM's own mouse handlers do.
    if (this.overlay) return;
    const r = this.ram;
    x = Math.max(0, Math.min(511, x | 0)); y = Math.max(0, Math.min(341, y | 0));
    if (((r[0x832] << 8) | r[0x833]) === x && ((r[0x830] << 8) | r[0x831]) === y) return;     // Mouse already there
    for (const a of [0x828, 0x82C]) { r[a] = y >> 8; r[a + 1] = y; r[a + 2] = x >> 8; r[a + 3] = x; }
    // leave RawMouse one pixel short so the cursor task sees a (unscaled) movement and updates Mouse
    const x0 = x > 0 ? x - 1 : 1;
    r[0x82E] = x0 >> 8; r[0x82F] = x0;
    r[0x8CE] = r[0x8CF];
  }

  // ---- bus -------------------------------------------------------------------------------------
  read8(a) {
    a &= 0xFFFFFF;
    if (a < 0x400000) return this.overlay ? this.rom[a & this.romMask] : this.ram[a & this.ramMask];
    if (a < this.romEnd) { if (this.overlay && this.isSE) this.overlay = false; return this.rom[a & this.romMask]; }
    if (a < 0x580000) return 0xFF;
    return this.ioRead8(a);
  }
  read16(a) {
    a &= 0xFFFFFF;
    if (a < 0x400000) {
      if (!this.overlay) { a &= this.ramMask; return (this.ram[a] << 8) | this.ram[a + 1]; }
      a &= this.romMask; return (this.rom[a] << 8) | this.rom[a + 1];
    }
    if (a < this.romEnd) { if (this.overlay && this.isSE) this.overlay = false; a &= this.romMask; return (this.rom[a] << 8) | this.rom[a + 1]; }
    if (a < 0x580000) return 0xFFFF;
    return this.ioRead16(a);
  }
  read32(a) {
    a &= 0xFFFFFF;
    if (a < 0x3FFFFE && !this.overlay) { a &= this.ramMask; const r = this.ram; return ((r[a] << 24) | (r[a + 1] << 16) | (r[a + 2] << 8) | r[a + 3]) >>> 0; }
    if (a >= PV_PORT && a < PV_PORT + 0x40) return this.pv.readReg(a - PV_PORT);
    return ((this.read16(a) << 16) | this.read16(a + 2)) >>> 0;
  }
  write8(a, v) {
    a &= 0xFFFFFF;
    if (a < 0x400000) { if (!this.overlay) this.ram[a & this.ramMask] = v; return; }
    if (a < 0x580000) return;
    this.ioWrite8(a, v);
  }
  write16(a, v) {
    a &= 0xFFFFFF;
    if (a < 0x400000) { if (!this.overlay) { a &= this.ramMask; this.ram[a] = v >> 8; this.ram[a + 1] = v; } return; }
    if (a < 0x580000) return;
    this.ioWrite16(a, v);
  }
  write32(a, v) {
    a &= 0xFFFFFF;
    if (a < 0x3FFFFE && !this.overlay) { a &= this.ramMask; const r = this.ram; r[a] = v >>> 24; r[a + 1] = v >>> 16; r[a + 2] = v >>> 8; r[a + 3] = v; return; }
    if (a >= PV_PORT && a < PV_PORT + 0x40) { this.pv.writeReg(a - PV_PORT, v); return; }
    this.write16(a, (v >>> 16) & 0xFFFF); this.write16(a + 2, v & 0xFFFF);
  }

  ioRead8(a) {
    if (a < 0x600000) return this.scsi.read((a >> 4) & 7, (a & 0x200) !== 0);
    if (a < 0x800000) return this.overlay ? this.ram[a & this.ramMask] : 0xFF;
    if (a < 0xA00000) return (a & 1) ? 0xFF : this.scc.read((a >> 1) & 3);
    if (a < 0xC00000) return 0xFF;
    if (a < 0xE00000) return this.iwm.access((a >> 9) & 15, false, 0);
    if (a >= 0xE80000 && a < 0xF00000) return this.via.read((a >> 9) & 15, this.cpu.cycles);
    if (a >= 0xF00000 && a < 0xF80000) return 0x00;                  // phase read
    return 0xFF;
  }
  ioWrite8(a, v) {
    if (a < 0x600000) { this.scsi.write((a >> 4) & 7, v, (a & 0x200) !== 0); return; }
    if (a < 0x800000) { if (this.overlay) this.ram[a & this.ramMask] = v; return; }
    if (a < 0xA00000) return;
    if (a < 0xC00000) { this.scc.write((a >> 1) & 3, v); return; }
    if (a < 0xE00000) { this.iwm.access((a >> 9) & 15, true, v); return; }
    if (a >= 0xE80000 && a < 0xF00000) this.via.write((a >> 9) & 15, v, this.cpu.cycles);
  }
  // Devices sit on one byte lane; a word access touches the device once.
  ioRead16(a) {
    if (a >= 0x600000 && a < 0x800000) return (this.ioRead8(a) << 8) | this.ioRead8(a + 1);
    if (a >= 0xC00000 && a < 0xE00000) return 0xFF00 | this.ioRead8(a + 1);        // IWM: low byte
    if (a >= 0xF00000) return (this.ioRead8(a) << 8) | this.ioRead8(a + 1);
    return (this.ioRead8(a) << 8) | 0xFF;
  }
  ioWrite16(a, v) {
    if (a >= 0x600000 && a < 0x800000) { this.ioWrite8(a, v >> 8); this.ioWrite8(a + 1, v & 0xFF); return; }
    if (a >= 0xE80000 && a < 0xF00000) { this.ioWrite8(a, v >> 8); return; }        // VIA: high byte
    this.ioWrite8(a + 1, v & 0xFF);                                               // SCC write, IWM, SCSI write: low byte
  }
}
