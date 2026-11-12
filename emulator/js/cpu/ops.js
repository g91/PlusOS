// ops.js - 68000/68010 instruction handlers and the 64K opcode dispatch table builder.
// Handlers are plain functions (cpu, opcode). 68020+ handlers live in ops020.js, the FPU in fpu.js.

import * as X from './ops020.js';
import { fpuDecode } from './fpu.js';

const MASK = [0xFF, 0xFFFF, 0xFFFFFFFF];
const MSB = [0x80, 0x8000, 0x80000000];

const ill = (c) => c.illegal();

function imm(c, sz) { return sz === 2 ? c.fetch32() : sz === 1 ? c.fetch16() : c.fetch16() & 0xFF; }
function sext(v, sz) { return sz === 0 ? (v << 24) >> 24 : sz === 1 ? (v << 16) >> 16 : v | 0; }

// ---- MOVE ---------------------------------------------------------------------
function move_b(c, op) { const v = c.rdEA((op >> 3) & 7, op & 7, 0); c.logic(v, 0); c.wrEA((op >> 6) & 7, (op >> 9) & 7, 0, v); }
function move_w(c, op) { const v = c.rdEA((op >> 3) & 7, op & 7, 1); c.logic(v, 1); c.wrEA((op >> 6) & 7, (op >> 9) & 7, 1, v); }
function move_l(c, op) { const v = c.rdEA((op >> 3) & 7, op & 7, 2); c.logic(v, 2); c.wrEA((op >> 6) & 7, (op >> 9) & 7, 2, v); }
// register-destination fast paths
function move_b_dn(c, op) { const v = c.rdEA((op >> 3) & 7, op & 7, 0); c.logic(v, 0); const r = (op >> 9) & 7; c.d[r] = (c.d[r] & 0xFFFFFF00) | v; }
function move_w_dn(c, op) { const v = c.rdEA((op >> 3) & 7, op & 7, 1); c.logic(v, 1); const r = (op >> 9) & 7; c.d[r] = (c.d[r] & 0xFFFF0000) | v; }
function move_l_dn(c, op) { const v = c.rdEA((op >> 3) & 7, op & 7, 2); c.logic(v, 2); c.d[(op >> 9) & 7] = v; }
function movea_w(c, op) { c.d[8 + ((op >> 9) & 7)] = (c.rdEA((op >> 3) & 7, op & 7, 1) << 16) >> 16; }
function movea_l(c, op) { c.d[8 + ((op >> 9) & 7)] = c.rdEA((op >> 3) & 7, op & 7, 2); }
function moveq(c, op) { const v = (op << 24) >> 24; c.d[(op >> 9) & 7] = v; c.N = v < 0 ? 1 : 0; c.Z = v === 0 ? 1 : 0; c.V = 0; c.C = 0; }

// ---- immediate group -------------------------------------------------------------
function ori(c, op) { const sz = (op >> 6) & 3, s = imm(c, sz), m = (op >> 3) & 7, r = op & 7; const v = (c.rdEA(m, r, sz) | s) >>> 0; c.logic(v, sz); c.wrBack(m, r, sz, v); if (m === 0 && sz === 2) c.cycles += 4; }
function andi(c, op) { const sz = (op >> 6) & 3, s = imm(c, sz), m = (op >> 3) & 7, r = op & 7; const v = (c.rdEA(m, r, sz) & s) >>> 0; c.logic(v, sz); c.wrBack(m, r, sz, v); if (m === 0 && sz === 2) c.cycles += 2; }
function eori(c, op) { const sz = (op >> 6) & 3, s = imm(c, sz), m = (op >> 3) & 7, r = op & 7; const v = (c.rdEA(m, r, sz) ^ s) >>> 0; c.logic(v, sz); c.wrBack(m, r, sz, v); if (m === 0 && sz === 2) c.cycles += 4; }
function addi(c, op) { const sz = (op >> 6) & 3, s = imm(c, sz), m = (op >> 3) & 7, r = op & 7; const v = c.add(s, c.rdEA(m, r, sz), sz); c.wrBack(m, r, sz, v); if (m === 0 && sz === 2) c.cycles += 4; }
function subi(c, op) { const sz = (op >> 6) & 3, s = imm(c, sz), m = (op >> 3) & 7, r = op & 7; const v = c.sub(s, c.rdEA(m, r, sz), sz); c.wrBack(m, r, sz, v); if (m === 0 && sz === 2) c.cycles += 4; }
function cmpi(c, op) { const sz = (op >> 6) & 3, s = imm(c, sz); c.cmp(s, c.rdEA((op >> 3) & 7, op & 7, sz), sz); if (sz === 2) c.cycles += 2; }
function ori_ccr(c) { c.setCCR(c.getCCR() | (c.fetch16() & 0x1F)); c.cycles += 12; }
function andi_ccr(c) { c.setCCR(c.getCCR() & c.fetch16()); c.cycles += 12; }
function eori_ccr(c) { c.setCCR(c.getCCR() ^ (c.fetch16() & 0x1F)); c.cycles += 12; }
function ori_sr(c) { if (!c.S) return c.privilege(); c.setSR(c.getSR() | c.fetch16()); c.cycles += 12; }
function andi_sr(c) { if (!c.S) return c.privilege(); c.setSR(c.getSR() & c.fetch16()); c.cycles += 12; }
function eori_sr(c) { if (!c.S) return c.privilege(); c.setSR(c.getSR() ^ c.fetch16()); c.cycles += 12; }

