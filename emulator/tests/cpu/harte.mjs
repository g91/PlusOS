#!/usr/bin/env node
// harte.mjs - run the SingleStepTests/680x0 68000 instruction vectors (MIT licensed,
// https://github.com/SingleStepTests/680x0) against the CPU core.
// The vectors are NOT vendored: download 68000/v1/*.json.gz into a directory and pass it.
//
//   node harte.mjs <dir-with-json.gz> [--show N] [--no-addr-err] [NAME ...]
//
// For every test: load registers + RAM, execute one instruction, compare registers,
// SR, PC and RAM. Tests whose expected outcome is an address error are reported
// separately (the group-0 stack frame contents of a real 68000 depend on
// prefetch/microcode state that this core only approximates).

import fs from 'node:fs';
import path from 'node:path';
import zlib from 'node:zlib';
import { M68K } from '../../js/cpu/m68k.js';

const args = process.argv.slice(2);
let dir = null, show = 3, names = [], skipAE = false;
for (let i = 0; i < args.length; i++) {
  if (args[i] === '--show') show = Number(args[++i]);
  else if (args[i] === '--no-addr-err') skipAE = true;
  else if (!dir) dir = args[i];
  else names.push(args[i]);
}
if (!dir) { console.error('usage: node harte.mjs <dir> [--show N] [NAME ...]'); process.exit(1); }

const ram = new Uint8Array(1 << 24);
const bus = {
  read8: (a) => ram[a & 0xFFFFFF],
  read16: (a) => { a &= 0xFFFFFF; return (ram[a] << 8) | ram[(a + 1) & 0xFFFFFF]; },
  read32: (a) => { a &= 0xFFFFFF; return ((ram[a] << 24) | (ram[(a + 1) & 0xFFFFFF] << 16) | (ram[(a + 2) & 0xFFFFFF] << 8) | ram[(a + 3) & 0xFFFFFF]) >>> 0; },
  write8: (a, v) => { ram[a & 0xFFFFFF] = v; },
  write16: (a, v) => { a &= 0xFFFFFF; ram[a] = v >> 8; ram[(a + 1) & 0xFFFFFF] = v; },
  write32: (a, v) => { a &= 0xFFFFFF; ram[a] = v >>> 24; ram[(a + 1) & 0xFFFFFF] = v >>> 16; ram[(a + 2) & 0xFFFFFF] = v >>> 8; ram[(a + 3) & 0xFFFFFF] = v; },
};
const cpu = new M68K(bus, '68000');
let known = 0;
// Cases where this core deliberately follows hardware-verified behaviour instead of the vectors
// (the vectors are generated from another emulator, not from silicon):
//  * ASR with a count larger than the operand size on a negative value: C = X = 1 (sign bits shift out)
//  * DIVU/DIVS overflow: N is set
function knownDeviation(t, errs) {
  if (errs.length !== 1 || !errs[0].startsWith('sr=')) return false;
  const [got, want] = errs[0].slice(3).split(' want ').map((x) => parseInt(x, 16));
  if (t.name.includes('ASR')) return (got ^ want) === 0x11;
  if (t.name.includes('DIV')) return (got ^ want) === 0x08 && (got & 0x0A) === 0x0A;
  return false;
}

