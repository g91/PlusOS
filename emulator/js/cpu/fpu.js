// fpu.js - 68881/68882/68040 floating point coprocessor.
// Registers are held as JS doubles (53-bit mantissa instead of 64): results of
// extended-precision programs can differ in the last 11 bits.

const dv = new DataView(new ArrayBuffer(8));
const P2_32 = 4294967296;

function setCC(c, v) {
  let cc = 0;
  if (v !== v) cc = 1;                                   // NAN
  else {
    if (v === 0) cc |= 4;                                // Z
    else if (v === Infinity || v === -Infinity) cc |= 2; // I
    if (v < 0 || (v === 0 && 1 / v < 0)) cc |= 8;        // N
  }
  c.fpsr = ((c.fpsr & 0x00FFFFFF) | (cc << 24)) >>> 0;
}

function fcond(c, p) {
  const cc = c.fpsr >>> 24, N = (cc >> 3) & 1, Z = (cc >> 2) & 1, NAN = cc & 1;
  switch (p & 15) {
    case 0: return false;
    case 1: return !!Z;
    case 2: return !(NAN | Z | N);
    case 3: return !!(Z | !(NAN | N));
    case 4: return !!(N & !(NAN | Z));
    case 5: return !!(Z | (N & !NAN));
    case 6: return !(NAN | Z);
    case 7: return !NAN;
    case 8: return !!NAN;
    case 9: return !!(NAN | Z);
    case 10: return !!(NAN | !(N | Z));
    case 11: return !!(NAN | Z | !N);
    case 12: return !!(NAN | (N & !Z));
    case 13: return !!(NAN | Z | N);
    case 14: return !Z;
    default: return true;
  }
}

// round to integer per FPCR rounding mode (0 nearest-even, 1 zero, 2 -inf, 3 +inf)
function roundInt(c, v, mode = (c.fpcr >> 4) & 3) {
  if (v !== v || !isFinite(v)) return v;
  switch (mode) {
    case 1: return Math.trunc(v);
    case 2: return Math.floor(v);
    case 3: return Math.ceil(v);
    default: {
      const f = Math.floor(v), d = v - f;
      if (d < 0.5) return f;
      if (d > 0.5) return f + 1;
      return (f % 2 === 0) ? f : f + 1;
    }
  }
}
function roundPrec(c, v) {
  const p = (c.fpcr >> 6) & 3;
  return p === 1 ? Math.fround(v) : v;
}
function toInt(c, v, bits) {
  const lim = Math.pow(2, bits - 1);
  if (v !== v) return bits === 32 ? -1 : -1 & (lim * 2 - 1);
  let r = roundInt(c, v);
  if (r >= lim) r = lim - 1; else if (r < -lim) r = -lim;
  return r;
}