// ---- bit operations -----------------------------------------------------------------
// type: 0 BTST 1 BCHG 2 BCLR 3 BSET
function bitop(c, op, bit) {
  const type = (op >> 6) & 3, m = (op >> 3) & 7, r = op & 7;
  if (m === 0) {
    const mask = 1 << (bit & 31), v = c.d[r];
    c.Z = (v & mask) === 0 ? 1 : 0;
    if (type === 1) c.d[r] = v ^ mask; else if (type === 2) c.d[r] = v & ~mask; else if (type === 3) c.d[r] = v | mask;
    c.cycles += type === 0 ? 2 : 4;
  } else {
    const mask = 1 << (bit & 7), v = c.rdEA(m, r, 0);
    c.Z = (v & mask) === 0 ? 1 : 0;
    if (type === 1) c.wr8(c.eaA, v ^ mask); else if (type === 2) c.wr8(c.eaA, v & ~mask); else if (type === 3) c.wr8(c.eaA, v | mask);
  }
}
function bit_dyn(c, op) { bitop(c, op, c.d[(op >> 9) & 7]); }
function bit_static(c, op) { bitop(c, op, c.fetch16()); }

function movep(c, op) {
  const a = (c.d[8 + (op & 7)] + c.fetchS16()) >>> 0, dn = (op >> 9) & 7, v = c.d[dn];
  switch ((op >> 6) & 3) {
    case 0: c.setD(dn, 1, (c.rd8(a) << 8) | c.rd8(a + 2)); break;
    case 1: c.d[dn] = (c.rd8(a) << 24) | (c.rd8(a + 2) << 16) | (c.rd8(a + 4) << 8) | c.rd8(a + 6); break;
    case 2: c.wr8(a, v >> 8); c.wr8(a + 2, v); break;
    default: c.wr8(a, v >>> 24); c.wr8(a + 2, v >> 16); c.wr8(a + 4, v >> 8); c.wr8(a + 6, v); break;
  }
}

// ---- line 4 ---------------------------------------------------------------------------
function negx(c, op) { const sz = (op >> 6) & 3, m = (op >> 3) & 7, r = op & 7; const v = c.subx(c.rdEA(m, r, sz), 0, sz); c.wrBack(m, r, sz, v); if (m === 0 && sz === 2) c.cycles += 2; }
function clr(c, op) {
  const sz = (op >> 6) & 3, m = (op >> 3) & 7;
  if (c.level === 0 && m >= 2) { c.rdEA(m, op & 7, sz); c.wrBack(m, op & 7, sz, 0); }     // the 68000 reads before clearing
  else c.wrEA(m, op & 7, sz, 0);
  c.N = 0; c.Z = 1; c.V = 0; c.C = 0; if (((op >> 3) & 7) === 0 && sz === 2) c.cycles += 2; }
function neg(c, op) { const sz = (op >> 6) & 3, m = (op >> 3) & 7, r = op & 7; const v = c.sub(c.rdEA(m, r, sz), 0, sz); c.wrBack(m, r, sz, v); if (m === 0 && sz === 2) c.cycles += 2; }
function not(c, op) { const sz = (op >> 6) & 3, m = (op >> 3) & 7, r = op & 7; const v = sz === 2 ? ~c.rdEA(m, r, sz) >>> 0 : ~c.rdEA(m, r, sz) & MASK[sz]; c.logic(v, sz); c.wrBack(m, r, sz, v); if (m === 0 && sz === 2) c.cycles += 2; }
function tst(c, op) { const sz = (op >> 6) & 3; c.logic(c.rdEA((op >> 3) & 7, op & 7, sz), sz); }
function tas(c, op) { const m = (op >> 3) & 7, r = op & 7; const v = c.rdEA(m, r, 0); c.logic(v, 0); c.wrBack(m, r, 0, v | 0x80); c.cycles += 2; }
function move_from_sr(c, op) { if (c.level >= 1 && !c.S) return c.privilege(); c.wrEA((op >> 3) & 7, op & 7, 1, c.getSR()); c.cycles += 2; }
function move_from_ccr(c, op) { c.wrEA((op >> 3) & 7, op & 7, 1, c.getCCR()); }
function move_to_ccr(c, op) { c.setCCR(c.rdEA((op >> 3) & 7, op & 7, 1)); c.cycles += 8; }
function move_to_sr(c, op) { if (!c.S) return c.privilege(); const v = c.rdEA((op >> 3) & 7, op & 7, 1); c.setSR(v); c.cycles += 8; }

function bcdAdd(c, s, d) {
  const ss = s + d + c.X;
  const bc = ((s & d) | (~ss & s) | (~ss & d)) & 0x88;
  const dc = (((ss + 0x66) ^ ss) & 0x110) >> 1;
  const corf = (bc | dc) - ((bc | dc) >> 2);
  const rr = ss + corf;
  c.X = c.C = ((bc | (ss & ~rr)) >> 7) & 1;
  c.V = ((~ss & rr) >> 7) & 1;
  const r = rr & 0xFF;
  if (r !== 0) c.Z = 0;
  c.N = r >> 7;
  return r;
}
function bcdSub(c, s, d) {        // d - s - X
  const dd = d - s - c.X;
  const bc = ((~d & s) | (dd & ~d) | (dd & s)) & 0x88;
  const corf = bc - (bc >> 2);
  const rr = dd - corf;
  c.X = c.C = ((bc | (~dd & rr)) >> 7) & 1;
  c.V = ((dd & ~rr) >> 7) & 1;
  const r = rr & 0xFF;
  if (r !== 0) c.Z = 0;
  c.N = r >> 7;
  return r;
}
function nbcd(c, op) { const m = (op >> 3) & 7, r = op & 7; const v = bcdSub(c, c.rdEA(m, r, 0), 0); c.wrBack(m, r, 0, v); c.cycles += 2; }
function abcd(c, op) {
  const rx = (op >> 9) & 7, ry = op & 7;
  if (op & 8) { const s = c.rd8(c.ea(4, ry, 0)), a = c.ea(4, rx, 0); c.wr8(a, bcdAdd(c, s, c.rd8(a))); }
  else { c.setD(rx, 0, bcdAdd(c, c.d[ry] & 0xFF, c.d[rx] & 0xFF)); c.cycles += 2; }
}
function sbcd(c, op) {
  const rx = (op >> 9) & 7, ry = op & 7;
  if (op & 8) { const s = c.rd8(c.ea(4, ry, 0)), a = c.ea(4, rx, 0); c.wr8(a, bcdSub(c, s, c.rd8(a))); }
  else { c.setD(rx, 0, bcdSub(c, c.d[ry] & 0xFF, c.d[rx] & 0xFF)); c.cycles += 2; }
}

