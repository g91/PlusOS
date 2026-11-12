// m68k.js - Motorola 680x0 CPU core (68000 / 68010 / 68020 / 68030 / 68040 and EC/LC variants).
// Original implementation written for the PlusOS web emulator.
//
// The core is bus-agnostic. It talks to a `bus` object:
//     read8(a) read16(a) read32(a)  -> unsigned values (read32 may be signed, the core normalises)
//     write8(a,v) write16(a,v) write32(a,v)
//     iack(level)      optional, returns a vector number or -1 for autovector
//     resetDevices()   optional, called by the RESET instruction
// Addresses handed to the bus are 32-bit unsigned; the bus masks them to its width.
// A bus signals a bus error by calling cpu.busError(addr, isWrite) (which throws).
//
// Cycle accounting: every 16-bit bus access costs 4 clocks (as on a 68000) and
// handlers add the internal clocks of the instruction, which lands close to the
// documented 68000 timings without a table per opcode.

import { buildTable } from './ops.js';

export const MASK = [0xFF, 0xFFFF, 0xFFFFFFFF];
export const MSB = [0x80, 0x8000, 0x80000000];
export const BYTES = [1, 2, 4];

// level: 0=68000 1=68010 2=68020 3=68030 4=68040
export const CPU_MODELS = {
  '68000':   { level: 0, fpu: false, mmu: false, addrBits: 24 },
  '68010':   { level: 1, fpu: false, mmu: false, addrBits: 24 },
  '68EC020': { level: 2, fpu: false, mmu: false, addrBits: 24 },
  '68020':   { level: 2, fpu: true,  mmu: false, addrBits: 32 },   // with 68881/68882
  '68030':   { level: 3, fpu: true,  mmu: true,  addrBits: 32 },   // with 68882
  '68EC030': { level: 3, fpu: true,  mmu: false, addrBits: 32 },
  '68040':   { level: 4, fpu: true,  mmu: true,  addrBits: 32 },
  '68LC040': { level: 4, fpu: false, mmu: true,  addrBits: 32 },
  '68EC040': { level: 4, fpu: false, mmu: false, addrBits: 32 },
};

const FAULT = { fault: true };          // sentinel thrown for group-0 exceptions
// Stack frame sizes by format (010+), used by RTE.
const FRAME_SIZE = { 0: 8, 1: 8, 2: 12, 3: 12, 4: 16, 7: 60, 8: 58, 9: 20, 0xA: 32, 0xB: 92 };

export class M68K {
  constructor(bus, model = '68000') {
    this.bus = bus;
    this.d = new Int32Array(16);          // D0-D7, A0-A7
    this.fp = new Float64Array(8);
    this.setModel(model);
    this.onALine = null;                  // optional hook(op) -> true if handled
    this.onFLine = null;                  // optional hook(op) -> true if handled
    this.onException = null;              // optional hook(vector, pc)
    this.insns = 0;
    this.cycles = 0;
    this.endCycles = 0;
    this.hardReset();
  }

  setModel(model) {
    const m = CPU_MODELS[model];
    if (!m) throw new Error('unknown CPU model ' + model);
    this.model = model;
    this.level = m.level;
    this.hasFPU = m.fpu;
    this.hasMMU = m.mmu;
    this.addrBits = m.addrBits;
    this.odd = m.level < 2 ? 1 : 0;       // word access to odd address faults on 000/010
    this.srMask = m.level < 2 ? 0xA71F : 0xF71F;
    this.tbl = buildTable(m.level, m.fpu, m.mmu);
  }

  hardReset() {
    this.d.fill(0);
    this.fp.fill(0);
    this.N = this.Z = this.V = this.C = this.X = 0;
    this.S = 1; this.M = 0; this.T1 = 0; this.T0 = 0; this.IM = 7;
    this.usp = this.isp = this.msp = 0;
    this.vbr = 0; this.sfc = 0; this.dfc = 0; this.cacr = 0; this.caar = 0;
    // MMU registers (030: tc, srp, crp, tt0, tt1, mmusr; 040: tc, urp, srp, itt0/1, dtt0/1, mmusr)
    this.mmu = { tc: 0, srpH: 0, srpL: 0, crpH: 0, crpL: 0, tt0: 0, tt1: 0, mmusr: 0,
                 urp: 0, srp: 0, itt0: 0, itt1: 0, dtt0: 0, dtt1: 0 };
    this.fpcr = 0; this.fpsr = 0; this.fpiar = 0;
    this.ipl = 0; this.nmi = 0;
    this.stopped = 0; this.halted = 0; this.slow = 0;
    this.ppc = 0; this.ir = 0; this.eaA = 0; this.excTaken = 0;
    this.faultVec = 0; this.faultAddr = 0; this.faultWrite = 0; this.faultInsn = 0;
    this.inFault = 0;
    this.reset();
  }

