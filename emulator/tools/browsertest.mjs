#!/usr/bin/env node
// browsertest.mjs - load the emulator page in headless Chrome/Edge through the DevTools
// protocol (no dependencies; needs Node >= 22 for the built-in WebSocket), wait, optionally
// type text, save a screenshot and report console errors.
//
//   node browsertest.mjs --url http://localhost:8068/emulator/?autostart=1 [--wait 25] [--out page.png]
//        [--type "root\n" --type-at 12] [--chrome PATH] [--width 900 --height 900]
// Exit status 1 if the page logged an error or threw an exception.

import { spawn } from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';

const args = process.argv.slice(2);
const opt = { url: 'http://localhost:8068/emulator/?autostart=1', wait: 25, out: null, chrome: null, width: 900, height: 900, types: [], port: 9333 };
for (let i = 0; i < args.length; i++) {
  const a = args[i];
  if (a === '--url') opt.url = args[++i];
  else if (a === '--wait') opt.wait = Number(args[++i]);
  else if (a === '--out') opt.out = args[++i];
  else if (a === '--chrome') opt.chrome = args[++i];
  else if (a === '--width') opt.width = Number(args[++i]);
  else if (a === '--height') opt.height = Number(args[++i]);
  else if (a === '--type') opt.types.push({ text: args[++i], at: 0 });
  else if (a === '--type-at') opt.types[opt.types.length - 1].at = Number(args[++i]);
  else if (a === '--port') opt.port = Number(args[++i]);
}
const candidates = [opt.chrome, 'C:/Program Files/Google/Chrome/Application/chrome.exe', 'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe',
  '/usr/bin/google-chrome', '/usr/bin/chromium', '/usr/bin/chromium-browser'].filter(Boolean);
const exe = candidates.find((p) => fs.existsSync(p));
if (!exe) { console.error('no Chrome/Edge found; use --chrome'); process.exit(2); }

const profile = fs.mkdtempSync(path.join(os.tmpdir(), 'plusos-emu-chrome-'));
const child = spawn(exe, ['--headless=new', `--remote-debugging-port=${opt.port}`, `--user-data-dir=${profile}`, '--no-first-run', '--mute-audio',
  '--autoplay-policy=no-user-gesture-required', `--window-size=${opt.width},${opt.height}`, 'about:blank'], { stdio: 'ignore' });
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

let target = null;
for (let i = 0; i < 50 && !target; i++) {
  await sleep(200);
  try { const list = await (await fetch(`http://127.0.0.1:${opt.port}/json`)).json(); target = list.find((t) => t.type === 'page'); } catch (e) { /* not up yet */ }
}
if (!target) { console.error('could not connect to the browser'); child.kill(); process.exit(2); }
const ws = new WebSocket(target.webSocketDebuggerUrl);
await new Promise((r) => { ws.onopen = r; });
let id = 0;
const pending = new Map(), errors = [], logs = [];
ws.onmessage = (ev) => {
  const m = JSON.parse(ev.data);
  if (m.id && pending.has(m.id)) { pending.get(m.id)(m.result || m.error); pending.delete(m.id); return; }
  if (m.method === 'Runtime.exceptionThrown') errors.push('exception: ' + (m.params.exceptionDetails.exception?.description || m.params.exceptionDetails.text));
  if (m.method === 'Runtime.consoleAPICalled') {
    const text = m.params.args.map((a) => a.value ?? a.description ?? '').join(' ');
    (m.params.type === 'error' ? errors : logs).push(`console.${m.params.type}: ${text}`);
  }
  if (m.method === 'Log.entryAdded' && m.params.entry.level === 'error') errors.push(`log: ${m.params.entry.text} ${m.params.entry.url || ''}`);
};
const cdp = (method, params = {}) => new Promise((resolve) => { const n = ++id; pending.set(n, resolve); ws.send(JSON.stringify({ id: n, method, params })); });
await cdp('Runtime.enable'); await cdp('Log.enable'); await cdp('Page.enable');
await cdp('Emulation.setDeviceMetricsOverride', { width: opt.width, height: opt.height, deviceScaleFactor: 1, mobile: false });
await cdp('Page.navigate', { url: opt.url });
const t0 = Date.now();
opt.types.sort((a, b) => a.at - b.at);
for (const t of opt.types) {
  await sleep(Math.max(0, t.at * 1000 - (Date.now() - t0)));
  await cdp('Runtime.evaluate', { expression: 'document.getElementById("screen").focus()' });
  for (const ch of t.text.replace(/\\n/g, '\n')) {
    const isEnter = ch === '\n';
    const key = isEnter ? 'Enter' : ch, code = isEnter ? 'Enter' : /[a-z]/i.test(ch) ? 'Key' + ch.toUpperCase() : /[0-9]/.test(ch) ? 'Digit' + ch : ch === ' ' ? 'Space' : '';
    const shift = /[A-Z]/.test(ch);
    if (!code) continue;
    if (shift) await cdp('Input.dispatchKeyEvent', { type: 'rawKeyDown', key: 'Shift', code: 'ShiftLeft', modifiers: 8 });
    await cdp('Input.dispatchKeyEvent', { type: 'rawKeyDown', key, code, modifiers: shift ? 8 : 0 });
    await sleep(70);
    await cdp('Input.dispatchKeyEvent', { type: 'keyUp', key, code, modifiers: shift ? 8 : 0 });
    if (shift) await cdp('Input.dispatchKeyEvent', { type: 'keyUp', key: 'Shift', code: 'ShiftLeft' });
    await sleep(70);
  }
}
await sleep(Math.max(0, opt.wait * 1000 - (Date.now() - t0)));
const info = await cdp('Runtime.evaluate', { expression: 'JSON.stringify({status: document.getElementById("status").textContent, readout: document.getElementById("readout").textContent, title: document.title})', returnByValue: true });
console.log('page state:', info.result?.value);
if (opt.out) {
  const shot = await cdp('Page.captureScreenshot', { format: 'png' });
  fs.writeFileSync(opt.out, Buffer.from(shot.data, 'base64'));
  console.log('wrote ' + opt.out);
}
for (const l of logs) console.log(l);
for (const e of errors) console.log('ERROR ' + e);
console.log(errors.length ? `${errors.length} error(s)` : 'no console errors');
ws.close(); child.kill();
await sleep(300);
try { fs.rmSync(profile, { recursive: true, force: true }); } catch (e) { /* profile still locked: leave it in the temp dir */ }
process.exit(errors.length ? 1 : 0);
