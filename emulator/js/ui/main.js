// main.js - browser front end: controls, display, input, sound. The machine itself
// runs in a Web Worker (worker.js); if module workers are unavailable it runs on
// the main thread through the same Engine class.

import { MACHINES, MACHINE_ORDER, identifyRom } from '../machines/table.js';
import { KEY_CODES, charToKey, KEY_SHIFT } from '../devices/keycodes.js';

const $ = (id) => document.getElementById(id);
const W = 512, H = 342;
const CPUS = ['68000', '68010', '68EC020', '68020', '68030', '68EC030', '68040', '68LC040', '68EC040'];
const DEFAULT_DISK = '../images/PlusOS.hda';
const q = new URLSearchParams(location.search);

const state = {
  engine: null, usingWorker: false, powered: false, paused: false, speed: 1, sound: false,
  roms: new Map(),          // key -> { label, bytes?: ArrayBuffer, path?: string, machine }
  disks: [],                // { name, bytes?: ArrayBuffer, path?: string, mode, missing?, info? }
  lastScreen: null, pressed: new Set(), dirty: [],
};

// ---- status / overlay -----------------------------------------------------------------------
function status(text, isError = false) { const s = $('status'); s.textContent = text; s.classList.toggle('err', isError); }
function overlay(text, showPower = false) {
  $('overlay').classList.toggle('hidden', text === null);
  if (text !== null) $('overlayText').textContent = text;
  $('overlayPower').hidden = !showPower;
}

// ---- display ------------------------------------------------------------------------------------
const canvas = $('screen'), ctx = canvas.getContext('2d', { alpha: false });
const imageData = ctx.createImageData(W, H), pixels = new Uint32Array(imageData.data.buffer);
function draw(screen) {
  let p = 0;
  for (let i = 0; i < screen.length; i++) {
    const b = screen[i];
    for (let m = 0x80; m; m >>= 1) pixels[p++] = b & m ? 0xFF1A1A1A : 0xFFF4F7F4;
  }
  ctx.putImageData(imageData, 0, 0);
}
function blank() { ctx.fillStyle = '#101010'; ctx.fillRect(0, 0, W, H); }
function layout() {
  const sel = Number($('scale').value);
  const fs = document.fullscreenElement === $('bezel');
  const embed = document.body.classList.contains('embed');
  const availW = fs || embed ? innerWidth : Math.min(innerWidth - 80, 1400), availH = fs || embed ? innerHeight : Math.max(342, innerHeight - 150);
  let s = sel || Math.max(1, Math.min(Math.floor(availW / W), Math.floor(availH / H)));
  if (innerWidth < W + 60 && !sel) s = Math.min(1, (innerWidth - 36) / W);      // phones: scale down smoothly
  canvas.style.width = Math.round(W * s) + 'px';
  canvas.style.height = Math.round(H * s) + 'px';
}
addEventListener('resize', layout);
document.addEventListener('fullscreenchange', layout);

// ---- sound -----------------------------------------------------------------------------------------
const SAMPLE_RATE = 22255;
let audioCtx = null, audioTime = 0;
function audioStart() {
  if (!audioCtx) { const AC = window.AudioContext || window.webkitAudioContext; if (!AC) return; audioCtx = new AC(); }
  if (audioCtx.state === 'suspended') audioCtx.resume();
}
function audioPush(samples) {
  if (!audioCtx || !state.sound || audioCtx.state !== 'running') return;
  const buf = audioCtx.createBuffer(1, samples.length, SAMPLE_RATE), ch = buf.getChannelData(0);
  for (let i = 0; i < samples.length; i++) ch[i] = samples[i] / 128;
  const now = audioCtx.currentTime;
  if (audioTime < now + 0.02 || audioTime > now + 0.4) audioTime = now + 0.06;      // (re)sync with a small cushion
  const src = audioCtx.createBufferSource();
  src.buffer = buf; src.connect(audioCtx.destination); src.start(audioTime);
  audioTime += samples.length / SAMPLE_RATE;
}