  // RESET exception: load SSP and PC from vector 0/1.
  reset() {
    this.S = 1; this.M = 0; this.T1 = this.T0 = 0; this.IM = 7;
    this.vbr = 0; this.cacr = 0;
    this.stopped = 0; this.halted = 0; this.inFault = 0;
    try {
      this.d[15] = this.bus.read32(0);
      this.pc = this.bus.read32(4) >>> 0;
    } catch (e) { if (e !== FAULT) throw e; this.halted = 1; }
    this.updateSlow();
  }

  // ---- status register -------------------------------------------------
  getCCR() { return (this.X << 4) | (this.N << 3) | (this.Z << 2) | (this.V << 1) | this.C; }
  setCCR(v) { this.X = (v >> 4) & 1; this.N = (v >> 3) & 1; this.Z = (v >> 2) & 1; this.V = (v >> 1) & 1; this.C = v & 1; }
  getSR() {
    return (this.T1 << 15) | (this.T0 << 14) | (this.S << 13) | (this.M << 12) | (this.IM << 8) | this.getCCR();
  }
  setSR(v) {
    v &= this.srMask;
    // save the active stack pointer
    if (this.S) { if (this.M) this.msp = this.d[15]; else this.isp = this.d[15]; } else this.usp = this.d[15];
    this.T1 = (v >> 15) & 1; this.T0 = (v >> 14) & 1;
    this.S = (v >> 13) & 1; this.M = (v >> 12) & 1; this.IM = (v >> 8) & 7;
    this.d[15] = this.S ? (this.M ? this.msp : this.isp) : this.usp;
    this.setCCR(v);
    this.updateSlow();
  }
  updateSlow() {
    this.slow = (this.stopped | this.halted | this.T1 | this.T0 | this.nmi | (this.ipl > this.IM ? 1 : 0)) ? 1 : 0;
  }

  // Interrupt priority level input (0-7). Level 7 is edge triggered (NMI).
  setIPL(level) {
    if (level === 7 && this.ipl !== 7) this.nmi = 1;
    this.ipl = level;
    this.updateSlow();
  }

  // ---- memory access -----------------------------------------------------
  rd8(a) { this.cycles += 4; return this.bus.read8(a); }
  rd16(a) { if ((a & this.odd) !== 0) this.addrError(a, 0, 0); this.cycles += 4; return this.bus.read16(a); }
  rd32(a) { if ((a & this.odd) !== 0) this.addrError(a, 0, 0); this.cycles += 8; return this.bus.read32(a) >>> 0; }
  wr8(a, v) { this.cycles += 4; this.bus.write8(a, v & 0xFF); }
  wr16(a, v) { if ((a & this.odd) !== 0) this.addrError(a, 1, 0); this.cycles += 4; this.bus.write16(a, v & 0xFFFF); }
  wr32(a, v) { if ((a & this.odd) !== 0) this.addrError(a, 1, 0); this.cycles += 8; this.bus.write32(a, v >>> 0); }
  rd(a, sz) { return sz === 0 ? this.rd8(a) : sz === 1 ? this.rd16(a) : this.rd32(a); }
  wr(a, sz, v) { if (sz === 0) this.wr8(a, v); else if (sz === 1) this.wr16(a, v); else this.wr32(a, v); }

  fetch16() { const pc = this.pc; this.pc = pc + 2; this.cycles += 4; return this.bus.read16(pc); }
  fetchS16() { const pc = this.pc; this.pc = pc + 2; this.cycles += 4; return (this.bus.read16(pc) << 16) >> 16; }
  fetch32() { const pc = this.pc; this.pc = pc + 4; this.cycles += 8; return this.bus.read32(pc) >>> 0; }