function swap(c, op) { const r = op & 7, v = c.d[r]; const n = (v << 16) | (v >>> 16); c.d[r] = n; c.N = n < 0 ? 1 : 0; c.Z = n === 0 ? 1 : 0; c.V = 0; c.C = 0; }
function ext_w(c, op) { const r = op & 7, v = (c.d[r] << 24) >> 24; c.d[r] = (c.d[r] & 0xFFFF0000) | (v & 0xFFFF); c.N = v < 0 ? 1 : 0; c.Z = v === 0 ? 1 : 0; c.V = 0; c.C = 0; }
function ext_l(c, op) { const r = op & 7, v = (c.d[r] << 16) >> 16; c.d[r] = v; c.N = v < 0 ? 1 : 0; c.Z = v === 0 ? 1 : 0; c.V = 0; c.C = 0; }
function pea(c, op) { const a = c.ea((op >> 3) & 7, op & 7, 2); c.push32(a); }
function lea(c, op) { c.d[8 + ((op >> 9) & 7)] = c.ea((op >> 3) & 7, op & 7, 2); }
function link(c, op) { const r = 8 + (op & 7); c.push32(r === 15 ? c.d[15] - 4 : c.d[r]); c.d[r] = c.d[15]; c.d[15] += c.fetchS16(); }
function unlk(c, op) { const r = 8 + (op & 7); c.d[15] = c.d[r]; c.d[r] = c.pop32(); }
function move_to_usp(c, op) { if (!c.S) return c.privilege(); c.usp = c.d[8 + (op & 7)]; }
function move_from_usp(c, op) { if (!c.S) return c.privilege(); c.d[8 + (op & 7)] = c.usp; }
function reset(c) { if (!c.S) return c.privilege(); if (c.bus.resetDevices) c.bus.resetDevices(); c.cycles += 128; }
function nop() { }
function stop(c) {
  if (!c.S) return c.privilege();
  c.setSR(c.fetch16());
  c.stopped = 1; c.updateSlow();
}
function rte(c) {
  if (!c.S) return c.privilege();
  if (c.level === 0) { const sr = c.pop16(), pc = c.pop32(); c.setSR(sr); c.pc = pc; }
  else c.rteFrame();
  c.cycles += 4;
}
function rtd(c) { const pc = c.pop32(); c.d[15] += c.fetchS16(); c.pc = pc; }
function rts(c) { c.pc = c.pop32(); c.cycles += 4; }
function trapv(c) { if (c.V) c.exception(7, c.pc, c.ppc); }
function rtr(c) { c.setCCR(c.pop16()); c.pc = c.pop32(); c.cycles += 4; }
function jsr(c, op) { const a = c.ea((op >> 3) & 7, op & 7, 2); c.push32(c.pc); c.pc = a; c.cycles += 4; }
function jmp(c, op) { c.pc = c.ea((op >> 3) & 7, op & 7, 2); c.cycles += 4; }
function trap(c, op) { c.exception(32 + (op & 15), c.pc); }
function chk_w(c, op) {
  const bound = (c.rdEA((op >> 3) & 7, op & 7, 1) << 16) >> 16, v = (c.d[(op >> 9) & 7] << 16) >> 16;
  c.Z = 0; c.V = 0; c.C = 0;
  c.cycles += 6;
  if (v < 0) { c.N = 1; c.exception(6, c.pc, c.ppc); } else if (v > bound) { c.N = 0; c.exception(6, c.pc, c.ppc); }
}

function movem_rm(c, op) {
  const sz = (op & 0x40) ? 2 : 1, inc = sz === 2 ? 4 : 2, mask = c.fetch16(), m = (op >> 3) & 7, r = op & 7, d = c.d;
  if (m === 4) {
    const init = d[8 + r];
    let a = init;
    for (let i = 0; i < 16; i++) if (mask & (1 << i)) {
      a = (a - inc) | 0;
      const rn = 15 - i;
      let v = d[rn];
      if (rn === 8 + r) v = c.level >= 2 ? init - inc : init;
      if (sz === 2) c.wr32(a >>> 0, v); else c.wr16(a >>> 0, v);
    }
    d[8 + r] = a;
  } else {
    let a = c.ea(m, r, sz);
    for (let i = 0; i < 16; i++) if (mask & (1 << i)) {
      if (sz === 2) c.wr32(a, d[i]); else c.wr16(a, d[i]);
      a = (a + inc) >>> 0;
    }
  }
}
function movem_mr(c, op) {
  const sz = (op & 0x40) ? 2 : 1, inc = sz === 2 ? 4 : 2, mask = c.fetch16(), m = (op >> 3) & 7, r = op & 7, d = c.d;
  let a = m === 3 ? d[8 + r] >>> 0 : c.ea(m, r, sz);
  for (let i = 0; i < 16; i++) if (mask & (1 << i)) {
    d[i] = sz === 2 ? c.rd32(a) : (c.rd16(a) << 16) >> 16;
    a = (a + inc) >>> 0;
  }
  if (m === 3) d[8 + r] = a;
  c.cycles += 4;
}