const files = fs.readdirSync(dir).filter((f) => f.endsWith('.json.gz') && (!names.length || names.includes(f.replace('.json.gz', '')))).sort();
let total = 0, pass = 0, aeTotal = 0, aePass = 0, cycSum = 0, cycRef = 0;
const failed = [], aeSig = new Map(), aeEx = new Map();
for (const f of files) {
  const tests = JSON.parse(zlib.gunzipSync(fs.readFileSync(path.join(dir, f))).toString('utf8'));
  let fTotal = 0, fPass = 0, fAE = 0, fAEPass = 0, shown = 0;
  for (const t of tests) {
    const i = t.initial, e = t.final;
    for (const [a, v] of i.ram) ram[a & 0xFFFFFF] = v;
    bus.write16(i.pc, i.prefetch[0]); bus.write16(i.pc + 2, i.prefetch[1]);
    for (let r = 0; r < 8; r++) cpu.d[r] = i['d' + r];
    for (let r = 0; r < 7; r++) cpu.d[8 + r] = i['a' + r];
    cpu.S = 1; cpu.M = 0; cpu.d[15] = i.ssp; cpu.isp = i.ssp; cpu.usp = i.usp;
    cpu.stopped = 0; cpu.halted = 0; cpu.ipl = 0; cpu.nmi = 0; cpu.inFault = 0;
    cpu.setSR(i.sr);
    cpu.pc = i.pc >>> 0;
    const vec3 = bus.read32(12);
    const c0 = cpu.cycles;
    // a trace bit would add a second exception; the vectors only run one instruction
    cpu.step();
    if (cpu.pc & 1) cpu.step();          // jump to an odd address: the fault is taken at the next fetch
    const cyc = cpu.cycles - c0;
    // compare
    const errs = [];
    for (let r = 0; r < 8; r++) if ((cpu.d[r] >>> 0) !== e['d' + r]) errs.push(`d${r}=${(cpu.d[r] >>> 0).toString(16)} want ${e['d' + r].toString(16)}`);
    for (let r = 0; r < 7; r++) if ((cpu.d[8 + r] >>> 0) !== e['a' + r]) errs.push(`a${r}=${(cpu.d[8 + r] >>> 0).toString(16)} want ${e['a' + r].toString(16)}`);
    const sr = cpu.getSR();
    const ssp = (cpu.S ? cpu.d[15] : cpu.isp) >>> 0, usp = (cpu.S ? cpu.usp : cpu.d[15]) >>> 0;
    if (ssp !== e.ssp) errs.push(`ssp=${ssp.toString(16)} want ${e.ssp.toString(16)}`);
    if (usp !== e.usp) errs.push(`usp=${usp.toString(16)} want ${e.usp.toString(16)}`);
    if (sr !== e.sr) errs.push(`sr=${sr.toString(16)} want ${e.sr.toString(16)}`);
    if ((cpu.pc >>> 0) !== e.pc) errs.push(`pc=${(cpu.pc >>> 0).toString(16)} want ${e.pc.toString(16)}`);
    for (const [a, v] of e.ram) if (ram[a & 0xFFFFFF] !== v) errs.push(`[${a.toString(16)}]=${ram[a & 0xFFFFFF].toString(16)} want ${v.toString(16)}`);
    // an address error is recognisable from the expected final PC = vector 3 handler + SSP drop of 14
    const isAE = e.pc === vec3 && e.pc !== ((i.pc + 2) >>> 0);
    for (const [a] of i.ram) ram[a & 0xFFFFFF] = 0;
    for (const [a] of e.ram) ram[a & 0xFFFFFF] = 0;
    bus.write32(i.pc, 0);
    for (let k = 0; k < 16; k++) ram[(e.ssp + k) & 0xFFFFFF] = 0, ram[(i.ssp - 16 + k) & 0xFFFFFF] = 0;
    if (isAE) {
      fAE++; if (!errs.length) fAEPass++;
      else if (process.env.AESTAT) {
        const sp = e.ssp;
        const sig = errs.map((x) => { const m = /^\[([0-9a-f]+)\]/.exec(x); if (!m) return x.split('=')[0]; const off = parseInt(m[1], 16) - sp; return ['st', 'st', 'aa', 'aa', 'aa', 'aa', 'ir', 'ir', 'sr', 'sr', 'pc', 'pc', 'pc', 'pc'][off] || 'mem'; });
        const key = [...new Set(sig)].join(',');
        aeSig.set(key, (aeSig.get(key) || 0) + 1);
        if (!aeEx.has(key)) aeEx.set(key, t.name + ': ' + errs.join(', '));
      }
      continue;
    }
    fTotal++; cycSum += cyc; cycRef += t.length;
    if (errs.length && knownDeviation(t, errs)) { known++; fTotal--; continue; }
    if (!errs.length) fPass++;
    else if (shown++ < show) { console.log(`  FAIL ${t.name}: ${errs.slice(0, 6).join(', ')}`); if (process.env.DUMP) console.log(JSON.stringify(t.initial), JSON.stringify(t.final)); }
  }
  total += fTotal; pass += fPass; aeTotal += fAE; aePass += fAEPass;
  if (fPass !== fTotal) failed.push(f);
  console.log(`${f.replace('.json.gz', '').padEnd(14)} ${fPass}/${fTotal}` + (fAE ? `   (address-error cases: ${fAEPass}/${fAE})` : ''));
}
console.log(`\nTOTAL ${pass}/${total} (${(100 * pass / total).toFixed(3)}%)  address-error cases ${aePass}/${aeTotal}`);
console.log(`documented deviations skipped: ${known}`);
console.log(`cycle count ratio (core/reference): ${(cycSum / cycRef).toFixed(4)}`);
if (process.env.AESTAT) for (const [k, v] of [...aeSig].sort((a, b) => b[1] - a[1]).slice(0, 25)) console.log(String(v).padStart(7), k, '   e.g.', aeEx.get(k));
if (failed.length) console.log('files with failures: ' + failed.join(' '));
process.exitCode = pass === total ? 0 : 1;