// ---- engine ----------------------------------------------------------------------------------------
function onEngineMessage(m) {
  switch (m.type) {
    case 'frame':
      state.lastScreen = m.screen;
      draw(m.screen);
      if (m.sound) audioPush(m.sound);
      $('led').classList.toggle('on', m.disk);
      $('readout').textContent = m.halted ? 'CPU halted' : `${m.fps.toFixed(0)} fps · ${m.mhz.toFixed(1)} MHz · ${(m.mhz / 7.8336 * 100).toFixed(0)}%`;
      if (m.dirty.some((d, i) => d !== state.dirty[i])) { state.dirty = m.dirty; renderDisks(); }
      break;
    case 'started': {
      const i = m.info;
      $('modelName').textContent = i.machine;
      status(`${i.machine} · ${i.cpu} · ${i.ramMB} MB · ` + (i.romPatched >= 0 ? 'paravirtual disk driver installed' : 'ROM unpatched (SCSI only)') +
        (i.disks.length ? ' · ' + i.disks.map((d) => `${d.name}: ${d.mode === 'scsi' ? 'SCSI ID ' + d.scsiId : (d.volumes.join(', ') || 'no volume found')}`).join('; ') : ' · no disk'));
      overlay(null);
      break;
    }
    case 'pram': try { localStorage.setItem('plusos-emu-pram-' + $('machine').value, Array.from(m.pram, (b) => b.toString(16).padStart(2, '0')).join('')); } catch (e) { /* private mode */ } break;
    case 'disk': download(new Blob([m.bytes]), m.name.replace(/(\.[^.]*)?$/, '-modified$1')); break;
    case 'error': status('Emulator error: ' + m.message, true); console.error(m.message); break;
  }
}
async function getEngine() {
  if (state.engine) return state.engine;
  if (q.get('worker') !== '0' && window.Worker) {
    try {
      const w = new Worker(new URL('./worker.js', import.meta.url), { type: 'module' });
      await new Promise((resolve, reject) => {
        const t = setTimeout(() => reject(new Error('worker timeout')), 4000);
        w.onmessage = (e) => { if (e.data.type === 'ready') { clearTimeout(t); resolve(); } };
        w.onerror = (e) => { clearTimeout(t); reject(new Error(e.message || 'worker failed')); };
      });
      w.onmessage = (e) => onEngineMessage(e.data);
      w.onerror = (e) => status('Worker error: ' + e.message, true);
      state.engine = { send: (msg, transfer) => w.postMessage(msg, transfer || []) };
      state.usingWorker = true;
      return state.engine;
    } catch (e) { console.warn('Web Worker unavailable, running on the main thread:', e.message); }
  }
  const { Engine } = await import('./engine.js');
  const eng = new Engine((msg) => onEngineMessage(msg));
  state.engine = { send: (msg) => eng.handle(msg) };
  return state.engine;
}
const send = (msg) => { if (state.engine && state.powered) state.engine.send(msg); };