// ---- line 5 -----------------------------------------------------------------------------
function addq(c, op) {
  const sz = (op >> 6) & 3, q = ((op >> 9) & 7) || 8, m = (op >> 3) & 7, r = op & 7;
  if (m === 1) { c.d[8 + r] += q; c.cycles += 4; return; }
  const v = c.add(q, c.rdEA(m, r, sz), sz); c.wrBack(m, r, sz, v);
  if (m === 0 && sz === 2) c.cycles += 4;
}
function subq(c, op) {
  const sz = (op >> 6) & 3, q = ((op >> 9) & 7) || 8, m = (op >> 3) & 7, r = op & 7;
  if (m === 1) { c.d[8 + r] -= q; c.cycles += 4; return; }
  const v = c.sub(q, c.rdEA(m, r, sz), sz); c.wrBack(m, r, sz, v);
  if (m === 0 && sz === 2) c.cycles += 4;
}
function scc(c, op) {
  const m = (op >> 3) & 7, t = c.cond((op >> 8) & 15);
  if (m === 0) { const r = op & 7; c.d[r] = t ? c.d[r] | 0xFF : c.d[r] & 0xFFFFFF00; if (t) c.cycles += 2; }
  else c.wr8(c.ea(m, op & 7, 0), t ? 0xFF : 0);
}
function dbcc(c, op) {
  const base = c.pc;
  const disp = c.fetchS16();
  if (c.cond((op >> 8) & 15)) { c.cycles += 4; return; }
  const r = op & 7, v = (c.d[r] - 1) & 0xFFFF;
  c.d[r] = (c.d[r] & 0xFFFF0000) | v;
  if (v !== 0xFFFF) { c.pc = (base + disp) >>> 0; c.cycles += 2; } else c.cycles += 6;
}

// ---- line 6 -----------------------------------------------------------------------------
function bra_b(c, op) { c.pc = (c.pc + ((op << 24) >> 24)) >>> 0; c.cycles += 6; }
function bra_w(c) { const base = c.pc; c.pc = (base + c.fetchS16()) >>> 0; c.cycles += 2; }
function bra_l(c) { const base = c.pc; c.pc = (base + c.fetch32()) >>> 0; }
function bsr_b(c, op) { c.push32(c.pc); c.pc = (c.pc + ((op << 24) >> 24)) >>> 0; c.cycles += 6; }
function bsr_w(c) { const base = c.pc, disp = c.fetchS16(); c.push32(c.pc); c.pc = (base + disp) >>> 0; c.cycles += 2; }
function bsr_l(c) { const base = c.pc, disp = c.fetch32(); c.push32(c.pc); c.pc = (base + disp) >>> 0; }
function bcc_b(c, op) { if (c.cond((op >> 8) & 15)) { c.pc = (c.pc + ((op << 24) >> 24)) >>> 0; c.cycles += 6; } else c.cycles += 4; }
function bcc_w(c, op) { if (c.cond((op >> 8) & 15)) { const base = c.pc; c.pc = (base + c.fetchS16()) >>> 0; c.cycles += 2; } else { c.pc += 2; c.cycles += 8; } }
function bcc_l(c, op) { if (c.cond((op >> 8) & 15)) { const base = c.pc; c.pc = (base + c.fetch32()) >>> 0; } else c.pc += 4; }
// specialised hot conditions
function bne_b(c, op) { if (c.Z === 0) { c.pc = (c.pc + ((op << 24) >> 24)) >>> 0; c.cycles += 6; } else c.cycles += 4; }
function beq_b(c, op) { if (c.Z !== 0) { c.pc = (c.pc + ((op << 24) >> 24)) >>> 0; c.cycles += 6; } else c.cycles += 4; }