// ---- extended / packed conversion ---------------------------------------------------------------
function extToDouble(se, mh, ml) {
  const sign = (se & 0x8000) ? -1 : 1, e = se & 0x7FFF;
  if (e === 0x7FFF) return ((mh & 0x7FFFFFFF) === 0 && ml === 0) ? sign * Infinity : NaN;
  const m = mh * P2_32 + ml;
  if (m === 0) return sign * 0;
  let x = e - 16383 - 63;
  // split the scaling so intermediate powers stay finite
  let r = m;
  while (x > 1000) { r *= Math.pow(2, 1000); x -= 1000; }
  while (x < -1000) { r *= Math.pow(2, -1000); x += 1000; }
  return sign * r * Math.pow(2, x);
}
function doubleToExt(v) {       // -> [se, mh, ml]
  dv.setFloat64(0, v);
  const hi = dv.getUint32(0), lo = dv.getUint32(4);
  const sign = (hi >>> 31) << 15, e = (hi >>> 20) & 0x7FF;
  let fh = hi & 0xFFFFF, fl = lo;
  if (e === 0x7FF) return [sign | 0x7FFF, (fh || fl) ? 0xC0000000 : 0, 0];
  if (e === 0) {
    if (fh === 0 && fl === 0) return [sign, 0, 0];
    // denormal: normalise
    let ex = -1022 + 16383, mh = ((fh << 11) | (fl >>> 21)) >>> 0, ml = (fl << 11) >>> 0;
    while (!(mh & 0x80000000)) { mh = ((mh << 1) | (ml >>> 31)) >>> 0; ml = (ml << 1) >>> 0; ex--; }
    return [sign | ex, mh, ml];
  }
  return [sign | (e - 1023 + 16383), (0x80000000 | (fh << 11) | (fl >>> 21)) >>> 0, (fl << 11) >>> 0];
}
function packedToDouble(w0, w1, w2) {
  const sm = w0 >>> 31, sexp = (w0 >>> 30) & 1;
  if (((w0 >>> 16) & 0x7FFF) === 0x7FFF) return (w1 === 0 && w2 === 0) ? (sm ? -Infinity : Infinity) : NaN;
  const exp = ((w0 >>> 24) & 0xF) * 100 + ((w0 >>> 20) & 0xF) * 10 + ((w0 >>> 16) & 0xF);
  let s = (sm ? '-' : '') + (w0 & 0xF) + '.';
  for (let i = 28; i >= 0; i -= 4) s += (w1 >>> i) & 0xF;
  for (let i = 28; i >= 0; i -= 4) s += (w2 >>> i) & 0xF;
  return parseFloat(s + 'e' + (sexp ? '-' : '+') + exp);
}
function doubleToPacked(v) {
  if (v !== v) return [0x7FFF0000, 0xFFFFFFFF, 0xFFFFFFFF];
  if (!isFinite(v)) return [(v < 0 ? 0xFFFF0000 : 0x7FFF0000) >>> 0, 0, 0];
  const neg = v < 0 || (v === 0 && 1 / v < 0);
  const s = Math.abs(v).toExponential(16);            // d.dddddddddddddddde+x
  const [mant, ex] = s.split('e');
  const digits = mant.replace('.', '');
  let e = parseInt(ex, 10);
  let w0 = (neg ? 0x80000000 : 0) | (e < 0 ? 0x40000000 : 0);
  e = Math.abs(e);
  w0 |= (Math.floor(e / 100) % 10) << 24 | (Math.floor(e / 10) % 10) << 20 | (e % 10) << 16;
  w0 |= digits.charCodeAt(0) - 48;
  let w1 = 0, w2 = 0;
  for (let i = 0; i < 8; i++) w1 = (w1 << 4) | (digits.charCodeAt(1 + i) - 48);
  for (let i = 0; i < 8; i++) w2 = (w2 << 4) | (digits.charCodeAt(9 + i) - 48);
  return [w0 >>> 0, w1 >>> 0, w2 >>> 0];
}

const FMT_BYTES = [4, 4, 12, 12, 2, 8, 1];

// effective address for an FPU memory operand of `n` bytes
function fea(c, mode, reg, n) {
  const d = c.d;
  if (mode === 3) { const a = d[8 + reg]; d[8 + reg] = a + (n === 1 && reg === 7 ? 2 : n); return a >>> 0; }
  if (mode === 4) { const a = d[8 + reg] - (n === 1 && reg === 7 ? 2 : n); d[8 + reg] = a; return a >>> 0; }
  if (mode === 7 && reg === 4) { const pc = c.pc; c.pc = pc + (n === 1 ? 2 : n); return n === 1 ? pc + 1 : pc; }
  return c.ea(mode, reg, 2);
}