// ---- configuration UI --------------------------------------------------------------------------------
function fillMachines() {
  const sel = $('machine');
  for (const id of MACHINE_ORDER) {
    const m = MACHINES[id], o = new Option(m.name + (m.status === 'planned' ? ' (planned)' : m.status === 'untested' ? ' (untested, needs your ROM)' : ''), id);
    if (m.status === 'planned') { o.disabled = true; o.style.color = '#999'; o.title = m.note || ''; }
    sel.add(o);
  }
  for (const c of CPUS) $('cpu').add(new Option(c, c));
  for (const id of MACHINE_ORDER) for (const r of MACHINES[id].roms || []) state.roms.set(r.path, { label: r.label, path: '../' + r.path, machine: id });
}
function machineChanged(keepCpu = false) {
  const m = MACHINES[$('machine').value];
  if (!keepCpu) $('cpu').value = m.cpu;
  const ram = $('ram'); ram.length = 0;
  for (const r of m.ramOptions) ram.add(new Option(r + ' MB', r));
  ram.value = q.get('ram') && m.ramOptions.includes(Number(q.get('ram'))) ? q.get('ram') : m.ramDefault;
  const rom = $('rom'); rom.length = 0;
  for (const [key, r] of state.roms) if (r.machine === m.id) rom.add(new Option(r.label, key));
  if (!rom.length) rom.add(new Option('— load a ROM file —', ''));
  $('modelName').textContent = m.name;
}
function renderDisks() {
  const box = $('disks'); box.textContent = '';
  if (!state.disks.length) { const d = document.createElement('div'); d.className = 'help'; d.textContent = 'No disk attached: the Mac will show the blinking “?” disk.'; box.append(d); }
  state.disks.forEach((d, i) => {
    const row = document.createElement('div'); row.className = 'disk';
    const name = document.createElement('b'); name.textContent = d.name;
    const info = document.createElement('small');
    info.textContent = d.missing ? 'not found' : d.bytes ? `${(d.bytes.byteLength / 1048576).toFixed(1)} MB` + (state.dirty[i] ? ' · modified' : '') : 'loads at power-on';
    const mode = document.createElement('select');
    mode.add(new Option('built-in driver', 'auto')); mode.add(new Option('SCSI (needs driver on disk)', 'scsi'));
    mode.value = d.mode; mode.title = 'How the image is attached (applies at next power-on)';
    mode.onchange = () => { d.mode = mode.value; };
    const dl = document.createElement('button'); dl.textContent = 'Download';
    dl.disabled = !state.powered; dl.title = 'Download the image including changes made by the emulated Mac';
    dl.onclick = () => send({ type: 'getdisk', index: state.disks.filter((x) => !x.missing).indexOf(d) });
    const rm = document.createElement('button'); rm.textContent = 'Remove';
    rm.onclick = () => { state.disks.splice(i, 1); renderDisks(); };
    row.append(name, info, mode, dl, rm);
    box.append(row);
  });
}
async function addFiles(files) {
  for (const f of files) {
    const bytes = await f.arrayBuffer();
    const u8 = new Uint8Array(bytes);
    const looksLikeRom = /\.rom$/i.test(f.name) || ([0x10000, 0x20000, 0x40000, 0x80000, 0x100000].includes(u8.length) && u8.length <= 0x100000 && !/\.(dsk|img|hda|image|dc42)$/i.test(f.name));
    if (looksLikeRom) {
      const id = identifyRom(u8);
      const key = 'upload:' + f.name;
      state.roms.set(key, { label: `${f.name} (${id.label}, ${id.checksum})`, bytes, machine: id.id || $('machine').value });
      if (id.id && MACHINES[id.id].status !== 'planned') $('machine').value = id.id;
      machineChanged();
      $('rom').value = key;
      status(`ROM ${f.name}: ${id.label}, checksum ${id.checksum}`);
    } else {
      state.disks.push({ name: f.name, bytes, mode: 'auto' });
      status(`Disk ${f.name} added; it is attached at the next power-on.`);
    }
  }
  renderDisks();
}
function download(blob, name) {
  const a = document.createElement('a');
  a.href = URL.createObjectURL(blob); a.download = name; document.body.append(a); a.click(); a.remove();
  setTimeout(() => URL.revokeObjectURL(a.href), 5000);
}
async function fetchBytes(path) {
  const r = await fetch(path, { cache: 'no-store' });
  if (!r.ok) throw new Error(`${path}: HTTP ${r.status}`);
  return r.arrayBuffer();
}

// ---- power ----------------------------------------------------------------------------------------------
async function powerOn() {
  try {
    const machine = $('machine').value, def = MACHINES[machine];
    const romEntry = state.roms.get($('rom').value);
    if (!romEntry) { status(`No ROM for the ${def.name}: use “Load ROM file…”.`, true); overlay('No ROM image', false); return; }
    overlay('Loading ROM…');
    if (!romEntry.bytes) romEntry.bytes = await fetchBytes(romEntry.path);
    const disks = [];
    for (const d of state.disks) {
      if (!d.bytes && d.path && !d.missing) {
        overlay(`Loading ${d.name}…`);
        try { d.bytes = await fetchBytes(d.path); } catch (e) { d.missing = true; status(`${d.name} not found (${e.message}); starting without it.`, true); }
      }
      if (d.bytes) disks.push({ name: d.name, bytes: d.bytes, mode: d.mode });
    }
    renderDisks();
    let pram = null;
    try { const s = localStorage.getItem('plusos-emu-pram-' + machine); if (s && s.length === 512) pram = Uint8Array.from(s.match(/../g), (h) => parseInt(h, 16)); } catch (e) { /* ignore */ }
    const engine = await getEngine();
    state.powered = true; state.paused = false; state.dirty = [];
    engine.send({ type: 'start', config: { machine, cpu: $('cpu').value, ramMB: Number($('ram').value), rom: romEntry.bytes, disks, pram, fastBoot: $('fast').checked, speed: state.speed, sound: state.sound } });
    setPowerUI();
    canvas.focus();
  } catch (e) {
    status('Could not start: ' + e.message, true);
    overlay('Could not start', true);
  }
}
function powerOff() {
  if (state.engine) state.engine.send({ type: 'stop' });
  state.powered = false;
  blank(); overlay('Power is off', true);
  $('readout').textContent = 'off'; $('led').classList.remove('on');
  setPowerUI();
}
function setPowerUI() {
  $('power').textContent = state.powered ? 'Power off' : 'Power on';
  for (const id of ['reset', 'nmi', 'pause']) $(id).disabled = !state.powered;
  $('pause').textContent = state.paused ? 'Resume' : 'Pause';
  renderDisks();
}