// ---- OR / AND / EOR / ADD / SUB / CMP -----------------------------------------------------
function or_ea_dn(c, op) { const sz = (op >> 6) & 3, dn = (op >> 9) & 7; const v = (c.rdEA((op >> 3) & 7, op & 7, sz) | c.getD(dn, sz)) >>> 0; c.logic(v, sz); c.setD(dn, sz, v); if (sz === 2) c.cycles += 2; }
function or_dn_ea(c, op) { const sz = (op >> 6) & 3, m = (op >> 3) & 7, r = op & 7; const v = (c.rdEA(m, r, sz) | c.getD((op >> 9) & 7, sz)) >>> 0; c.logic(v, sz); c.wrBack(m, r, sz, v); }
function and_ea_dn(c, op) { const sz = (op >> 6) & 3, dn = (op >> 9) & 7; const v = (c.rdEA((op >> 3) & 7, op & 7, sz) & c.getD(dn, sz)) >>> 0; c.logic(v, sz); c.setD(dn, sz, v); if (sz === 2) c.cycles += 2; }
function and_dn_ea(c, op) { const sz = (op >> 6) & 3, m = (op >> 3) & 7, r = op & 7; const v = (c.rdEA(m, r, sz) & c.getD((op >> 9) & 7, sz)) >>> 0; c.logic(v, sz); c.wrBack(m, r, sz, v); }
function eor_dn_ea(c, op) { const sz = (op >> 6) & 3, m = (op >> 3) & 7, r = op & 7; const v = (c.rdEA(m, r, sz) ^ c.getD((op >> 9) & 7, sz)) >>> 0; c.logic(v, sz); c.wrBack(m, r, sz, v); if (m === 0 && sz === 2) c.cycles += 4; }
function add_ea_dn(c, op) { const sz = (op >> 6) & 3, dn = (op >> 9) & 7; c.setD(dn, sz, c.add(c.rdEA((op >> 3) & 7, op & 7, sz), c.getD(dn, sz), sz)); if (sz === 2) c.cycles += 2; }
function add_dn_ea(c, op) { const sz = (op >> 6) & 3, m = (op >> 3) & 7, r = op & 7; const v = c.add(c.getD((op >> 9) & 7, sz), c.rdEA(m, r, sz), sz); c.wrBack(m, r, sz, v); }
function sub_ea_dn(c, op) { const sz = (op >> 6) & 3, dn = (op >> 9) & 7; c.setD(dn, sz, c.sub(c.rdEA((op >> 3) & 7, op & 7, sz), c.getD(dn, sz), sz)); if (sz === 2) c.cycles += 2; }
function sub_dn_ea(c, op) { const sz = (op >> 6) & 3, m = (op >> 3) & 7, r = op & 7; const v = c.sub(c.getD((op >> 9) & 7, sz), c.rdEA(m, r, sz), sz); c.wrBack(m, r, sz, v); }
function cmp_ea_dn(c, op) { const sz = (op >> 6) & 3; c.cmp(c.rdEA((op >> 3) & 7, op & 7, sz), c.getD((op >> 9) & 7, sz), sz); if (sz === 2) c.cycles += 2; }
function adda_w(c, op) { const s = (c.rdEA((op >> 3) & 7, op & 7, 1) << 16) >> 16; c.d[8 + ((op >> 9) & 7)] += s; c.cycles += 4; }
function adda_l(c, op) { const s = c.rdEA((op >> 3) & 7, op & 7, 2); c.d[8 + ((op >> 9) & 7)] += s; c.cycles += 2; }
function suba_w(c, op) { const s = (c.rdEA((op >> 3) & 7, op & 7, 1) << 16) >> 16; c.d[8 + ((op >> 9) & 7)] -= s; c.cycles += 4; }
function suba_l(c, op) { const s = c.rdEA((op >> 3) & 7, op & 7, 2); c.d[8 + ((op >> 9) & 7)] -= s; c.cycles += 2; }
function cmpa_w(c, op) { const s = ((c.rdEA((op >> 3) & 7, op & 7, 1) << 16) >> 16) >>> 0; c.cmp(s, c.d[8 + ((op >> 9) & 7)] >>> 0, 2); c.cycles += 2; }
function cmpa_l(c, op) { c.cmp(c.rdEA((op >> 3) & 7, op & 7, 2), c.d[8 + ((op >> 9) & 7)] >>> 0, 2); c.cycles += 2; }
function cmpm(c, op) { const sz = (op >> 6) & 3; const s = c.rd(c.ea(3, op & 7, sz), sz), d = c.rd(c.ea(3, (op >> 9) & 7, sz), sz); c.cmp(s, d, sz); }
function addx(c, op) {
  const sz = (op >> 6) & 3, rx = (op >> 9) & 7, ry = op & 7;
  if (op & 8) { const s = c.rd(c.ea(4, ry, sz), sz), a = c.ea(4, rx, sz); c.wr(a, sz, c.addx(s, c.rd(a, sz), sz)); }
  else { c.setD(rx, sz, c.addx(c.getD(ry, sz), c.getD(rx, sz), sz)); if (sz === 2) c.cycles += 4; }
}
function subx(c, op) {
  const sz = (op >> 6) & 3, rx = (op >> 9) & 7, ry = op & 7;
  if (op & 8) { const s = c.rd(c.ea(4, ry, sz), sz), a = c.ea(4, rx, sz); c.wr(a, sz, c.subx(s, c.rd(a, sz), sz)); }
  else { c.setD(rx, sz, c.subx(c.getD(ry, sz), c.getD(rx, sz), sz)); if (sz === 2) c.cycles += 4; }
}
function exg_dd(c, op) { const a = (op >> 9) & 7, b = op & 7, t = c.d[a]; c.d[a] = c.d[b]; c.d[b] = t; c.cycles += 2; }
function exg_aa(c, op) { const a = 8 + ((op >> 9) & 7), b = 8 + (op & 7), t = c.d[a]; c.d[a] = c.d[b]; c.d[b] = t; c.cycles += 2; }
function exg_da(c, op) { const a = (op >> 9) & 7, b = 8 + (op & 7), t = c.d[a]; c.d[a] = c.d[b]; c.d[b] = t; c.cycles += 2; }

// ---- multiply / divide ----------------------------------------------------------------------
function mulu(c, op) {
  const s = c.rdEA((op >> 3) & 7, op & 7, 1), dn = (op >> 9) & 7;
  const r = ((c.d[dn] & 0xFFFF) * s) >>> 0;
  c.d[dn] = r; c.N = r >>> 31; c.Z = r === 0 ? 1 : 0; c.V = 0; c.C = 0;
  let n = 0; for (let v = s; v; v &= v - 1) n++;
  c.cycles += 34 + 2 * n;
}
function muls(c, op) {
  const s = c.rdEA((op >> 3) & 7, op & 7, 1), dn = (op >> 9) & 7;
  const r = Math.imul((c.d[dn] << 16) >> 16, (s << 16) >> 16);
  c.d[dn] = r; c.N = r < 0 ? 1 : 0; c.Z = r === 0 ? 1 : 0; c.V = 0; c.C = 0;
  let n = 0; for (let v = ((s << 1) ^ s) & 0xFFFF; v; v &= v - 1) n++;
  c.cycles += 34 + 2 * n;
}
function divu(c, op) {
  const s = c.rdEA((op >> 3) & 7, op & 7, 1), dn = (op >> 9) & 7, d = c.d[dn] >>> 0;
  if (s === 0) { c.N = 0; c.Z = 0; c.V = 0; c.C = 0; c.exception(5, c.pc, c.ppc); return; }
  const q = Math.floor(d / s);
  c.C = 0;
  if (q > 0xFFFF) { c.V = 1; c.N = 1; c.cycles += 6; return; }       // overflow: V and N set, Z unchanged
  c.d[dn] = ((d % s) << 16) | q;
  c.N = q >> 15; c.Z = q === 0 ? 1 : 0; c.V = 0;
  c.cycles += 126;
}
function divs(c, op) {
  const s = (c.rdEA((op >> 3) & 7, op & 7, 1) << 16) >> 16, dn = (op >> 9) & 7, d = c.d[dn];
  if (s === 0) { c.N = 0; c.Z = 0; c.V = 0; c.C = 0; c.exception(5, c.pc, c.ppc); return; }
  const q = Math.trunc(d / s);
  c.C = 0;
  if (q > 32767 || q < -32768) { c.V = 1; c.N = 1; c.cycles += 12; return; }
  const rem = d % s;
  c.d[dn] = ((rem & 0xFFFF) << 16) | (q & 0xFFFF);
  c.N = q < 0 ? 1 : 0; c.Z = q === 0 ? 1 : 0; c.V = 0;
  c.cycles += 146;
}