function readOperand(c, mode, reg, fmt) {
  if (mode === 0) {
    const v = c.d[reg];
    switch (fmt) {
      case 0: return v;
      case 1: dv.setInt32(0, v); return dv.getFloat32(0);
      case 4: return (v << 16) >> 16;
      case 6: return (v << 24) >> 24;
      default: return NaN;
    }
  }
  const a = fea(c, mode, reg, FMT_BYTES[fmt]);
  switch (fmt) {
    case 0: return c.rd32(a) | 0;
    case 1: dv.setUint32(0, c.rd32(a)); return dv.getFloat32(0);
    case 2: return extToDouble(c.rd16(a), c.rd32((a + 4) >>> 0), c.rd32((a + 8) >>> 0));
    case 3: return packedToDouble(c.rd32(a), c.rd32((a + 4) >>> 0), c.rd32((a + 8) >>> 0));
    case 4: return (c.rd16(a) << 16) >> 16;
    case 5: dv.setUint32(0, c.rd32(a)); dv.setUint32(4, c.rd32((a + 4) >>> 0)); return dv.getFloat64(0);
    default: return (c.rd8(a) << 24) >> 24;
  }
}
function writeOperand(c, mode, reg, fmt, v) {
  if (fmt === 7) fmt = 3;
  if (mode === 0) {
    switch (fmt) {
      case 0: c.d[reg] = toInt(c, v, 32); return;
      case 1: dv.setFloat32(0, v); c.d[reg] = dv.getInt32(0); return;
      case 4: c.setD(reg, 1, toInt(c, v, 16)); return;
      case 6: c.setD(reg, 0, toInt(c, v, 8)); return;
      default: return;
    }
  }
  const a = fea(c, mode, reg, FMT_BYTES[fmt]);
  switch (fmt) {
    case 0: c.wr32(a, toInt(c, v, 32)); break;
    case 1: dv.setFloat32(0, v); c.wr32(a, dv.getUint32(0)); break;
    case 2: { const e = doubleToExt(v); c.wr16(a, e[0]); c.wr16((a + 2) >>> 0, 0); c.wr32((a + 4) >>> 0, e[1]); c.wr32((a + 8) >>> 0, e[2]); break; }
    case 3: { const p = doubleToPacked(v); c.wr32(a, p[0]); c.wr32((a + 4) >>> 0, p[1]); c.wr32((a + 8) >>> 0, p[2]); break; }
    case 4: c.wr16(a, toInt(c, v, 16)); break;
    case 5: dv.setFloat64(0, v); c.wr32(a, dv.getUint32(0)); c.wr32((a + 4) >>> 0, dv.getUint32(4)); break;
    default: c.wr8(a, toInt(c, v, 8)); break;
  }
}

const CONSTS = {
  0x00: Math.PI, 0x0B: Math.log10(2), 0x0C: Math.E, 0x0D: Math.LOG2E, 0x0E: Math.LOG10E, 0x0F: 0,
  0x30: Math.LN2, 0x31: Math.LN10, 0x32: 1, 0x33: 10, 0x34: 100, 0x35: 1e4, 0x36: 1e8, 0x37: 1e16,
  0x38: 1e32, 0x39: 1e64, 0x3A: 1e128, 0x3B: 1e256, 0x3C: Infinity, 0x3D: Infinity, 0x3E: Infinity, 0x3F: Infinity,
};

function setQuotient(c, q) {
  const s = q < 0 ? 0x80 : 0;
  c.fpsr = ((c.fpsr & 0xFF00FFFF) | ((s | (Math.abs(q) % 128)) << 16)) >>> 0;
}

// Arithmetic. Returns the result or undefined when nothing is stored.
function arith(c, opm, src, dstReg) {
  const fp = c.fp, dst = fp[dstReg];
  let r;
  switch (opm) {
    case 0x00: case 0x40: case 0x44: r = src; break;
    case 0x01: r = roundInt(c, src); break;
    case 0x02: r = Math.sinh(src); break;
    case 0x03: r = Math.trunc(src); break;
    case 0x04: case 0x41: case 0x45: r = Math.sqrt(src); break;
    case 0x06: r = Math.log1p(src); break;
    case 0x08: r = Math.expm1(src); break;
    case 0x09: r = Math.tanh(src); break;
    case 0x0A: r = Math.atan(src); break;
    case 0x0C: r = Math.asin(src); break;
    case 0x0D: r = Math.atanh(src); break;
    case 0x0E: r = Math.sin(src); break;
    case 0x0F: r = Math.tan(src); break;
    case 0x10: r = Math.exp(src); break;
    case 0x11: r = Math.pow(2, src); break;
    case 0x12: r = Math.pow(10, src); break;
    case 0x14: r = Math.log(src); break;
    case 0x15: r = Math.log10(src); break;
    case 0x16: r = Math.log2(src); break;
    case 0x18: case 0x58: case 0x5C: r = Math.abs(src); break;
    case 0x19: r = Math.cosh(src); break;
    case 0x1A: case 0x5A: case 0x5E: r = -src; break;
    case 0x1C: r = Math.acos(src); break;
    case 0x1D: r = Math.cos(src); break;
    case 0x1E: r = (src === 0 || !isFinite(src)) ? src : Math.floor(Math.log2(Math.abs(src))); break;
    case 0x1F: if (src === 0 || !isFinite(src)) r = src; else { const e = Math.floor(Math.log2(Math.abs(src))); r = src / Math.pow(2, e); } break;
    case 0x20: case 0x60: case 0x64: r = dst / src; break;
    case 0x21: { r = dst % src; if (isFinite(dst) && isFinite(src) && src !== 0) setQuotient(c, Math.trunc(dst / src)); break; }
    case 0x22: case 0x62: case 0x66: r = dst + src; break;
    case 0x23: case 0x63: case 0x67: r = dst * src; break;
    case 0x24: r = Math.fround(dst / src); break;
    case 0x25: {
      if (!isFinite(dst) || src === 0 || src !== src) r = NaN;
      else if (!isFinite(src)) r = dst;
      else { const q = roundInt(c, dst / src, 0); r = dst - q * src; setQuotient(c, q); }
      break;
    }
    case 0x26: r = dst * Math.pow(2, Math.trunc(src)); break;
    case 0x27: r = Math.fround(dst * src); break;
    case 0x28: case 0x68: case 0x6C: r = dst - src; break;
    case 0x38: {                                            // FCMP
      let t;
      if (dst !== dst || src !== src) t = NaN; else if (dst === src) t = (dst === 0 && 1 / dst < 0) ? -0 : 0; else t = dst < src ? -1 : 1;
      setCC(c, t);
      return undefined;
    }
    case 0x3A: setCC(c, src); return undefined;             // FTST
    default:
      if (opm >= 0x30 && opm <= 0x37) {                     // FSINCOS
        fp[opm & 7] = Math.cos(src);
        r = Math.sin(src);
        break;
      }
      return null;                                          // unimplemented
  }
  if (opm >= 0x40) r = (opm & 4) ? r : Math.fround(r);      // 040 FSxxx / FDxxx rounding
  else r = roundPrec(c, r);
  setCC(c, r);
  return r;
}

