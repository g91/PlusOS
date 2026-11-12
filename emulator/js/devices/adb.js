// adb.js - Apple Desktop Bus as seen by the Macintosh SE / Classic: the ADB
// transceiver (a microcontroller between the VIA and the bus) plus a keyboard
// and a mouse device.
//
// VIA interface: PB4/PB5 = state lines ST0/ST1 (outputs), PB3 = /INT (input),
// data through the VIA shift register clocked by the transceiver.
//   state 0  the Mac has loaded a command byte into the shift register
//   state 1  transfer an even data byte (first, third, ...)
//   state 2  transfer an odd data byte
//   state 3  idle; the transceiver auto-polls the last Talk command
// Every byte transfer ends with a shift-register interrupt. /INT low on a data
// byte means "no (more) data" (bus timeout); on an odd byte it flags a service
// request from another device.
//
// ADB command byte: aaaa cc rr  (address, 00=reset/flush 10=listen 11=talk, register)

class ADBDevice {
  constructor(addr, handler) { this.defAddr = addr; this.defHandler = handler; this.reset(); }
  reset() { this.addr = this.defAddr; this.handler = this.defHandler; }
  hasData() { return false; }
  talk(reg) {
    if (reg === 3) return [0x60 | ((Math.random() * 16) & 0x0F), this.handler];
    return null;
  }
  listen(reg, data) {
    if (reg === 3 && data.length >= 2) {
      const h = data[1];
      if (h === 0xFE || h === 0x00) this.addr = data[0] & 0x0F;
      else if (h !== 0xFD && h !== 0xFF && this.acceptHandler(h)) this.handler = h;
    }
  }
  acceptHandler() { return false; }
  flush() { }
}

class ADBKeyboard extends ADBDevice {
  constructor() { super(2, 2); this.queue = []; }
  reset() { super.reset(); if (this.queue) this.queue.length = 0; }
  flush() { this.queue.length = 0; }
  hasData() { return this.queue.length > 0; }
  key(code, up) { if (this.queue.length < 64) this.queue.push((code & 0x7F) | (up ? 0x80 : 0)); }
  acceptHandler(h) { return h === 2 || h === 3; }
  talk(reg) {
    if (reg === 0) {
      if (!this.queue.length) return null;
      const a = this.queue.shift();
      const b = this.queue.length ? this.queue.shift() : 0xFF;
      return [a, b];
    }
    if (reg === 2) return [0xFF, 0xFF];           // modifier / LED register: nothing pressed
    return super.talk(reg);
  }
}

class ADBMouse extends ADBDevice {
  constructor() { super(3, 1); this.dx = 0; this.dy = 0; this.down = false; this.reported = false; }
  hasData() { return this.dx !== 0 || this.dy !== 0 || this.down !== this.reported; }
  flush() { this.dx = this.dy = 0; }
  acceptHandler(h) { return h === 1 || h === 2; }
  talk(reg) {
    if (reg === 0) {
      if (!this.hasData()) return null;
      const cx = Math.max(-63, Math.min(63, this.dx)), cy = Math.max(-63, Math.min(63, this.dy));
      this.dx -= cx; this.dy -= cy;
      this.reported = this.down;
      return [(this.down ? 0 : 0x80) | (cy & 0x7F), 0x80 | (cx & 0x7F)];
    }
    return super.talk(reg);
  }
}

const T_BYTE = 2600;          // CPU cycles for one byte on the bus (~330 us)
const T_AUTOPOLL = 86000;     // auto-poll interval (~11 ms)

export class ADBTransceiver {
  constructor(machine, via) {
    this.m = machine; this.via = via;
    this.kbd = new ADBKeyboard();
    this.mouse = new ADBMouse();
    this.devices = [this.kbd, this.mouse];
    this.reset();
  }
  reset() {
    for (const d of this.devices) d.reset();
    this.st = 3; this.intLow = false; this.lastCmd = 0; this.reply = null; this.pos = 0;
    this.listenBuf = null; this.listenCmd = 0; this.pollArmed = false; this.gen = 0;
  }