// ---- shifts and rotates -----------------------------------------------------------------------
// type: 0 AS, 1 LS, 2 ROX, 3 RO.  v is unsigned and already masked to the size.
export function doShift(c, type, left, sz, cnt, v) {
  const bits = 8 << sz, m = MASK[sz], msb = MSB[sz];
  c.V = 0;
  switch (type) {
    case 0:
      if (cnt === 0) c.C = 0;
      else if (left) {
        if (cnt >= bits) { c.V = v !== 0 ? 1 : 0; c.C = c.X = cnt === bits ? v & 1 : 0; v = 0; }
        else {
          const hi = v >>> (bits - 1 - cnt), all = Math.pow(2, cnt + 1) - 1;
          c.V = (hi === 0 || hi === all) ? 0 : 1;
          c.C = c.X = (v >>> (bits - cnt)) & 1;
          v = sz === 2 ? (v << cnt) >>> 0 : (v << cnt) & m;
        }
      } else {
        const neg = (v & msb) !== 0;
        if (cnt >= bits) { c.C = c.X = neg ? 1 : 0; v = neg ? m : 0; }
        else { c.C = c.X = (v >>> (cnt - 1)) & 1; const sv = sext(v, sz) >> cnt; v = sz === 2 ? sv >>> 0 : sv & m; }
      }
      break;
    case 1:
      if (cnt === 0) c.C = 0;
      else if (left) {
        if (cnt > bits) { c.C = c.X = 0; v = 0; }
        else if (cnt === bits) { c.C = c.X = v & 1; v = 0; }
        else { c.C = c.X = (v >>> (bits - cnt)) & 1; v = sz === 2 ? (v << cnt) >>> 0 : (v << cnt) & m; }
      } else {
        if (cnt > bits) { c.C = c.X = 0; v = 0; }
        else if (cnt === bits) { c.C = c.X = (v >>> (bits - 1)) & 1; v = 0; }
        else { c.C = c.X = (v >>> (cnt - 1)) & 1; v = v >>> cnt; }
      }
      break;
    case 2: {
      let x = c.X;
      cnt %= bits + 1;
      if (left) for (let i = 0; i < cnt; i++) { const o = (v & msb) !== 0 ? 1 : 0; v = sz === 2 ? ((v << 1) | x) >>> 0 : ((v << 1) | x) & m; x = o; }
      else for (let i = 0; i < cnt; i++) { const o = v & 1; v = (v >>> 1); if (x) v = (v | msb) >>> 0; x = o; }
      c.X = x; c.C = x;
      break;
    }
    default:
      if (cnt === 0) c.C = 0;
      else {
        const k = cnt % bits;
        if (left) { if (k) v = sz === 2 ? ((v << k) | (v >>> (32 - k))) >>> 0 : ((v << k) | (v >>> (bits - k))) & m; c.C = v & 1; }
        else { if (k) v = sz === 2 ? ((v >>> k) | (v << (32 - k))) >>> 0 : ((v >>> k) | (v << (bits - k))) & m; c.C = (v & msb) !== 0 ? 1 : 0; }
      }
  }
  c.N = (v & msb) !== 0 ? 1 : 0;
  c.Z = v === 0 ? 1 : 0;
  return v;
}
function shift_reg(c, op) {
  const sz = (op >> 6) & 3, r = op & 7;
  const cnt = (op & 0x20) ? c.d[(op >> 9) & 7] & 63 : (((op >> 9) & 7) || 8);
  c.setD(r, sz, doShift(c, (op >> 3) & 3, (op >> 8) & 1, sz, cnt, c.getD(r, sz)));
  c.cycles += 2 + 2 * cnt + (sz === 2 ? 2 : 0);
}
function shift_mem(c, op) {
  const m = (op >> 3) & 7, r = op & 7;
  const v = doShift(c, (op >> 9) & 3, (op >> 8) & 1, 1, 1, c.rdEA(m, r, 1));
  c.wr16(c.eaA, v);
}

// ---- A-line / F-line ----------------------------------------------------------------------------
function aline(c, op) { if (c.onALine !== null && c.onALine(op)) return; c.exception(10, c.ppc); }
function fline(c, op) { if (c.onFLine !== null && c.onFLine(op)) return; c.exception(11, c.ppc); }

// ---- 68010 ------------------------------------------------------------------------------------
function movec(c, op) {
  if (!c.S) return c.privilege();
  const ext = c.fetch16(), rn = ext >>> 12, creg = ext & 0xFFF, toCtl = op & 1, L = c.level, mm = c.mmu;
  let v = c.d[rn];
  const active = (isM) => c.M === isM;
  switch (creg) {
    case 0x000: if (toCtl) c.sfc = v & 7; else v = c.sfc; break;
    case 0x001: if (toCtl) c.dfc = v & 7; else v = c.dfc; break;
    case 0x800: if (toCtl) c.usp = v; else v = c.usp; break;
    case 0x801: if (toCtl) c.vbr = v >>> 0; else v = c.vbr; break;
    case 0x002: if (L < 2) return c.illegal();
      if (toCtl) c.cacr = (v & (L === 2 ? 0x3 : L === 3 ? 0x3313 : 0x80008000)) >>> 0; else v = c.cacr; break;
    case 0x802: if (L !== 2 && L !== 3) return c.illegal(); if (toCtl) c.caar = v >>> 0; else v = c.caar; break;
    case 0x803: if (L < 2) return c.illegal();
      if (toCtl) { if (active(1)) c.d[15] = v; else c.msp = v; } else v = active(1) ? c.d[15] : c.msp; break;
    case 0x804: if (L < 2) return c.illegal();
      if (toCtl) { if (active(0)) c.d[15] = v; else c.isp = v; } else v = active(0) ? c.d[15] : c.isp; break;
    case 0x003: if (L < 4) return c.illegal(); if (toCtl) mm.tc = v & 0xC000; else v = mm.tc; break;
    case 0x004: if (L < 4) return c.illegal(); if (toCtl) mm.itt0 = v; else v = mm.itt0; break;
    case 0x005: if (L < 4) return c.illegal(); if (toCtl) mm.itt1 = v; else v = mm.itt1; break;
    case 0x006: if (L < 4) return c.illegal(); if (toCtl) mm.dtt0 = v; else v = mm.dtt0; break;
    case 0x007: if (L < 4) return c.illegal(); if (toCtl) mm.dtt1 = v; else v = mm.dtt1; break;
    case 0x805: if (L < 4) return c.illegal(); if (toCtl) mm.mmusr = v; else v = mm.mmusr; break;
    case 0x806: if (L < 4) return c.illegal(); if (toCtl) mm.urp = v; else v = mm.urp; break;
    case 0x807: if (L < 4) return c.illegal(); if (toCtl) mm.srp = v; else v = mm.srp; break;
    default: return c.illegal();
  }
  if (!toCtl) c.d[rn] = v;
  c.cycles += 4;
}
function moves(c, op) {
  if (!c.S) return c.privilege();
  const ext = c.fetch16(), sz = (op >> 6) & 3, rn = ext >>> 12, m = (op >> 3) & 7, r = op & 7;
  if (ext & 0x800) c.wrEA(m, r, sz, c.d[rn]);
  else {
    const v = c.rdEA(m, r, sz);
    if (rn >= 8) c.d[rn] = sext(v, sz); else c.setD(rn, sz, v);
  }
}