function fpGeneral(c, op) {
  const ext = c.fetch16(), mode = (op >> 3) & 7, reg = op & 7;
  const cls = ext >> 13, fp = c.fp;
  c.fpiar = c.ppc;
  switch (cls) {
    case 0: case 2: {
      const dst = (ext >> 7) & 7, fmt = (ext >> 10) & 7, opm = ext & 0x7F;
      let src;
      if (cls === 0) src = fp[fmt];
      else if (fmt === 7) { fp[dst] = CONSTS[ext & 0x7F] ?? 0; setCC(c, fp[dst]); return; }   // FMOVECR
      else src = readOperand(c, mode, reg, fmt);
      const r = arith(c, opm, src, dst);
      if (r === null) return c.exception(11, c.ppc);
      if (r !== undefined) fp[dst] = r;
      c.cycles += 40;
      return;
    }
    case 3: {                                               // FMOVE FPn -> <ea>
      const fmt = (ext >> 10) & 7;
      writeOperand(c, mode, reg, fmt, fp[(ext >> 7) & 7]);
      c.cycles += 40;
      return;
    }
    case 4: case 5: {                                       // control registers <-> <ea>
      const sel = (ext >> 10) & 7, toEA = cls === 5;
      const regs = [];
      if (sel & 4) regs.push('fpcr'); if (sel & 2) regs.push('fpsr'); if (sel & 1) regs.push('fpiar');
      if (mode === 0 || mode === 1) {
        const rn = mode === 0 ? reg : 8 + reg, name = regs[0];
        if (!name) return;
        if (toEA) c.d[rn] = c[name]; else c[name] = name === 'fpcr' ? c.d[rn] & 0xFFFF : c.d[rn] >>> 0;
        return;
      }
      const a = fea(c, mode, reg, 4 * regs.length);
      for (let i = 0; i < regs.length; i++) {
        const name = regs[i], at = (a + 4 * i) >>> 0;
        if (toEA) c.wr32(at, c[name]); else { const v = c.rd32(at); c[name] = name === 'fpcr' ? v & 0xFFFF : v; }
      }
      return;
    }
    default: {                                              // FMOVEM
      const toEA = cls === 7, m2 = (ext >> 11) & 3;
      let list = (m2 & 1) ? c.d[(ext >> 4) & 7] & 0xFF : ext & 0xFF;
      if (mode === 4) {
        // predecrement: list bit 0 = FP0 ... written from FP7 downwards
        let a = c.d[8 + reg] >>> 0;
        for (let i = 7; i >= 0; i--) if (list & (1 << i)) {
          a = (a - 12) >>> 0;
          const e = doubleToExt(fp[i]);
          c.wr16(a, e[0]); c.wr16(a + 2, 0); c.wr32(a + 4, e[1]); c.wr32(a + 8, e[2]);
        }
        c.d[8 + reg] = a;
        return;
      }
      let a = mode === 3 ? c.d[8 + reg] >>> 0 : c.ea(mode, reg, 2);
      for (let i = 0; i < 8; i++) if (list & (0x80 >> i)) {
        if (toEA) { const e = doubleToExt(fp[i]); c.wr16(a, e[0]); c.wr16(a + 2, 0); c.wr32(a + 4, e[1]); c.wr32(a + 8, e[2]); }
        else fp[i] = extToDouble(c.rd16(a), c.rd32(a + 4), c.rd32(a + 8));
        a = (a + 12) >>> 0;
      }
      if (mode === 3) c.d[8 + reg] = a;
    }
  }
}

