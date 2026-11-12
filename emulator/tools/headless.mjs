#!/usr/bin/env node
// headless.mjs - run the emulator without a browser, take screenshots, inject input.
//
//   node headless.mjs --machine plus|se|classic --rom PATH [--disk PATH]... [--scsi PATH]...
//        [--cpu 68000|68020|...] [--ram 4] [--frames N] [--screenshot out.png]
//        [--keys "text\n" --keys-at F]   type text starting at frame F (default: frames/2)
//        [--at F:ACTION]...              timed actions, see below
//        [--trace-traps] [--no-pv] [--fast (skip the ROM RAM test)] [--save-disk out.img] [--quiet]
//
// --at actions:   type=TEXT   key=CODE (hex Mac key code, press+release)   keydown=CODE   keyup=CODE
//                 move=DX,DY  mouseto=X,Y   click   press   release   shot=FILE.png   nmi   reset
// Text escapes: \n = Return, \t = Tab, \e = Escape, \b = Backspace.
// Prints PC / exception / A-trap statistics at the end.

import fs from 'node:fs';
import path from 'node:path';
import zlib from 'node:zlib';
import { fileURLToPath } from 'node:url';
import { createMachine, MACHINES, screenToRGBA } from '../js/machines/index.js';
import { typeString } from '../js/devices/keycodes.js';

const repoRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');

// ---- minimal PNG encoder (8-bit RGBA or greyscale, zlib from Node) ----------------------
const CRC_TABLE = (() => { const t = new Uint32Array(256); for (let n = 0; n < 256; n++) { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1; t[n] = c >>> 0; } return t; })();
function crc32(buf) { let c = 0xFFFFFFFF; for (let i = 0; i < buf.length; i++) c = CRC_TABLE[(c ^ buf[i]) & 0xFF] ^ (c >>> 8); return (c ^ 0xFFFFFFFF) >>> 0; }
function chunk(type, data) {
  const out = Buffer.alloc(12 + data.length);
  out.writeUInt32BE(data.length, 0); out.write(type, 4, 'ascii'); data.copy(out, 8);
  out.writeUInt32BE(crc32(out.subarray(4, 8 + data.length)), 8 + data.length);
  return out;
}
export function encodePNG(width, height, rgba, scale = 1) {
  const w = width * scale, h = height * scale;
  const raw = Buffer.alloc((w + 1) * h);                    // greyscale, filter byte per row
  for (let y = 0; y < h; y++) {
    const sy = (y / scale) | 0;
    raw[y * (w + 1)] = 0;
    for (let x = 0; x < w; x++) raw[y * (w + 1) + 1 + x] = rgba[(sy * width + ((x / scale) | 0)) * 4];
  }
  const ihdr = Buffer.alloc(13);
  ihdr.writeUInt32BE(w, 0); ihdr.writeUInt32BE(h, 4); ihdr[8] = 8; ihdr[9] = 0; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
  return Buffer.concat([Buffer.from([0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A]), chunk('IHDR', ihdr), chunk('IDAT', zlib.deflateSync(raw, { level: 9 })), chunk('IEND', Buffer.alloc(0))]);
}