// ---- decoder ----------------------------------------------------------------------------------
// addressing-mode class masks: bit n = category n
//  0 Dn  1 An  2 (An)  3 (An)+  4 -(An)  5 d16(An)  6 d8(An,Xn)  7 abs.W  8 abs.L  9 d16(PC)  10 d8(PC,Xn)  11 #imm
const ALL = 0xFFF, DATA = 0xFFD, MEM = 0xFFC, CTRL = 0x7E4, ALT = 0x1FF, DALT = 0x1FD, MALT = 0x1FC, CALT = 0x1E4;
function eaCat(mode, reg) { return mode < 7 ? mode : reg <= 4 ? 7 + reg : -1; }
function ok(op, cls) { const k = eaCat((op >> 3) & 7, op & 7); return k >= 0 && (cls >> k) & 1; }

function decode(op, L, fpu, mmu) {
  const mode = (op >> 3) & 7, reg = op & 7, sz = (op >> 6) & 3, hi = (op >> 9) & 7;
  switch (op >> 12) {
    case 0x0:
      if (op & 0x100) {
        if (mode === 1) return movep;
        if (sz === 0) return ok(op, DATA) ? bit_dyn : ill;
        return ok(op, DALT) ? bit_dyn : ill;
      }
      if (sz === 3) {
        if (hi <= 2) return L >= 2 && ok(op, CTRL) ? X.chk2 : ill;
        if (hi === 3) { if (L !== 2) return ill; return mode <= 1 ? X.rtm : ok(op, CTRL) ? X.callm : ill; }
        if (hi === 4) return ok(op, DALT) ? bit_static : ill;
        if (L < 2) return ill;
        if ((op & 0x3F) === 0x3C) return hi >= 6 ? X.cas2 : ill;
        return ok(op, MALT) ? X.cas : ill;
      }
      if ((op & 0x3F) === 0x3C) {
        if (hi === 0) return sz === 0 ? ori_ccr : sz === 1 ? ori_sr : ill;
        if (hi === 1) return sz === 0 ? andi_ccr : sz === 1 ? andi_sr : ill;
        if (hi === 5) return sz === 0 ? eori_ccr : sz === 1 ? eori_sr : ill;
        return ill;
      }
      switch (hi) {
        case 0: return ok(op, DALT) ? ori : ill;
        case 1: return ok(op, DALT) ? andi : ill;
        case 2: return ok(op, DALT) ? subi : ill;
        case 3: return ok(op, DALT) ? addi : ill;
        case 4: return ok(op, sz === 0 ? (DATA & ~0x800) : DALT) ? bit_static : ill;
        case 5: return ok(op, DALT) ? eori : ill;
        case 6: return ok(op, L >= 1 ? (DATA & ~0x800) : DALT) ? cmpi : ill;
        default: return L >= 1 && ok(op, MALT) ? moves : ill;
      }
    case 0x1: case 0x2: case 0x3: {
      const s = op >> 12, dm = (op >> 6) & 7;
      if (!ok(op, ALL) || (s === 1 && mode === 1)) return ill;
      if (dm === 1) return s === 1 ? ill : s === 3 ? movea_w : movea_l;
      const dk = eaCat(dm, hi);
      if (dk < 0 || !((DALT >> dk) & 1)) return ill;
      if (dm === 0) return s === 1 ? move_b_dn : s === 3 ? move_w_dn : move_l_dn;
      return s === 1 ? move_b : s === 3 ? move_w : move_l;
    }
    case 0x4:
      if (op & 0x100) {
        const k = (op >> 6) & 7;
        if (k === 7) { if (mode === 0) return L >= 2 && hi === 4 ? X.extb : ill; return ok(op, CTRL) ? lea : ill; }
        if (k === 6) return ok(op, DATA) ? chk_w : ill;
        if (k === 4) return L >= 2 && ok(op, DATA) ? X.chk_l : ill;
        return ill;
      }
      switch ((op >> 8) & 0xF) {
        case 0x0: return ok(op, DALT) ? (sz === 3 ? move_from_sr : negx) : ill;
        case 0x2: if (sz === 3) return L >= 1 && ok(op, DALT) ? move_from_ccr : ill; return ok(op, DALT) ? clr : ill;
        case 0x4: if (sz === 3) return ok(op, DATA) ? move_to_ccr : ill; return ok(op, DALT) ? neg : ill;
        case 0x6: if (sz === 3) return ok(op, DATA) ? move_to_sr : ill; return ok(op, DALT) ? not : ill;
        case 0x8:
          if (sz === 0) { if (mode === 1) return L >= 2 ? X.link_l : ill; return ok(op, DALT) ? nbcd : ill; }
          if (sz === 1) { if (mode === 0) return swap; if (mode === 1) return X.bkpt; return ok(op, CTRL) ? pea : ill; }
          if (mode === 0) return sz === 2 ? ext_w : ext_l;
          return ok(op, CALT | 0x10) ? movem_rm : ill;
        case 0xA:
          if (sz === 3) { if (op === 0x4AFC) return ill; return ok(op, DALT) ? tas : ill; }
          if (L >= 2) return ok(op, sz === 0 ? DATA : ALL) ? tst : ill;
          return ok(op, DALT) ? tst : ill;
        case 0xC:
          if (sz === 0) return L >= 2 && ok(op, DATA) ? X.mull : ill;
          if (sz === 1) return L >= 2 && ok(op, DATA) ? X.divl : ill;
          return ok(op, CTRL | 0x8) ? movem_mr : ill;
        case 0xE:
          if (sz === 0) return ill;
          if (sz === 2) return ok(op, CTRL) ? jsr : ill;
          if (sz === 3) return ok(op, CTRL) ? jmp : ill;
          switch ((op >> 4) & 3) {
            case 0: return trap;
            case 1: return (op & 8) ? unlk : link;
            case 2: return (op & 8) ? move_from_usp : move_to_usp;
            default:
              switch (op & 0xF) {
                case 0: return reset; case 1: return nop; case 2: return stop; case 3: return rte;
                case 4: return L >= 1 ? rtd : ill; case 5: return rts; case 6: return trapv; case 7: return rtr;
                case 0xA: case 0xB: return L >= 1 ? movec : ill;
                default: return ill;
              }
          }
        default: return ill;
      }
    case 0x5:
      if (sz === 3) {
        if (mode === 1) return dbcc;
        if (mode === 7 && reg >= 2) return L >= 2 && reg <= 4 ? X.trapcc : ill;
        return ok(op, DALT) ? scc : ill;
      }
      if (!ok(op, ALT) || (mode === 1 && sz === 0)) return ill;
      return (op & 0x100) ? subq : addq;
    case 0x6: {
      const cc = (op >> 8) & 15, disp = op & 0xFF;
      if (disp === 0) return cc === 0 ? bra_w : cc === 1 ? bsr_w : bcc_w;
      if (disp === 0xFF && L >= 2) return cc === 0 ? bra_l : cc === 1 ? bsr_l : bcc_l;
      return cc === 0 ? bra_b : cc === 1 ? bsr_b : cc === 6 ? bne_b : cc === 7 ? beq_b : bcc_b;
    }
    case 0x7: return (op & 0x100) ? ill : moveq;
    case 0x8: {
      const k = (op >> 6) & 7;
      if (k === 3) return ok(op, DATA) ? divu : ill;
      if (k === 7) return ok(op, DATA) ? divs : ill;
      if (k < 3) return ok(op, DATA) ? or_ea_dn : ill;
      if (mode <= 1) return k === 4 ? sbcd : L >= 2 ? (k === 5 ? X.pack : X.unpk) : ill;
      return ok(op, MALT) ? or_dn_ea : ill;
    }
    case 0x9: case 0xD: {
      const isAdd = (op >> 12) === 0xD, k = (op >> 6) & 7;
      if (k === 3) return ok(op, ALL) ? (isAdd ? adda_w : suba_w) : ill;
      if (k === 7) return ok(op, ALL) ? (isAdd ? adda_l : suba_l) : ill;
      if (k < 3) { if (!ok(op, ALL) || (mode === 1 && sz === 0)) return ill; return isAdd ? add_ea_dn : sub_ea_dn; }
      if (mode <= 1) return isAdd ? addx : subx;
      return ok(op, MALT) ? (isAdd ? add_dn_ea : sub_dn_ea) : ill;
    }
    case 0xA: return aline;
    case 0xB: {
      const k = (op >> 6) & 7;
      if (k === 3) return ok(op, ALL) ? cmpa_w : ill;
      if (k === 7) return ok(op, ALL) ? cmpa_l : ill;
      if (k < 3) { if (!ok(op, ALL) || (mode === 1 && sz === 0)) return ill; return cmp_ea_dn; }
      if (mode === 1) return cmpm;
      return ok(op, DALT) ? eor_dn_ea : ill;
    }
    case 0xC: {
      const k = (op >> 6) & 7;
      if (k === 3) return ok(op, DATA) ? mulu : ill;
      if (k === 7) return ok(op, DATA) ? muls : ill;
      if (k < 3) return ok(op, DATA) ? and_ea_dn : ill;
      if (mode <= 1) {
        if (k === 4) return abcd;
        if (k === 5) return mode === 0 ? exg_dd : exg_aa;
        return mode === 1 ? exg_da : ill;
      }
      return ok(op, MALT) ? and_dn_ea : ill;
    }
    case 0xE:
      if (sz === 3) {
        if (op & 0x800) {
          if (L < 2) return ill;
          const t = (op >> 8) & 7;                       // 0 tst 1 extu 2 chg 3 exts 4 clr 5 ffo 6 set 7 ins
          const cls = (t === 0 || t === 1 || t === 3 || t === 5) ? (CTRL | 1) : (CALT | 1);
          return ok(op, cls) ? X.bitfield : ill;
        }
        return ok(op, MALT) ? shift_mem : ill;
      }
      return shift_reg;
    default: {
      if (L < 2 && !(fpu || mmu)) return fline;
      const h = X.decodeF(op, L, fpu, mmu) || (fpu ? fpuDecode(op, L) : null);
      return h || fline;
    }
  }
}

const tableCache = new Map();
export function buildTable(level, fpu, mmu) {
  const key = level + (fpu ? 'f' : '') + (mmu ? 'm' : '');
  let t = tableCache.get(key);
  if (!t) {
    t = new Array(65536);
    for (let op = 0; op < 65536; op++) t[op] = decode(op, level, fpu, mmu);
    tableCache.set(key, t);
  }
  return t;
}
export { ill, fline, sext };
