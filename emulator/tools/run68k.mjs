#!/usr/bin/env node
// run68k.mjs - bare 680x0 CPU harness for running flat test binaries.
//
//   node run68k.mjs [--cpu 68000|68010|68020|68EC020|68030|68040|...] [--base 0x10000]
//                   [--ram 4M] [--max-insns N] [--trace] [--quiet] prog.bin
//
// Memory map
//   $00000000 .. RAM top     RAM (zero filled). The flat binary is loaded at --base.
//   $00FF0000  byte  write   putchar to stdout
//   $00FF0004  long  write   exit(code)
//   $00FF0008  long  read    number of instructions executed so far
//   $00FF1000 ..             exception stubs (vector n -> $00FF1000 + 2n)
// Reset state: SSP = top of RAM, PC = base, supervisor mode, IPL mask 7.
// Every vector (2..255) initially points at a stub that prints
//   vector number, PC, SR and (for bus/address errors) the fault address
// to stderr and exits with status 2. Programs may install their own handlers by
// writing the vector table. Accesses outside RAM and the I/O page raise a bus error.
// Exit status: the program's exit code (low 8 bits as the OS allows); 2 = unhandled
// exception; 3 = --max-insns reached; 4 = CPU halted (double fault); 1 = usage error.

import fs from 'node:fs';
import { M68K, CPU_MODELS } from '../js/cpu/m68k.js';

const IO_BASE = 0x00FF0000, STUB_BASE = 0x00FF1000;
const VEC_NAMES = { 2: 'bus error', 3: 'address error', 4: 'illegal instruction', 5: 'zero divide', 6: 'CHK', 7: 'TRAPV',
  8: 'privilege violation', 9: 'trace', 10: 'A-line', 11: 'F-line', 14: 'format error', 15: 'uninitialized interrupt', 24: 'spurious interrupt' };

function parseNum(s) {
  const m = /^(0x[0-9a-f]+|\$[0-9a-f]+|\d+)([kKmM]?)$/i.exec(s);
  if (!m) throw new Error('bad number: ' + s);
  let v = m[1].startsWith('$') ? parseInt(m[1].slice(1), 16) : Number(m[1]);
  if (m[2] === 'k' || m[2] === 'K') v *= 1024; else if (m[2] === 'm' || m[2] === 'M') v *= 1024 * 1024;
  return v;
}

function usage() {
  process.stderr.write('usage: node run68k.mjs [--cpu MODEL] [--base ADDR] [--ram SIZE] [--max-insns N] [--trace] [--quiet] prog.bin\n' +
    '  models: ' + Object.keys(CPU_MODELS).join(' ') + '\n');
  process.exit(1);
}

const args = process.argv.slice(2);
let cpuModel = '68000', base = 0x10000, ramSize = 4 * 1024 * 1024, maxInsns = Infinity, trace = false, quiet = false, file = null;
for (let i = 0; i < args.length; i++) {
  const a = args[i];
  if (a === '--cpu') cpuModel = args[++i];
  else if (a === '--base') base = parseNum(args[++i]);
  else if (a === '--ram') ramSize = parseNum(args[++i]);
  else if (a === '--max-insns') maxInsns = parseNum(args[++i]);
  else if (a === '--trace') trace = true;
  else if (a === '--quiet') quiet = true;
  else if (a.startsWith('--')) usage();
  else file = a;
}
if (!file) usage();
if (!CPU_MODELS[cpuModel]) { const alt = cpuModel.toUpperCase(); if (CPU_MODELS[alt]) cpuModel = alt; else usage(); }

const prog = fs.readFileSync(file);
if (base + prog.length > ramSize) { process.stderr.write('program does not fit in RAM\n'); process.exit(1); }
const ram = new Uint8Array(ramSize);
ram.set(prog, base);

const AMASK = CPU_MODELS[cpuModel].addrBits === 24 ? 0xFFFFFF : 0xFFFFFFFF;
class Exit { constructor(code) { this.code = code; } }
let cpu;
const outBuf = [];
function flushOut() { if (outBuf.length) { fs.writeSync(1, Buffer.from(outBuf)); outBuf.length = 0; } }

function reportException(vec) {
  flushOut();
  const sp = cpu.d[15] >>> 0;
  const r16 = (a) => (a + 1 < ramSize ? (ram[a] << 8) | ram[a + 1] : 0);
  const r32 = (a) => ((r16(a) << 16) | r16(a + 2)) >>> 0;
  let sr, pc, fault = null;
  if (cpu.level === 0 && (vec === 2 || vec === 3)) { fault = r32(sp + 2); sr = r16(sp + 8); pc = r32(sp + 10); }
  else { sr = r16(sp); pc = r32(sp + 2); if (vec === 2 || vec === 3) fault = cpu.faultAddr; }
  const hex = (v, n) => v.toString(16).toUpperCase().padStart(n, '0');
  let msg = `\n*** unhandled exception: vector ${vec}` + (VEC_NAMES[vec] ? ` (${VEC_NAMES[vec]})` : vec >= 32 && vec < 48 ? ` (TRAP #${vec - 32})` : '') +
    ` PC=$${hex(pc, 8)} SR=$${hex(sr, 4)}`;
  if (fault !== null) msg += ` fault address=$${hex(fault, 8)}`;
  msg += ` (instruction at $${hex(lastExcInsn >>> 0, 8)}, ${cpu.insns} instructions executed)\n`;
  for (let i = 0; i < 8; i++) msg += `D${i}=${hex(cpu.d[i] >>> 0, 8)}${i === 7 ? '\n' : ' '}`;
  for (let i = 0; i < 8; i++) msg += `A${i}=${hex(cpu.d[8 + i] >>> 0, 8)}${i === 7 ? '\n' : ' '}`;
  process.stderr.write(msg);
  throw new Exit(2);
}