// ---- arguments ---------------------------------------------------------------------------
const args = process.argv.slice(2);
const opt = { machine: 'plus', rom: null, disks: [], frames: 600, screenshot: null, keys: null, keysAt: -1, traceTraps: false, cpu: null, ram: 0, pv: true, fast: false, at: [], saveDisk: null, quiet: false, scale: 1 };
for (let i = 0; i < args.length; i++) {
  const a = args[i];
  switch (a) {
    case '--machine': opt.machine = args[++i]; break;
    case '--rom': opt.rom = args[++i]; break;
    case '--disk': opt.disks.push({ path: args[++i], mode: 'auto' }); break;
    case '--scsi': opt.disks.push({ path: args[++i], mode: 'scsi' }); break;
    case '--frames': opt.frames = Number(args[++i]); break;
    case '--screenshot': opt.screenshot = args[++i]; break;
    case '--scale': opt.scale = Number(args[++i]); break;
    case '--keys': opt.keys = args[++i]; break;
    case '--keys-at': opt.keysAt = Number(args[++i]); break;
    case '--at': { const s = args[++i], k = s.indexOf(':'); opt.at.push({ frame: Number(s.slice(0, k)), action: s.slice(k + 1) }); break; }
    case '--trace-traps': opt.traceTraps = true; break;
    case '--cpu': opt.cpu = args[++i]; break;
    case '--ram': opt.ram = Number(args[++i]); break;
    case '--no-pv': opt.pv = false; break;
    case '--fast': opt.fast = true; break;
    case '--save-disk': opt.saveDisk = args[++i]; break;
    case '--quiet': opt.quiet = true; break;
    default: console.error('unknown option ' + a); process.exit(1);
  }
}
const def = MACHINES[opt.machine];
if (!def) { console.error('unknown machine ' + opt.machine); process.exit(1); }
if (!opt.rom && def.roms && def.roms.length) opt.rom = path.join(repoRoot, def.roms[0].path);
if (!opt.rom) { console.error('no ROM: use --rom'); process.exit(1); }

const unescape = (s) => s.replace(/\\n/g, '\n').replace(/\\t/g, '\t').replace(/\\e/g, '\u001b').replace(/\\b/g, '\b');
if (opt.keys !== null) opt.at.push({ frame: opt.keysAt >= 0 ? opt.keysAt : opt.frames >> 1, action: 'type=' + opt.keys });
opt.at.sort((a, b) => a.frame - b.frame);

const mac = createMachine({ machine: opt.machine, rom: new Uint8Array(fs.readFileSync(opt.rom)), ramMB: opt.ram || undefined, cpu: opt.cpu || undefined, pvDisk: opt.pv, fastBoot: opt.fast });
const images = [];
for (const d of opt.disks) {
  const bytes = new Uint8Array(fs.readFileSync(d.path));
  const img = mac.attachDisk(bytes, path.basename(d.path), d.mode);
  images.push(img);
  if (!opt.quiet) console.log(`disk ${path.basename(d.path)}: ${img.info.description} -> ${img.mode}` + (img.mode === 'scsi' ? ` id ${img.scsiId}` : ` (${img.info.volumes.map((v) => v.name || '?').join(', ')})`));
}
if (!opt.quiet) console.log(`${def.name}, CPU ${mac.cpu.model}, RAM ${mac.ramSize >> 20} MB, ROM ${path.basename(opt.rom)}` + (mac.romPatched >= 0 ? `, .Sony replaced at ROM+$${mac.romPatched.toString(16)}` : ', ROM unpatched'));

// ---- tracing ---------------------------------------------------------------------------------
const trapCount = new Map(), excCount = new Map();
let lastTraps = [];
if (opt.traceTraps) {
  mac.cpu.onALine = (op) => { trapCount.set(op, (trapCount.get(op) || 0) + 1); lastTraps.push(op); if (lastTraps.length > 40) lastTraps.shift(); return false; };
}
mac.cpu.onException = (vec) => { if (vec !== 10) excCount.set(vec, (excCount.get(vec) || 0) + 1); };

function shot(file) {
  const rgba = new Uint8Array(512 * 342 * 4);
  screenToRGBA(mac.screen(), rgba);
  fs.writeFileSync(file, encodePNG(512, 342, rgba, opt.scale));
  if (!opt.quiet) console.log(`frame ${mac.frames}: wrote ${file}`);
}

