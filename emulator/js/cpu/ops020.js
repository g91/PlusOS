// ops020.js - 68020/030/040 integer instruction handlers, cache/MMU control instructions.

const MASK = [0xFF, 0xFFFF, 0xFFFFFFFF];
const MSB = [0x80, 0x8000, 0x80000000];
function sext(v, sz) { return sz === 0 ? (v << 24) >> 24 : sz === 1 ? (v << 16) >> 16 : v | 0; }

export function extb(c, op) { const r = op & 7, v = (c.d[r] << 24) >> 24; c.d[r] = v; c.N = v < 0 ? 1 : 0; c.Z = v === 0 ? 1 : 0; c.V = 0; c.C = 0; }
export function link_l(c, op) { const r = 8 + (op & 7); c.push32(c.d[r]); c.d[r] = c.d[15]; c.d[15] += c.fetch32(); }
export function bkpt(c) { c.illegal(); }
export function callm(c) { c.illegal(); }       // 68020 only; module calls are not supported -> illegal instruction trap
export function rtm(c) { c.illegal(); }

export function chk_l(c, op) {
  const bound = c.rdEA((op >> 3) & 7, op & 7, 2) | 0, v = c.d[(op >> 9) & 7];
  c.Z = 0; c.V = 0; c.C = 0;
  if (v < 0) { c.N = 1; c.exception(6, c.pc, c.ppc); } else if (v > bound) { c.N = 0; c.exception(6, c.pc, c.ppc); }
}

export function chk2(c, op) {
  const sz = (op >> 9) & 3, ext = c.fetch16(), rn = ext >>> 12;
  const a = c.ea((op >> 3) & 7, op & 7, sz);
  let lo = c.rd(a, sz), hi = c.rd((a + (1 << sz)) >>> 0, sz), v;
  if (rn >= 8) { lo = sext(lo, sz) >>> 0; hi = sext(hi, sz) >>> 0; v = c.d[rn] >>> 0; }
  else v = c.getD(rn, sz);
  const m = rn >= 8 ? 4294967296 : (MASK[sz] + 1);
  const rel = ((v - lo) % m + m) % m, span = ((hi - lo) % m + m) % m;
  c.Z = (v === lo || v === hi) ? 1 : 0;
  c.C = rel > span ? 1 : 0;
  if ((ext & 0x800) && c.C) c.exception(6, c.pc, c.ppc);
}

export function cas(c, op) {
  const sz = ((op >> 9) & 3) - 1, ext = c.fetch16(), du = (ext >> 6) & 7, dc = ext & 7;
  const m = (op >> 3) & 7, r = op & 7;
  const dest = c.rdEA(m, r, sz);
  c.cmp(c.getD(dc, sz), dest, sz);
  if (c.Z) c.wr(c.eaA, sz, c.getD(du, sz)); else c.setD(dc, sz, dest);
}
export function cas2(c, op) {
  const sz = (op & 0x200) ? 2 : 1;
  const e1 = c.fetch16(), e2 = c.fetch16();
  const a1 = c.d[e1 >>> 12] >>> 0, a2 = c.d[e2 >>> 12] >>> 0;
  const d1 = c.rd(a1, sz), d2 = c.rd(a2, sz);
  c.cmp(c.getD(e1 & 7, sz), d1, sz);
  if (c.Z) c.cmp(c.getD(e2 & 7, sz), d2, sz);
  if (c.Z) { c.wr(a1, sz, c.getD((e1 >> 6) & 7, sz)); c.wr(a2, sz, c.getD((e2 >> 6) & 7, sz)); }
  else { c.setD(e1 & 7, sz, d1); c.setD(e2 & 7, sz, d2); }
}

export function trapcc(c, op) {
  const k = op & 7;
  if (k === 2) c.pc += 2; else if (k === 3) c.pc += 4;
  if (c.cond((op >> 8) & 15)) c.exception(7, c.pc, c.ppc);
}

export function pack(c, op) {
  const adj = c.fetch16(), rx = (op >> 9) & 7, ry = op & 7;
  if (op & 8) {
    const lo = c.rd8(c.ea(4, ry, 0)), hi = c.rd8(c.ea(4, ry, 0));
    const t = (((hi << 8) | lo) + adj) & 0xFFFF;
    c.wr8(c.ea(4, rx, 0), ((t >> 4) & 0xF0) | (t & 0x0F));
  } else {
    const t = ((c.d[ry] & 0xFFFF) + adj) & 0xFFFF;
    c.setD(rx, 0, ((t >> 4) & 0xF0) | (t & 0x0F));
  }
}
export function unpk(c, op) {
  const adj = c.fetch16(), rx = (op >> 9) & 7, ry = op & 7;
  if (op & 8) {
    const b = c.rd8(c.ea(4, ry, 0));
    const t = ((((b << 4) & 0x0F00) | (b & 0x0F)) + adj) & 0xFFFF;
    c.wr8(c.ea(4, rx, 0), t & 0xFF); c.wr8(c.ea(4, rx, 0), t >> 8);
  } else {
    const b = c.d[ry] & 0xFF;
    c.setD(rx, 1, (((b << 4) & 0x0F00) | (b & 0x0F)) + adj);
  }
}