const bus = {
  read8(a) { a = (a & AMASK) >>> 0; if (a < ramSize) return ram[a]; return this.io(a, 1); },
  read16(a) {
    a = (a & AMASK) >>> 0;
    if (a < ramSize - 1) return (ram[a] << 8) | ram[a + 1];
    if (a >= STUB_BASE && a < STUB_BASE + 512) reportException((a - STUB_BASE) >> 1);
    return this.io(a, 2);
  },
  read32(a) {
    a = (a & AMASK) >>> 0;
    if (a < ramSize - 3) return ((ram[a] << 24) | (ram[a + 1] << 16) | (ram[a + 2] << 8) | ram[a + 3]) >>> 0;
    return this.io(a, 4);
  },
  write8(a, v) {
    a = (a & AMASK) >>> 0;
    if (a < ramSize) { ram[a] = v; return; }
    if (a >= IO_BASE && a < IO_BASE + 4) { outBuf.push(v & 0xFF); if (v === 10 || outBuf.length > 4096) flushOut(); return; }
    cpu.busError(a, true);
  },
  write16(a, v) {
    a = (a & AMASK) >>> 0;
    if (a < ramSize - 1) { ram[a] = v >> 8; ram[a + 1] = v; return; }
    if (a >= IO_BASE && a < IO_BASE + 4) { outBuf.push(v & 0xFF); return; }
    if (a === IO_BASE + 4 || a === IO_BASE + 6) throw new Exit(v & 0xFFFF);
    cpu.busError(a, true);
  },
  write32(a, v) {
    a = (a & AMASK) >>> 0;
    if (a < ramSize - 3) { ram[a] = v >>> 24; ram[a + 1] = v >>> 16; ram[a + 2] = v >>> 8; ram[a + 3] = v; return; }
    if (a === IO_BASE + 4) throw new Exit(v | 0);
    if (a === IO_BASE) { outBuf.push(v & 0xFF); return; }
    cpu.busError(a, true);
  },
  io(a, n) {
    if (a >= IO_BASE + 8 && a < IO_BASE + 12) {
      const v = cpu.insns >>> 0;
      if (n === 4) return v;
      const sh = (4 - n - (a - IO_BASE - 8)) * 8;
      return (v >>> sh) & (n === 2 ? 0xFFFF : 0xFF);
    }
    if (a >= IO_BASE && a < IO_BASE + 8) return 0;
    cpu.busError(a, false);
    return 0;
  },
};

// reset vectors + exception stubs
const w32 = (a, v) => { ram[a] = v >>> 24; ram[a + 1] = v >>> 16; ram[a + 2] = v >>> 8; ram[a + 3] = v; };
w32(0, ramSize);
w32(4, base);
for (let v = 2; v < 256; v++) w32(v * 4, STUB_BASE + v * 2);

cpu = new M68K(bus, cpuModel);
let lastExcInsn = 0;
cpu.onException = () => { lastExcInsn = cpu.ppc; };

function traceLine() {
  const hex = (v, n) => (v >>> 0).toString(16).toUpperCase().padStart(n, '0');
  const pc = cpu.pc >>> 0;
  let s = `${hex(pc, 8)}: ${hex(pc < ramSize - 1 ? (ram[pc] << 8) | ram[pc + 1] : 0, 4)}  SR=${hex(cpu.getSR(), 4)}  D:`;
  for (let i = 0; i < 8; i++) s += ' ' + hex(cpu.d[i], 8);
  s += '  A:';
  for (let i = 0; i < 8; i++) s += ' ' + hex(cpu.d[8 + i], 8);
  process.stderr.write(s + '\n');
}

let code = 0;
try {
  if (trace) {
    while (cpu.insns < maxInsns && !cpu.halted) { traceLine(); cpu.step(); if (cpu.stopped) break; }
  } else {
    while (cpu.insns < maxInsns && !cpu.halted) {
      cpu.run(Math.min(2000000, (maxInsns - cpu.insns) * 4));
      if (cpu.stopped) break;        // STOP with no interrupt source: nothing will ever wake it
    }
  }
  flushOut();
  if (cpu.halted) { process.stderr.write(`\n*** CPU halted (double fault) at PC=$${(cpu.ppc >>> 0).toString(16)}\n`); code = 4; }
  else if (cpu.stopped) { process.stderr.write(`\n*** CPU stopped (STOP instruction) at PC=$${(cpu.pc >>> 0).toString(16)}\n`); code = 0; }
  else { if (!quiet) process.stderr.write(`\n*** --max-insns limit reached (${cpu.insns} instructions) PC=$${(cpu.pc >>> 0).toString(16)}\n`); code = 3; }
} catch (e) {
  if (!(e instanceof Exit)) { flushOut(); throw e; }
  flushOut();
  code = e.code;
}
process.exitCode = code & 0xFF;