// ---- run ---------------------------------------------------------------------------------------
const keyQueue = [];      // { code, down }
let atIdx = 0, keyDelay = 0;
function doAction(action) {
  const eq = action.indexOf('='), name = eq < 0 ? action : action.slice(0, eq), val = eq < 0 ? '' : action.slice(eq + 1);
  switch (name) {
    case 'type': keyQueue.push(...typeString(unescape(val))); break;
    case 'key': { const c = parseInt(val, 16); keyQueue.push({ code: c, down: true }, { code: c, down: false }); break; }
    case 'keydown': mac.keyDown(parseInt(val, 16)); break;
    case 'keyup': mac.keyUp(parseInt(val, 16)); break;
    case 'move': { const [dx, dy] = val.split(',').map(Number); mac.mouseMove(dx, dy); break; }
    case 'mouseto': { const [x, y] = val.split(',').map(Number); mouseTarget = { x, y, frames: 4 }; break; }
    case 'click': mac.mouseButton(true); clickRelease = mac.frames + 6; break;
    case 'press': mac.mouseButton(true); break;
    case 'release': mac.mouseButton(false); break;
    case 'shot': shot(val); break;
    case 'nmi': mac.interruptButton(); break;
    case 'reset': mac.reset(); break;
    default: console.error('unknown action ' + action);
  }
}
let mouseTarget = null, clickRelease = -1;
const t0 = Date.now();
for (let f = 0; f < opt.frames; f++) {
  while (atIdx < opt.at.length && opt.at[atIdx].frame <= f) doAction(opt.at[atIdx++].action);
  if (keyQueue.length && --keyDelay <= 0) { const k = keyQueue.shift(); if (k.down) mac.keyDown(k.code); else mac.keyUp(k.code); keyDelay = 3; }
  if (mouseTarget) { mac.mouseTo(mouseTarget.x, mouseTarget.y); if (--mouseTarget.frames <= 0) mouseTarget = null; }
  if (clickRelease === f) mac.mouseButton(false);
  mac.runFrame();
  if (mac.cpu.halted) { console.log(`CPU halted (double fault) at frame ${f}`); break; }
}
const secs = (Date.now() - t0) / 1000;
if (opt.screenshot) shot(opt.screenshot);

const hex = (v, n = 8) => (v >>> 0).toString(16).toUpperCase().padStart(n, '0');
const c = mac.cpu;
console.log(`ran ${mac.frames} frames (${(mac.frames / 60.15).toFixed(1)} s emulated) in ${secs.toFixed(2)} s host = ${(mac.frames / 60.15 / secs).toFixed(1)}x real time, ${(c.insns / secs / 1e6).toFixed(1)} MIPS`);
console.log(`PC=$${hex(c.pc)} SR=$${hex(c.getSR(), 4)} SP=$${hex(c.d[15])} stopped=${c.stopped} halted=${c.halted} overlay=${mac.overlay}`);
console.log(`D: ${Array.from(c.d.subarray(0, 8), (v) => hex(v)).join(' ')}\nA: ${Array.from(c.d.subarray(8, 16), (v) => hex(v)).join(' ')}`);
if (excCount.size) console.log('exceptions: ' + [...excCount].sort((a, b) => a[0] - b[0]).map(([v, n]) => `v${v}x${n}`).join(' '));
console.log(`pvdisk: ${mac.pv.stats.reads} reads (${mac.pv.stats.bytesRead} bytes), ${mac.pv.stats.writes} writes (${mac.pv.stats.bytesWritten} bytes), opened=${mac.pv.opened}`);
for (let id = 0; id < 7; id++) { const t = mac.scsi.targets[id]; if (t) console.log(`scsi id ${id}: ${t.commands} commands, last: ${t.log.slice(-24).map((o) => hex(o, 2)).join(' ')}`); }
if (opt.traceTraps) {
  const top = [...trapCount].sort((a, b) => b[1] - a[1]).slice(0, 24);
  console.log('A-traps (top): ' + top.map(([op, n]) => `${hex(op, 4)}x${n}`).join(' '));
  console.log('last traps: ' + lastTraps.map((op) => hex(op, 4)).join(' '));
}
if (opt.saveDisk && images.length) { fs.writeFileSync(opt.saveDisk, images[0].bytes); console.log('saved ' + opt.saveDisk + (images[0].dirty ? ' (modified)' : ' (unmodified)')); }