  // ---- VIA hooks ----------------------------------------------------------------------
  srWrite() { }                                   // the byte is picked up when the state lines change
  readPB(v) { return this.intLow ? v & ~0x08 : v | 0x08; }
  writePB(pins) {
    const st = (pins >> 4) & 3;
    if (st === this.st) return;
    this.st = st;
    this.gen++;
    const gen = this.gen;
    switch (st) {
      case 0: this.m.schedule(T_BYTE, () => { if (gen === this.gen) this.command(this.via.sr); }); break;
      case 1: case 2: this.m.schedule(T_BYTE, () => { if (gen === this.gen) this.dataByte(st); }); break;
      default: this.endTransaction(); this.armAutopoll(); break;
    }
  }

  device(addr) { return this.devices.find((d) => d.addr === addr) || null; }
  srqFrom(except) { return this.devices.some((d) => d !== except && d.hasData()); }

  command(cmd) {
    this.endTransaction();
    this.lastCmd = cmd;
    this.reply = null; this.pos = 0; this.listenBuf = null;
    const addr = cmd >> 4, type = (cmd >> 2) & 3, reg = cmd & 3, dev = this.device(addr);
    if ((cmd & 0x0F) === 0) { for (const d of this.devices) d.reset(); }           // SendReset
    else if (type === 0 && reg === 1) { if (dev) dev.flush(); }                     // Flush
    else if (type === 3) { if (dev) this.reply = dev.talk(reg); }                   // Talk
    else if (type === 2) { this.listenBuf = []; this.listenCmd = cmd; }             // Listen
    this.intLow = false;
    this.via.shiftDone();
  }

  dataByte(st) {
    if (this.listenBuf) {
      if (this.listenBuf.length < 8) this.listenBuf.push(this.via.sr);
      this.intLow = false;
      this.via.shiftDone();
      return;
    }
    if (this.reply && this.pos < this.reply.length) {
      const b = this.reply[this.pos++];
      // odd byte: flag a service request from another device
      this.intLow = st === 2 && this.srqFrom(this.device(this.lastCmd >> 4));
      this.via.shiftIn(b);
    } else {
      // timeout: no (more) data. /INT low on the even byte reports it; on the odd byte it would mean SRQ.
      this.intLow = st === 1 ? true : this.srqFrom(this.device(this.lastCmd >> 4));
      this.via.shiftIn(0xFF);
    }
  }

  endTransaction() {
    if (this.listenBuf) {
      const dev = this.device(this.listenCmd >> 4);
      if (dev && this.listenBuf.length) dev.listen(this.listenCmd & 3, this.listenBuf);
      this.listenBuf = null;
    }
    this.intLow = false;
  }

  // Idle: re-issue the last Talk command until some device has something to say.
  armAutopoll() {
    if (this.pollArmed) return;
    this.pollArmed = true;
    this.m.schedule(T_AUTOPOLL, () => this.autopoll());
  }
  autopoll() {
    this.pollArmed = false;
    if (this.st !== 3) return;
    if (((this.lastCmd >> 2) & 3) === 3 && this.devices.some((d) => d.hasData())) {
      const dev = this.device(this.lastCmd >> 4);
      this.reply = dev ? dev.talk(this.lastCmd & 3) : null;
      this.pos = 0; this.listenBuf = null;
      this.intLow = false;
      this.via.shiftIn(this.lastCmd);
      return;                                     // the Mac now walks through states 1/2 and back to 3
    }
    this.armAutopoll();
  }

  // ---- host input -------------------------------------------------------------------------
  keyDown(code) { this.kbd.key(code, false); }
  keyUp(code) { this.kbd.key(code, true); }
  mouseButton(down) { this.mouse.down = down; }
  mouseMove(dx, dy) { this.mouse.dx += dx; this.mouse.dy += dy; }
}