function fscc(c, op) {
  const ext = c.fetch16(), mode = (op >> 3) & 7, t = fcond(c, ext);
  if (mode === 0) c.setD(op & 7, 0, t ? 0xFF : 0); else c.wr8(c.ea(mode, op & 7, 0), t ? 0xFF : 0);
}
function fdbcc(c, op) {
  const ext = c.fetch16(), base = c.pc, disp = c.fetchS16();
  if (fcond(c, ext)) return;
  const r = op & 7, v = (c.d[r] - 1) & 0xFFFF;
  c.d[r] = (c.d[r] & 0xFFFF0000) | v;
  if (v !== 0xFFFF) c.pc = (base + disp) >>> 0;
}
function ftrapcc(c, op) {
  const ext = c.fetch16(), k = op & 7;
  if (k === 2) c.pc += 2; else if (k === 3) c.pc += 4;
  if (fcond(c, ext)) c.exception(7, c.pc, c.ppc);
}
function fbcc_w(c, op) { const base = c.pc, disp = c.fetchS16(); if (fcond(c, op)) c.pc = (base + disp) >>> 0; }
function fbcc_l(c, op) { const base = c.pc, disp = c.fetch32(); if (fcond(c, op)) c.pc = (base + disp) >>> 0; }

function fsave(c, op) {
  if (!c.S) return c.privilege();
  const mode = (op >> 3) & 7, reg = op & 7;
  // idle frame: 68881 = $1F18 (28 bytes), 68040 = $4100 (4 bytes)
  const size = c.level === 4 ? 4 : 28, head = c.level === 4 ? 0x41000000 : 0x1F180000;
  let a;
  if (mode === 4) { a = (c.d[8 + reg] - size) >>> 0; c.d[8 + reg] = a; } else a = c.ea(mode, reg, 2);
  c.wr32(a, head);
  for (let i = 4; i < size; i += 4) c.wr32((a + i) >>> 0, 0);
}
function frestore(c, op) {
  if (!c.S) return c.privilege();
  const mode = (op >> 3) & 7, reg = op & 7;
  const a = mode === 3 ? c.d[8 + reg] >>> 0 : c.ea(mode, reg, 2);
  const head = c.rd32(a);
  let size = 4;
  if ((head >>> 24) === 0) { c.fp.fill(NaN); c.fpcr = 0; c.fpsr = 0; c.fpiar = 0; }      // null frame resets the FPU
  else size += (head >>> 16) & 0xFF;
  if (mode === 3) c.d[8 + reg] = a + size;
}

export function fpuDecode(op, L) {
  if ((op & 0xFE00) !== 0xF200) return null;              // coprocessor id 1 only
  const type = (op >> 6) & 7, mode = (op >> 3) & 7, reg = op & 7;
  switch (type) {
    case 0: return (mode === 7 && reg > 4) ? null : fpGeneral;
    case 1:
      if (mode === 1) return fdbcc;
      if (mode === 7 && reg >= 2) return reg <= 4 ? ftrapcc : null;
      return fscc;
    case 2: return fbcc_w;
    case 3: return fbcc_l;
    case 4: return (mode === 2 || mode === 4 || mode === 5 || mode === 6 || (mode === 7 && reg <= 1)) ? fsave : null;
    case 5: return (mode === 2 || mode === 3 || mode === 5 || mode === 6 || (mode === 7 && reg <= 3)) ? frestore : null;
    default: return null;
  }
}
export { extToDouble, doubleToExt, packedToDouble, doubleToPacked };