  push16(v) { const sp = (this.d[15] - 2) | 0; this.d[15] = sp; this.wr16(sp >>> 0, v); }
  push32(v) { const sp = (this.d[15] - 4) | 0; this.d[15] = sp; this.wr32(sp >>> 0, v); }
  pop16() { const sp = this.d[15]; this.d[15] = sp + 2; return this.rd16(sp >>> 0); }
  pop32() { const sp = this.d[15]; this.d[15] = sp + 4; return this.rd32(sp >>> 0); }

  // ---- effective addresses ----------------------------------------------
  // Returns the address of a memory operand (modes 2..7); side effects applied.
  ea(mode, reg, sz) {
    const d = this.d;
    switch (mode) {
      case 2: return d[8 + reg] >>> 0;
      case 3: { const a = d[8 + reg]; d[8 + reg] = a + (sz === 0 ? (reg === 7 ? 2 : 1) : sz === 1 ? 2 : 4); return a >>> 0; }
      case 4: { const a = d[8 + reg] - (sz === 0 ? (reg === 7 ? 2 : 1) : sz === 1 ? 2 : 4); d[8 + reg] = a; this.cycles += 2; return a >>> 0; }
      case 5: return (d[8 + reg] + this.fetchS16()) >>> 0;
      case 6: return this.eaIndex(d[8 + reg]);
      default:
        switch (reg) {
          case 0: return this.fetchS16() >>> 0;
          case 1: return this.fetch32();
          case 2: { const pc = this.pc; return (pc + this.fetchS16()) >>> 0; }
          case 3: return this.eaIndex(this.pc);
          default: {       // immediate: operand lives in the instruction stream
            const pc = this.pc;
            if (sz === 2) { this.pc = pc + 4; return pc; }
            this.pc = pc + 2;
            return sz === 0 ? pc + 1 : pc;
          }
        }
    }
  }

  eaIndex(base) {
    const ext = this.fetch16();
    let xn = this.d[ext >>> 12];
    if ((ext & 0x800) === 0) xn = (xn << 16) >> 16;
    this.cycles += 2;
    if (this.level >= 2) {
      xn <<= (ext >> 9) & 3;
      if (ext & 0x100) return this.eaFull(base, ext, xn);
    }
    return (base + ((ext << 24) >> 24) + xn) >>> 0;
  }

  // 68020+ full extension word format (base/outer displacement, memory indirect).
  eaFull(base, ext, xn) {
    if (ext & 0x80) base = 0;
    if (ext & 0x40) xn = 0;
    let bd = 0;
    switch ((ext >> 4) & 3) { case 2: bd = this.fetchS16(); break; case 3: bd = this.fetch32(); break; }
    const iis = ext & 7;
    if (iis === 0) return (base + bd + xn) >>> 0;
    let od = 0;
    switch (iis & 3) { case 2: od = this.fetchS16(); break; case 3: od = this.fetch32(); break; }
    if (iis & 4) return (this.rd32((base + bd) >>> 0) + xn + od) >>> 0;     // post-indexed
    return (this.rd32((base + bd + xn) >>> 0) + od) >>> 0;                  // pre-indexed
  }

  getD(r, sz) { const v = this.d[r]; return sz === 0 ? v & 0xFF : sz === 1 ? v & 0xFFFF : v >>> 0; }
  setD(r, sz, v) {
    const d = this.d;
    if (sz === 0) d[r] = (d[r] & 0xFFFFFF00) | (v & 0xFF);
    else if (sz === 1) d[r] = (d[r] & 0xFFFF0000) | (v & 0xFFFF);
    else d[r] = v;
  }

  // Read an operand of any mode. Remembers the address for wrBack().
  rdEA(mode, reg, sz) {
    if (mode === 0) { const v = this.d[reg]; return sz === 0 ? v & 0xFF : sz === 1 ? v & 0xFFFF : v >>> 0; }
    if (mode === 1) { const v = this.d[8 + reg]; return sz === 2 ? v >>> 0 : v & 0xFFFF; }
    const a = this.ea(mode, reg, sz);
    this.eaA = a;
    return sz === 0 ? this.rd8(a) : sz === 1 ? this.rd16(a) : this.rd32(a);
  }
  // Write an operand, computing the EA.
  wrEA(mode, reg, sz, v) {
    if (mode === 0) this.setD(reg, sz, v);
    else if (mode === 1) this.d[8 + reg] = sz === 1 ? (v << 16) >> 16 : v;
    else { const a = this.ea(mode, reg, sz); if (sz === 0) this.wr8(a, v); else if (sz === 1) this.wr16(a, v); else this.wr32(a, v); }
  }
  // Write back to the operand last read with rdEA (read-modify-write).
  wrBack(mode, reg, sz, v) {
    if (mode === 0) this.setD(reg, sz, v);
    else if (mode === 1) this.d[8 + reg] = v;
    else { const a = this.eaA; if (sz === 0) this.wr8(a, v); else if (sz === 1) this.wr16(a, v); else this.wr32(a, v); }
  }

