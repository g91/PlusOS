// scc.js - Zilog 8530 Serial Communications Controller, reduced to what the
// compact Macs need without serial peripherals: register file, reset, and
// external/status interrupts on the DCD inputs (the Plus mouse X1/Y1 lines).
// The transmitter is always "empty"; nothing is ever received.
//
// Channel index: 0 = A (modem port, mouse X1 on DCD), 1 = B (printer port, mouse Y1 on DCD).

class Channel {
  constructor() { this.wr = new Uint8Array(16); this.dcd = 0; this.reset(); }
  reset() { this.wr.fill(0); this.ptr = 0; this.latched = false; this.rr0Latch = 0; this.extIP = false; }
  rr0() { return 0x04 | 0x10 | 0x40 | (this.dcd ? 0x08 : 0); }       // tx buffer empty, sync/hunt (idle line), tx underrun/EOM, DCD
}

export class SCC8530 {
  constructor(host) {
    this.host = host;                 // { irq(state) }
    this.ch = [new Channel(), new Channel()];
    this.irqState = false;
  }
  reset() { this.ch[0].reset(); this.ch[1].reset(); this.updateIRQ(); }

  updateIRQ() {
    const mie = (this.ch[0].wr[9] & 0x08) !== 0;
    const s = mie && ((this.ch[0].extIP && (this.ch[0].wr[1] & 1)) || (this.ch[1].extIP && (this.ch[1].wr[1] & 1))) ? true : false;
    if (s !== this.irqState) { this.irqState = s; if (this.host.irq) this.host.irq(s); }
  }

  // DCD input change (mouse quadrature). Returns false if the previous change has not been serviced yet.
  setDCD(c, level) {
    const ch = this.ch[c];
    level = level ? 1 : 0;
    if (ch.dcd === level) return true;
    if (ch.latched) return false;
    ch.dcd = level;
    if (ch.wr[15] & 0x08) {
      ch.rr0Latch = ch.rr0();
      if (ch.wr[1] & 1) { ch.latched = true; ch.extIP = true; this.updateIRQ(); }
    }
    return true;
  }
  canChangeDCD(c) { return !this.ch[c].latched; }

  readCtl(c) {
    const ch = this.ch[c], reg = ch.ptr;
    ch.ptr = 0;
    switch (reg) {
      case 0: return ch.latched ? ch.rr0Latch : ch.rr0();
      case 1: return 0x07;                                       // all sent, residue codes
      case 2:
        if (c === 0) return ch.wr[2];
        {
          // modified vector (status low): which interrupt is pending
          let code = 3;
          if (this.ch[0].extIP && (this.ch[0].wr[1] & 1)) code = 5;
          else if (this.ch[1].extIP && (this.ch[1].wr[1] & 1)) code = 1;
          const v = this.ch[0].wr[2];
          return (this.ch[0].wr[9] & 0x10) ? (v & 0x8F) | (((code & 1) << 2 | (code & 2) | (code >> 2)) << 4) : (v & 0xF1) | (code << 1);
        }
      case 3: return c === 0 ? ((this.ch[0].extIP ? 0x08 : 0) | (this.ch[1].extIP ? 0x01 : 0)) : 0;
      case 8: return 0;
      case 10: return 0;
      case 12: return ch.wr[12];
      case 13: return ch.wr[13];
      case 15: return ch.wr[15];
      default: return 0;
    }
  }

  writeCtl(c, v) {
    const ch = this.ch[c];
    if (ch.ptr === 0) {
      ch.ptr = v & 7;
      const cmd = (v >> 3) & 7;
      if (cmd === 1) ch.ptr |= 8;                                // point high
      else if (cmd === 2) { ch.latched = false; ch.extIP = false; this.updateIRQ(); }     // reset ext/status interrupts
      ch.wr[0] = v;
      return;
    }
    const reg = ch.ptr;
    ch.ptr = 0;
    ch.wr[reg] = v;
    if (reg === 9) {
      this.ch[0].wr[9] = this.ch[1].wr[9] = v;                    // WR9 is shared
      if ((v & 0xC0) === 0xC0) { this.ch[0].reset(); this.ch[1].reset(); }
      else if (v & 0x80) this.softReset(0);
      else if (v & 0x40) this.softReset(1);
    } else if (reg === 2) this.ch[0].wr[2] = this.ch[1].wr[2] = v;
    this.updateIRQ();
  }
  softReset(c) { this.ch[c].reset(); }

  readData() { return 0; }
  writeData() { }

  // Bus access. reg bit 0: 0 = channel B, 1 = channel A; bit 1: 0 = control, 1 = data.
  read(reg) { const c = (reg & 1) ? 0 : 1; return (reg & 2) ? this.readData(c) : this.readCtl(c); }
  write(reg, v) { const c = (reg & 1) ? 0 : 1; if (reg & 2) this.writeData(c, v); else this.writeCtl(c, v); }
}