export function mull(c, op) {
  const ext = c.fetch16(), dl = (ext >> 12) & 7, dh = ext & 7;
  const s = c.rdEA((op >> 3) & 7, op & 7, 2);
  let p;
  if (ext & 0x800) p = BigInt(s | 0) * BigInt(c.d[dl]); else p = BigInt(s) * BigInt(c.d[dl] >>> 0);
  const lo = Number(BigInt.asUintN(32, p));
  c.C = 0;
  if (ext & 0x400) {
    const hi = Number(BigInt.asUintN(32, p >> 32n));
    c.d[dh] = hi; c.d[dl] = lo;
    c.N = hi >>> 31; c.Z = (hi === 0 && lo === 0) ? 1 : 0; c.V = 0;
  } else {
    c.d[dl] = lo;
    c.N = lo >>> 31; c.Z = lo === 0 ? 1 : 0;
    if (ext & 0x800) c.V = p !== BigInt(lo | 0) ? 1 : 0; else c.V = p !== BigInt(lo) ? 1 : 0;
  }
  c.cycles += 40;
}
export function divl(c, op) {
  const ext = c.fetch16(), dq = (ext >> 12) & 7, dr = ext & 7, signed = ext & 0x800;
  const s = c.rdEA((op >> 3) & 7, op & 7, 2);
  if (s === 0) { c.exception(5, c.pc, c.ppc); return; }
  let dividend, divisor;
  if (signed) {
    divisor = BigInt(s | 0);
    dividend = (ext & 0x400) ? BigInt.asIntN(64, (BigInt(c.d[dr] >>> 0) << 32n) | BigInt(c.d[dq] >>> 0)) : BigInt(c.d[dq]);
  } else {
    divisor = BigInt(s);
    dividend = (ext & 0x400) ? (BigInt(c.d[dr] >>> 0) << 32n) | BigInt(c.d[dq] >>> 0) : BigInt(c.d[dq] >>> 0);
  }
  const q = dividend / divisor, rem = dividend % divisor;
  c.C = 0;
  c.cycles += 80;
  if (signed ? (q > 2147483647n || q < -2147483648n) : q > 0xFFFFFFFFn) { c.V = 1; c.N = 1; c.Z = 0; return; }
  const qn = Number(BigInt.asIntN(32, q));
  c.d[dr] = Number(BigInt.asIntN(32, rem));
  c.d[dq] = qn;
  c.N = qn < 0 ? 1 : 0; c.Z = qn === 0 ? 1 : 0; c.V = 0;
}

// Bit fields. type: 0 tst 1 extu 2 chg 3 exts 4 clr 5 ffo 6 set 7 ins
export function bitfield(c, op) {
  const type = (op >> 8) & 7, ext = c.fetch16(), m = (op >> 3) & 7, r = op & 7, rn = (ext >> 12) & 7;
  let offset = (ext & 0x800) ? c.d[(ext >> 6) & 7] : (ext >> 6) & 31;
  let width = (ext & 0x20) ? c.d[ext & 7] & 31 : ext & 31;
  if (width === 0) width = 32;
  const top = 32 - width;
  const fullOffset = offset;
  let field, ins = 0;
  if (type === 7) { ins = width === 32 ? c.d[rn] >>> 0 : (c.d[rn] & (Math.pow(2, width) - 1)) >>> 0; }
  if (m === 0) {
    offset &= 31;
    const v = c.d[r];
    const rot = offset ? ((v << offset) | (v >>> (32 - offset))) >>> 0 : v >>> 0;
    field = rot >>> top;
    let nf = field, write = true;
    switch (type) {
      case 2: nf = (~field) >>> top << 0; nf = ((~field << top) >>> top) >>> 0; break;
      case 4: nf = 0; break;
      case 6: nf = (0xFFFFFFFF >>> top); break;
      case 7: nf = ins; break;
      default: write = false;
    }
    if (write) {
      const keep = width === 32 ? 0 : (rot << width) >>> width;        // low (32-width) bits of rot
      const nrot = (((nf << top) >>> 0) | keep) >>> 0;
      c.d[r] = offset ? (nrot >>> offset) | (nrot << (32 - offset)) : nrot;
    }
  } else {
    let a = c.ea(m, r, 2);
    a = (a + (offset >> 3)) >>> 0;
    offset &= 7;
    const nbytes = (offset + width + 7) >> 3;
    let hi = 0, b4 = 0;
    for (let i = 0; i < 4 && i < nbytes; i++) hi |= c.rd8((a + i) >>> 0) << (24 - 8 * i);
    hi >>>= 0;
    if (nbytes > 4) b4 = c.rd8((a + 4) >>> 0);
    field = (((hi << offset) | (b4 >>> (8 - offset))) >>> top) >>> 0;
    let nf = field, write = true;
    switch (type) {
      case 2: nf = ((~field << top) >>> top) >>> 0; break;
      case 4: nf = 0; break;
      case 6: nf = (0xFFFFFFFF >>> top); break;
      case 7: nf = ins; break;
      default: write = false;
    }
    if (write) {
      const shift = Math.pow(2, 40 - offset - width);
      let total = hi * 256 + b4;
      total = total - field * shift + nf * shift;
      for (let i = 0; i < nbytes; i++) c.wr8((a + i) >>> 0, Math.floor(total / Math.pow(2, 8 * (4 - i))) % 256);
    }
  }
  const fl = type === 7 ? ins : field;
  c.N = (fl >>> (width - 1)) & 1; c.Z = fl === 0 ? 1 : 0; c.V = 0; c.C = 0;
  switch (type) {
    case 1: c.d[rn] = field; break;
    case 3: c.d[rn] = width === 32 ? field : (field << top) >> top; break;
    case 5: {
      let n = 0;
      while (n < width && !((field >>> (width - 1 - n)) & 1)) n++;
      c.d[rn] = fullOffset + n;
      break;
    }
  }
  c.cycles += 8;
}