  // ---- arithmetic helpers (set flags, return result) ------------------------
  logic(r, sz) { this.N = (r & MSB[sz]) !== 0 ? 1 : 0; this.Z = r === 0 ? 1 : 0; this.V = 0; this.C = 0; }
  add(s, d, sz) {
    const r0 = s + d, r = sz === 2 ? r0 >>> 0 : r0 & MASK[sz], m = MSB[sz];
    this.C = this.X = r0 > MASK[sz] ? 1 : 0;
    this.V = ((s ^ r) & (d ^ r) & m) !== 0 ? 1 : 0;
    this.N = (r & m) !== 0 ? 1 : 0; this.Z = r === 0 ? 1 : 0;
    return r;
  }
  addx(s, d, sz) {
    const r0 = s + d + this.X, r = sz === 2 ? r0 >>> 0 : r0 & MASK[sz], m = MSB[sz];
    this.C = this.X = r0 > MASK[sz] ? 1 : 0;
    this.V = ((s ^ r) & (d ^ r) & m) !== 0 ? 1 : 0;
    this.N = (r & m) !== 0 ? 1 : 0; if (r !== 0) this.Z = 0;
    return r;
  }
  sub(s, d, sz) {           // d - s
    const r0 = d - s, r = sz === 2 ? r0 >>> 0 : r0 & MASK[sz], m = MSB[sz];
    this.C = this.X = s > d ? 1 : 0;
    this.V = ((s ^ d) & (r ^ d) & m) !== 0 ? 1 : 0;
    this.N = (r & m) !== 0 ? 1 : 0; this.Z = r === 0 ? 1 : 0;
    return r;
  }
  subx(s, d, sz) {
    const x = this.X, r0 = d - s - x, r = sz === 2 ? r0 >>> 0 : r0 & MASK[sz], m = MSB[sz];
    this.C = this.X = s + x > d ? 1 : 0;
    this.V = ((s ^ d) & (r ^ d) & m) !== 0 ? 1 : 0;
    this.N = (r & m) !== 0 ? 1 : 0; if (r !== 0) this.Z = 0;
    return r;
  }
  cmp(s, d, sz) {           // flags of d - s, X untouched
    const r0 = d - s, r = sz === 2 ? r0 >>> 0 : r0 & MASK[sz], m = MSB[sz];
    this.C = s > d ? 1 : 0;
    this.V = ((s ^ d) & (r ^ d) & m) !== 0 ? 1 : 0;
    this.N = (r & m) !== 0 ? 1 : 0; this.Z = r === 0 ? 1 : 0;
  }

  cond(cc) {
    switch (cc) {
      case 0: return true;
      case 1: return false;
      case 2: return (this.C | this.Z) === 0;        // HI
      case 3: return (this.C | this.Z) !== 0;        // LS
      case 4: return this.C === 0;                   // CC
      case 5: return this.C !== 0;                   // CS
      case 6: return this.Z === 0;                   // NE
      case 7: return this.Z !== 0;                   // EQ
      case 8: return this.V === 0;                   // VC
      case 9: return this.V !== 0;                   // VS
      case 10: return this.N === 0;                  // PL
      case 11: return this.N !== 0;                  // MI
      case 12: return this.N === this.V;             // GE
      case 13: return this.N !== this.V;             // LT
      case 14: return this.Z === 0 && this.N === this.V;   // GT
      default: return this.Z !== 0 || this.N !== this.V;   // LE
    }
  }

