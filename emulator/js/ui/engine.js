// engine.js - runs a machine in real time. No DOM access: used inside the Web
// Worker (worker.js) and, as a fallback, directly on the main thread.
//
// post(message, transferList) delivers messages to the UI:
//   { type: 'frame', screen: Uint8Array(21888), sound: Int8Array, fps, mhz, frames, disk }
//   { type: 'started', info } { type: 'pram', pram } { type: 'disk', index, name, bytes } { type: 'error', message }

import { createMachine } from '../machines/index.js';

const FRAME_HZ = 60.147;

export class Engine {
  constructor(post) {
    this.post = post;
    this.mac = null;
    this.running = false;
    this.speed = 1;            // 1, 2, ... or 0 = as fast as possible
    this.timer = null;
    this.soundBuf = [];
    this.soundEnabled = false;
    this.diskActivity = 0;
  }

  handle(msg) {
    try {
      switch (msg.type) {
        case 'start': this.start(msg.config); break;
        case 'stop': this.stop(); break;
        case 'pause': this.running = !msg.paused; if (this.running) this.resume(); break;
        case 'reset': if (this.mac) this.mac.reset(); break;
        case 'nmi': if (this.mac) this.mac.interruptButton(); break;
        case 'speed': this.speed = msg.speed; this.resume(); break;
        case 'sound': this.soundEnabled = !!msg.on; break;
        case 'keydown': if (this.mac) this.mac.keyDown(msg.code); break;
        case 'keyup': if (this.mac) this.mac.keyUp(msg.code); break;
        case 'mousemove': if (this.mac) this.mac.mouseMove(msg.dx, msg.dy); break;
        case 'mouseto': if (this.mac) this.mac.mouseTo(msg.x, msg.y); break;
        case 'mousebutton': if (this.mac) this.mac.mouseButton(msg.down); break;
        case 'getdisk': this.sendDisk(msg.index); break;
      }
    } catch (e) {
      this.post({ type: 'error', message: String(e && e.stack || e) });
    }
  }

  start(cfg) {
    this.stop();
    const mac = createMachine({ machine: cfg.machine, rom: new Uint8Array(cfg.rom), ramMB: cfg.ramMB, cpu: cfg.cpu, pvDisk: cfg.pvDisk !== false,
      fastBoot: cfg.fastBoot !== false, pram: cfg.pram ? new Uint8Array(cfg.pram) : null });
    const disks = [];
    for (const d of cfg.disks || []) {
      const img = mac.attachDisk(new Uint8Array(d.bytes), d.name, d.mode || 'auto', !!d.readOnly);
      disks.push({ name: d.name, mode: img.mode, description: img.info.description, volumes: img.info.volumes.map((v) => v.name), scsiId: img.scsiId });
    }
    mac.onPramWrite = (pram) => { this.pramDirty = true; };
    mac.pv.onActivity = () => { this.diskActivity = 6; };
    this.mac = mac;
    this.speed = cfg.speed === undefined ? 1 : cfg.speed;
    this.soundEnabled = !!cfg.sound;
    this.running = true;
    this.post({ type: 'started', info: { machine: mac.def.name, cpu: mac.cpu.model, ramMB: mac.ramSize >> 20, romPatched: mac.romPatched, disks } });
    this.resume();
  }

  stop() { this.running = false; if (this.timer !== null) { clearTimeout(this.timer); this.timer = null; } this.mac = null; }

  resume() {
    if (this.timer !== null) clearTimeout(this.timer);
    this.t0 = performance.now(); this.f0 = this.mac ? this.mac.frames : 0;
    this.statT = this.t0; this.statF = this.f0; this.statC = this.mac ? this.mac.cpu.cycles : 0;
    this.fps = 0; this.mhz = 0;
    if (this.running && this.mac) this.tick();
  }

  tick() {
    this.timer = null;
    if (!this.running || !this.mac) return;
    const mac = this.mac, start = performance.now();
    let n;
    if (this.speed === 0) n = 1000;                                        // limited by the time budget below
    else {
      n = Math.floor((start - this.t0) / 1000 * FRAME_HZ * this.speed) - (mac.frames - this.f0);
      if (n > 12) { this.t0 = start; this.f0 = mac.frames; n = 2; }       // fell behind (tab was hidden): resync
    }
    let ran = 0;
    try {
      while (ran < n) {
        mac.runFrame();
        ran++;
        if (this.soundEnabled && this.speed === 1) this.collectSound(mac);
        if (performance.now() - start > 14) break;
      }
    } catch (e) {
      this.running = false;
      this.post({ type: 'error', message: String(e && e.stack || e) });
      return;
    }
    const now = performance.now();
    if (now - this.statT >= 500) {
      this.fps = (mac.frames - this.statF) * 1000 / (now - this.statT);
      this.mhz = (mac.cpu.cycles - this.statC) / (now - this.statT) / 1000;
      this.statT = now; this.statF = mac.frames; this.statC = mac.cpu.cycles;
    }
    if (ran > 0) {
      const screen = mac.screen().slice();
      let sound = null;
      if (this.soundBuf.length) { sound = Int8Array.from(this.soundBuf); this.soundBuf.length = 0; }
      if (this.diskActivity > 0) this.diskActivity--;
      const msg = { type: 'frame', screen, sound, fps: this.fps, mhz: this.mhz, frames: mac.frames, disk: this.diskActivity > 0,
        halted: mac.cpu.halted !== 0, dirty: mac.images.map((i) => i.dirty) };
      this.post(msg, sound ? [screen.buffer, sound.buffer] : [screen.buffer]);
      if (this.pramDirty) { this.pramDirty = false; this.post({ type: 'pram', pram: mac.rtc.pram.slice() }); }
    }
    this.timer = setTimeout(() => this.tick(), this.speed === 0 ? 0 : Math.max(1, 8 - (performance.now() - start)));
  }

  // 370 samples per frame (one per scan line, 22.25 kHz), signed 8 bit, volume applied
  collectSound(mac) {
    const s = mac.sound, out = this.soundBuf;
    if (!mac.soundOn) { for (let i = 0; i < s.length; i++) out.push(0); return; }
    const vol = (mac.soundVolume + 1) / 8;
    for (let i = 0; i < s.length; i++) out.push(Math.round((s[i] - 128) * vol));
  }

  sendDisk(index) {
    const img = this.mac && this.mac.images[index];
    if (!img) return;
    const bytes = img.bytes.slice();
    this.post({ type: 'disk', index, name: img.name, bytes }, [bytes.buffer]);
  }
}