// ---- 68040 MOVE16, cache control; 68030/68851 MMU instructions -------------------------------------
function move16_abs(c, op) {
  const an = 8 + (op & 7), abs = c.fetch32();
  const reg = c.d[an] >>> 0;
  const toAbs = (op & 8) === 0;
  const src = (toAbs ? reg : abs) & ~15, dst = (toAbs ? abs : reg) & ~15;
  for (let i = 0; i < 16; i += 4) c.wr32((dst + i) >>> 0, c.rd32((src + i) >>> 0));
  if (!(op & 0x10)) c.d[an] = reg + 16;
}
function move16_pp(c, op) {
  const ext = c.fetch16(), ax = 8 + (op & 7), ay = 8 + ((ext >> 12) & 7);
  const src = c.d[ax] & ~15, dst = c.d[ay] & ~15;
  for (let i = 0; i < 16; i += 4) c.wr32((dst + i) >>> 0, c.rd32((src + i) >>> 0));
  c.d[ax] += 16;
  if (ax !== ay) c.d[ay] += 16;
}
function privNop(c) { if (!c.S) c.privilege(); }

// 68030 / 68851: F000-F03F + extension word
function mmu030(c, op) {
  if (!c.S) return c.privilege();
  const ext = c.fetch16(), m = (op >> 3) & 7, r = op & 7, mm = c.mmu;
  const kind = ext >> 13;
  if (kind === 2 || kind === 0 || kind === 3) {
    // PMOVE
    const preg = (ext >> 10) & 7, toEA = ext & 0x200;
    let name, quad = false, word = false;
    if (kind === 2) {
      if (preg === 0) name = 'tc'; else if (preg === 2) { name = 'srp'; quad = true; } else if (preg === 3) { name = 'crp'; quad = true; } else return c.illegal();
    } else if (kind === 0) {
      if (preg === 2) name = 'tt0'; else if (preg === 3) name = 'tt1'; else return c.illegal();
    } else { name = 'mmusr'; word = true; }
    if (quad) {
      const a = c.ea(m, r, 2);
      if (toEA) { c.wr32(a, mm[name + 'H']); c.wr32((a + 4) >>> 0, mm[name + 'L']); }
      else { mm[name + 'H'] = c.rd32(a); mm[name + 'L'] = c.rd32((a + 4) >>> 0); }
    } else if (toEA) c.wrEA(m, r, word ? 1 : 2, mm[name]);
    else mm[name] = c.rdEA(m, r, word ? 1 : 2);
    return;
  }
  if (kind === 1) {                       // PLOAD / PFLUSH / PFLUSHA
    if ((ext & 0x1C00) === 0x0400) return;                        // PFLUSHA
    if ((ext & 0x1C00) === 0x1000) return;                        // PFLUSH fc,#mask
    c.ea(m, r, 2);                                               // PLOAD / PFLUSH fc,#mask,<ea>: consume the EA
    return;
  }
  if (kind === 4) {                       // PTEST: identity mapping, nothing ever faults
    c.ea(m, r, 2);
    mm.mmusr = 0;
    if (ext & 0x100) c.d[8 + ((ext >> 5) & 7)] = 0;
    return;
  }
  c.illegal();
}

export function decodeF(op, L, fpu, mmu) {
  const cp = (op >> 9) & 7;
  if (L === 4) {
    if ((op & 0xFF20) === 0xF400 || (op & 0xFF20) === 0xF420) return privNop;      // CINV / CPUSH
    if (mmu && (op & 0xFFE0) === 0xF500) return privNop;                           // PFLUSH family
    if (mmu && ((op & 0xFFF8) === 0xF548 || (op & 0xFFF8) === 0xF568)) return privNop;   // PTESTW / PTESTR
    if ((op & 0xFFE0) === 0xF600) return move16_abs;
    if ((op & 0xFFF8) === 0xF620) return move16_pp;
    return null;
  }
  if (cp === 0 && mmu && (L === 3 || L === 2) && (op & 0xFFC0) === 0xF000) {
    const mode = (op >> 3) & 7, reg = op & 7;
    if (mode === 7 && reg > 1) return null;
    return mmu030;
  }
  return null;
}