// ---- input ------------------------------------------------------------------------------------------------
function keyEvent(e, down) {
  if (!state.powered || document.activeElement !== canvas) return;
  const code = KEY_CODES[e.code];
  if (code === undefined) return;
  // leave browser/OS shortcuts that cannot be delivered anyway
  if (e.code === 'F11' || e.code === 'F12' || (e.code === 'Escape' && document.pointerLockElement)) return;
  e.preventDefault();
  if (down) { if (state.pressed.has(code) && e.repeat) return; state.pressed.add(code); send({ type: 'keydown', code }); }
  else { state.pressed.delete(code); send({ type: 'keyup', code }); }
}
function releaseAll() { for (const code of state.pressed) send({ type: 'keyup', code }); state.pressed.clear(); }
addEventListener('keydown', (e) => keyEvent(e, true));
addEventListener('keyup', (e) => keyEvent(e, false));
addEventListener('blur', releaseAll);
canvas.addEventListener('blur', releaseAll);

function canvasPos(e) {
  const r = canvas.getBoundingClientRect();
  return { x: Math.floor((e.clientX - r.left) * W / r.width), y: Math.floor((e.clientY - r.top) * H / r.height) };
}
const locked = () => document.pointerLockElement === canvas;
let lockRemainder = { x: 0, y: 0 };
canvas.addEventListener('pointerdown', (e) => {
  audioStart();
  canvas.focus();
  if (!state.powered) return;
  if (e.pointerType === 'mouse' && $('mouseMode').value === 'lock' && !locked()) { canvas.requestPointerLock(); return; }
  if (!locked()) { const p = canvasPos(e); send({ type: 'mouseto', x: p.x, y: p.y }); canvas.setPointerCapture(e.pointerId); }
  // give the cursor one frame to arrive before the button goes down (touch taps)
  if (e.pointerType === 'mouse') send({ type: 'mousebutton', down: true });
  else setTimeout(() => send({ type: 'mousebutton', down: true }), 40);
  e.preventDefault();
});
canvas.addEventListener('pointermove', (e) => {
  if (!state.powered) return;
  if (locked()) {
    const r = canvas.getBoundingClientRect(), k = W / r.width;
    lockRemainder.x += e.movementX * k; lockRemainder.y += e.movementY * k;
    const dx = Math.trunc(lockRemainder.x), dy = Math.trunc(lockRemainder.y);
    lockRemainder.x -= dx; lockRemainder.y -= dy;
    if (dx || dy) send({ type: 'mousemove', dx, dy });
  } else if ($('mouseMode').value === 'abs' || e.pointerType !== 'mouse') {
    const p = canvasPos(e); send({ type: 'mouseto', x: p.x, y: p.y });
  }
});
const pointerUp = (e) => { if (state.powered) setTimeout(() => send({ type: 'mousebutton', down: false }), e.pointerType === 'mouse' ? 0 : 90); };
canvas.addEventListener('pointerup', pointerUp);
canvas.addEventListener('pointercancel', pointerUp);
canvas.addEventListener('contextmenu', (e) => e.preventDefault());
document.addEventListener('pointerlockchange', () => canvas.classList.toggle('locked', locked()));
$('mouseMode').onchange = () => { canvas.classList.toggle('hidecursor', false); if (locked()) document.exitPointerLock(); };

// soft keyboard (touch devices): a hidden text field collects characters
const soft = $('softkbd');
function tap(code, shift) {
  if (shift) send({ type: 'keydown', code: KEY_SHIFT });
  send({ type: 'keydown', code });
  setTimeout(() => { send({ type: 'keyup', code }); if (shift) send({ type: 'keyup', code: KEY_SHIFT }); }, 60);
}
soft.addEventListener('input', () => {
  const text = soft.value; soft.value = '';
  let delay = 0;
  for (const ch of text) { const k = charToKey(ch); if (k) { setTimeout(() => tap(k[0], k[1]), delay); delay += 130; } }
});
soft.addEventListener('keydown', (e) => {
  if (e.key === 'Enter') { tap(0x24); e.preventDefault(); } else if (e.key === 'Backspace') { tap(0x33); e.preventDefault(); }
});
$('kbd').onclick = () => { soft.focus(); };
for (const b of document.querySelectorAll('button[data-key]')) {
  b.onclick = () => {
    const code = parseInt(b.dataset.key, 16);
    if (b.dataset.toggle) { const on = b.classList.toggle('on'); send({ type: on ? 'keydown' : 'keyup', code }); }
    else tap(code);
    if (document.activeElement !== soft) canvas.focus();
  };
}