  // ---- exceptions -----------------------------------------------------------
  // Group 1/2 exception. `pc` is the PC to stack; `fmt2addr` (>= 0) selects a
  // format $2 frame on 68020+ (instruction address for CHK/TRAPcc/div0/trace).
  exception(vec, pc, fmt2addr = -1) {
    if (this.onException) this.onException(vec, pc);
    const sr = this.getSR();
    this.excTaken = 1;
    if (!this.S) { this.usp = this.d[15]; this.d[15] = this.M ? this.msp : this.isp; this.S = 1; }
    this.T1 = this.T0 = 0;
    if (this.level >= 1) {
      if (fmt2addr >= 0 && this.level >= 2) { this.push32(fmt2addr); this.push16(0x2000 | (vec << 2)); }
      else this.push16(vec << 2);
    }
    this.push32(pc);
    this.push16(sr);
    this.pc = this.rd32((this.vbr + (vec << 2)) >>> 0);
    this.cycles += 14;
    this.updateSlow();
  }

  illegal() { this.exception(4, this.ppc); }
  privilege() { this.exception(8, this.ppc); }

  interrupt(level) {
    let vec = -1;
    if (this.bus.iack) vec = this.bus.iack(level);
    if (vec < 0) vec = 24 + level;
    const sr = this.getSR();
    if (this.onException) this.onException(vec, this.pc);
    this.stopped = 0;
    if (!this.S) { this.usp = this.d[15]; this.d[15] = this.M ? this.msp : this.isp; this.S = 1; }
    this.T1 = this.T0 = 0;
    this.IM = level;
    if (this.level >= 1) this.push16(vec << 2);
    this.push32(this.pc);
    this.push16(sr);
    if (this.M) {                       // 020+: throwaway frame on the interrupt stack
      this.msp = this.d[15]; this.d[15] = this.isp; this.M = 0;
      this.push16(0x1000 | (vec << 2));
      this.push32(this.pc);
      this.push16(sr | 0x2000);
    }
    this.pc = this.rd32((this.vbr + (vec << 2)) >>> 0);
    this.cycles += 24;
    this.updateSlow();
  }

  // Group 0: bus error (vector 2) / address error (vector 3). Throws.
  busError(addr, write) {
    this.faultVec = 2; this.faultAddr = addr >>> 0; this.faultWrite = write ? 1 : 0; this.faultInsn = 0;
    throw FAULT;
  }
  addrError(addr, write, insn) {
    this.faultVec = 3; this.faultAddr = addr >>> 0; this.faultWrite = write; this.faultInsn = insn;
    throw FAULT;
  }

  group0() {
    if (this.inFault) { this.halted = 1; this.updateSlow(); return; }     // double fault
    this.inFault = 1;
    try {
      const vec = this.faultVec, addr = this.faultAddr;
      if (this.onException) this.onException(vec, this.ppc);
      const sr = this.getSR();
      if (!this.S) { this.usp = this.d[15]; this.d[15] = this.M ? this.msp : this.isp; this.S = 1; }
      this.T1 = this.T0 = 0;
      const fc = (sr & 0x2000 ? 4 : 0) | (this.faultInsn ? 2 : 1);
      if (this.level === 0) {
        this.push32(this.faultInsn ? addr : this.pc - 2);
        this.push16(sr);
        this.push16(this.ir);
        this.push32(addr);
        this.push16((this.ir & 0xFFE0) | (this.faultWrite ? 0 : 0x10) | (this.faultInsn ? 0x08 : 0) | fc);
      } else if (this.level === 1) {
        // format $8, 29 words
        this.d[15] -= 32;                                   // internal state
        this.push16(this.ir); this.push16(0); this.push16(0); this.push16(0); this.push16(0); this.push16(0);
        this.push32(addr);
        this.push16((this.faultWrite ? 0 : 0x0100) | fc);    // special status word
        this.push16(0x8000 | (vec << 2));
        this.push32(this.ppc);
        this.push16(sr);
      } else if (this.level < 4) {
        // format $A, 16 words (fault at instruction boundary; no rerun requested)
        this.push32(0); this.push32(0); this.push32(addr);
        this.push16(0); this.push16(0);
        this.push16((this.faultWrite ? 0 : 0x40) | fc);      // SSW, DF clear
        this.push16(0);
        this.push16(0xA000 | (vec << 2));
        this.push32(this.ppc);
        this.push16(sr);
      } else {
        // 68040 format $7 access error frame, 30 words
        this.d[15] -= 36;
        this.push32(addr);                                  // fault address
        this.push16(0); this.push16(0); this.push16(0);
        this.push16((this.faultWrite ? 0 : 0x0100) | fc);    // SSW
        this.push32(addr);                                  // effective address
        this.push16(0x7000 | (vec << 2));
        this.push32(this.ppc);
        this.push16(sr);
      }
      this.pc = this.rd32((this.vbr + (vec << 2)) >>> 0);
      this.cycles += 30;
      this.inFault = 0;
    } catch (e) {
      if (e !== FAULT) throw e;
      this.halted = 1;
    }
    this.updateSlow();
  }