// ---- controls -------------------------------------------------------------------------------------------
$('power').onclick = () => { audioStart(); state.powered ? powerOff() : powerOn(); };
$('overlayPower').onclick = () => { audioStart(); powerOn(); };
$('reset').onclick = () => { send({ type: 'reset' }); canvas.focus(); };
$('nmi').onclick = () => { send({ type: 'nmi' }); canvas.focus(); };
$('pause').onclick = () => { state.paused = !state.paused; send({ type: 'pause', paused: state.paused }); setPowerUI(); };
for (const b of document.querySelectorAll('button[data-speed]')) {
  b.onclick = () => {
    state.speed = Number(b.dataset.speed);
    document.querySelectorAll('button[data-speed]').forEach((x) => x.classList.toggle('on', x === b));
    send({ type: 'speed', speed: state.speed });
  };
}
$('sound').onclick = () => {
  state.sound = !state.sound;
  if (state.sound) audioStart();
  $('sound').textContent = 'Sound: ' + (state.sound ? 'on' : 'off');
  $('sound').classList.toggle('on', state.sound);
  send({ type: 'sound', on: state.sound });
};
$('fullscreen').onclick = () => { const b = $('bezel'); if (document.fullscreenElement) document.exitFullscreen(); else if (b.requestFullscreen) b.requestFullscreen(); canvas.focus(); };
$('shot').onclick = () => canvas.toBlob((blob) => blob && download(blob, 'plusos-screen.png'), 'image/png');
$('scale').onchange = layout;
$('machine').onchange = () => machineChanged();
$('romFile').onchange = (e) => { addFiles(e.target.files); e.target.value = ''; };
$('diskFile').onchange = (e) => { addFiles(e.target.files); e.target.value = ''; };
addEventListener('dragover', (e) => { e.preventDefault(); $('mac').classList.add('drop'); });
addEventListener('dragleave', () => $('mac').classList.remove('drop'));
addEventListener('drop', (e) => { e.preventDefault(); $('mac').classList.remove('drop'); if (e.dataTransfer.files.length) addFiles(e.dataTransfer.files); });
addEventListener('beforeunload', (e) => { if (state.dirty.some(Boolean)) { e.preventDefault(); e.returnValue = ''; } });

// ---- start-up ------------------------------------------------------------------------------------------------
fillMachines();
if (q.get('machine') && MACHINES[q.get('machine')] && MACHINES[q.get('machine')].status !== 'planned') $('machine').value = q.get('machine');
machineChanged();
if (q.get('cpu') && CPUS.includes(q.get('cpu').toUpperCase())) $('cpu').value = q.get('cpu').toUpperCase();
if (q.get('rom')) { state.roms.set('url', { label: q.get('rom'), path: q.get('rom'), machine: $('machine').value }); machineChanged(true); $('rom').value = 'url'; }
if (q.get('speed') !== null && ['0', '1', '2'].includes(q.get('speed'))) { state.speed = Number(q.get('speed')); document.querySelectorAll('button[data-speed]').forEach((x) => x.classList.toggle('on', x.dataset.speed === q.get('speed'))); }
if (q.get('fast') === '0') $('fast').checked = false;
if (q.get('scale')) $('scale').value = q.get('scale');
if (q.get('mouse') === 'lock') $('mouseMode').value = 'lock';
if (q.get('embed') === '1') document.body.classList.add('embed');
const diskParams = q.getAll('disk');
if (diskParams.length) { for (const d of diskParams) if (d && d !== 'none') state.disks.push({ name: d.split('/').pop(), path: d, mode: q.get('scsi') === '1' ? 'scsi' : 'auto' }); }
else state.disks.push({ name: 'PlusOS.hda', path: DEFAULT_DISK, mode: 'auto' });
renderDisks();
layout();
blank();
overlay('Power is off', true);
status('Ready. Choose a model and press Power on.');
if (q.get('autostart') === '1') powerOn();