  // RTE helper for 68010+: returns false if a format error exception was raised.
  rteFrame() {
    for (;;) {
      const sp = this.d[15] >>> 0;
      const sr = this.rd16(sp), pc = this.rd32(sp + 2), fw = this.rd16(sp + 6);
      const size = FRAME_SIZE[fw >> 12];
      if (size === undefined) { this.exception(14, this.ppc); return false; }
      this.d[15] = sp + size;
      this.setSR(sr);
      this.pc = pc;
      if ((fw >> 12) !== 1) return true;
    }
  }

  // ---- execution --------------------------------------------------------------
  // Handles the rare states; returns true if the main loop should `continue`.
  doSlow() {
    if (this.halted) { this.cycles = this.endCycles; return true; }
    if (this.nmi) { this.nmi = 0; this.interrupt(7); return true; }
    if (this.ipl > this.IM) { this.interrupt(this.ipl); return true; }
    if (this.stopped) { this.cycles = this.endCycles; return true; }
    if (this.T1 | this.T0) {
      // execute one instruction, then take the trace exception
      const t1 = this.T1;
      const pc = this.pc; this.ppc = pc;
      if (pc & 1) this.addrError(pc, 0, 1);
      const op = this.bus.read16(pc); this.pc = pc + 2; this.cycles += 4; this.ir = op;
      this.excTaken = 0; this.insns++;
      const pcBefore = this.pc;
      this.tbl[op](this, op);
      // T1: every instruction. T0 (020+): change of flow only (approximated by a non-sequential PC).
      if (!this.excTaken && (t1 || this.changedFlow(pcBefore))) this.exception(9, this.pc, this.ppc);
      return true;
    }
    this.updateSlow();
    return this.slow !== 0;
  }
  changedFlow(pcAfterOpcode) { const delta = this.pc - pcAfterOpcode; return delta < 0 || delta > 20; }

  // Run for (at least) `n` clock cycles.
  run(n) {
    this.endCycles = this.cycles + n;
    const bus = this.bus;
    for (;;) {
      try {
        const tbl = this.tbl;
        while (this.cycles < this.endCycles) {
          if (this.slow !== 0 && this.doSlow()) continue;
          const pc = this.pc;
          this.ppc = pc;
          if ((pc & 1) !== 0) this.addrError(pc, 0, 1);
          const op = bus.read16(pc);
          this.pc = pc + 2;
          this.cycles += 4;
          this.ir = op;
          this.insns++;
          tbl[op](this, op);
        }
        return;
      } catch (e) {
        if (e !== FAULT) throw e;
        this.group0();
      }
    }
  }

  // Execute exactly one instruction (or one interrupt entry). For debuggers/harnesses.
  step() {
    const save = this.endCycles;
    this.endCycles = this.cycles + 1;
    try {
      if (this.slow !== 0 && this.doSlow()) return;
      const pc = this.pc;
      this.ppc = pc;
      if ((pc & 1) !== 0) this.addrError(pc, 0, 1);
      const op = this.bus.read16(pc);
      this.pc = pc + 2; this.cycles += 4; this.ir = op; this.insns++;
      this.tbl[op](this, op);
    } catch (e) {
      if (e !== FAULT) throw e;
      this.group0();
    } finally { this.endCycles = save; }
  }

  // Snapshot of the programmer-visible state (for traces and tests).
  state() {
    const r = [];
    for (let i = 0; i < 16; i++) r.push(this.d[i] >>> 0);
    return { d: r.slice(0, 8), a: r.slice(8), pc: this.pc >>> 0, sr: this.getSR() };
  }
}
export { FAULT };
