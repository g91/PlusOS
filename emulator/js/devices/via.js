// via.js - Synertek/Rockwell 6522 Versatile Interface Adapter.
//
// Timers count the E clock (CPU clock / 10 on the compact Macs). The chip is
// advanced lazily: every register access and every machine time slice calls
// sync(now) with the current CPU cycle count; nextEvent() tells the machine
// when the next timer underflow happens so the CPU slice can end exactly there.
//
// host callbacks (all optional):
//   readA() / readB()        -> input pin levels (bits configured as inputs)
//   writeA(v) / writeB(v)    -> called when output pins may have changed (v = pin levels)
//   srWrite(v)               -> CPU wrote the shift register (keyboard / ADB command byte)
//   srRead()                 -> CPU read the shift register
//   irq(state)               -> interrupt line changed

export const IFR_CA2 = 0x01, IFR_CA1 = 0x02, IFR_SR = 0x04, IFR_CB2 = 0x08, IFR_CB1 = 0x10, IFR_T2 = 0x20, IFR_T1 = 0x40;

export class VIA6522 {
  constructor(host, clockDiv = 10) {
    this.host = host;
    this.div = clockDiv;
    this.reset(0);
  }

  reset(now) {
    this.ora = 0; this.orb = 0; this.ddra = 0; this.ddrb = 0;
    this.t1c = 0xFFFF; this.t1l = 0xFFFF; this.t1armed = false;
    this.t2c = 0xFFFF; this.t2l = 0xFF; this.t2armed = false;
    this.sr = 0; this.acr = 0; this.pcr = 0; this.ifr = 0; this.ier = 0;
    this.last = now;           // CPU cycle of the last timer update (multiple of div)
    this.irqState = false;
    this.pb7 = 1;
  }

  // ---- time ------------------------------------------------------------------
  sync(now) {
    const ticks = Math.floor((now - this.last) / this.div);
    if (ticks <= 0) return;
    this.last += ticks * this.div;
    // timer 1
    let c = this.t1c - ticks;
    if (c < 0) {
      if (this.acr & 0x40) {                 // free running: reload from latch, period = latch + 2
        const period = this.t1l + 2;
        if (this.t1armed) this.setIFR(IFR_T1);
        c = ((c % period) + period) % period;
        if (c > this.t1l) c = this.t1l;
      } else {
        if (this.t1armed) { this.t1armed = false; this.setIFR(IFR_T1); }
        c &= 0xFFFF;
      }
    }
    this.t1c = c;
    // timer 2 (interval mode only; pulse counting on PB6 is not used by the Mac)
    if (!(this.acr & 0x20)) {
      let c2 = this.t2c - ticks;
      if (c2 < 0) {
        if (this.t2armed) { this.t2armed = false; this.setIFR(IFR_T2); }
        c2 &= 0xFFFF;
      }
      this.t2c = c2;
    }
  }

  // CPU cycle at which the next timer interrupt can occur (Infinity if none armed).
  nextEvent() {
    let n = Infinity;
    if (this.t1armed) n = this.last + (this.t1c + 1) * this.div;
    if (this.t2armed && !(this.acr & 0x20)) { const t = this.last + (this.t2c + 1) * this.div; if (t < n) n = t; }
    return n;
  }

  // ---- interrupts -------------------------------------------------------------
  setIFR(bits) { this.ifr |= bits; this.updateIRQ(); }
  clearIFR(bits) { this.ifr &= ~bits; this.updateIRQ(); }
  updateIRQ() {
    const s = (this.ifr & this.ier & 0x7F) !== 0;
    if (s !== this.irqState) { this.irqState = s; if (this.host.irq) this.host.irq(s); }
  }

  // Control line edges, called by the machine.
  pulseCA1() { this.setIFR(IFR_CA1); }
  pulseCA2() { this.setIFR(IFR_CA2); }
  pulseCB1() { this.setIFR(IFR_CB1); }
  pulseCB2() { this.setIFR(IFR_CB2); }

  // Shift register, driven by the external device (keyboard / ADB transceiver).
  shiftIn(byte) { this.sr = byte & 0xFF; this.setIFR(IFR_SR); }
  shiftDone() { this.setIFR(IFR_SR); }
  get srMode() { return (this.acr >> 2) & 7; }
  get srOutput() { return (this.acr & 0x10) !== 0; }

  // ---- pins ----------------------------------------------------------------------
  pinsA() { const inp = this.host.readA ? this.host.readA() : 0xFF; return (this.ora & this.ddra) | (inp & ~this.ddra & 0xFF); }
  pinsB() { const inp = this.host.readB ? this.host.readB() : 0xFF; return (this.orb & this.ddrb) | (inp & ~this.ddrb & 0xFF); }
  outA() { return (this.ora & this.ddra) | (~this.ddra & 0xFF); }      // undriven pins float high
  outB() { return (this.orb & this.ddrb) | (~this.ddrb & 0xFF); }

  // ---- register access -------------------------------------------------------------
  read(reg, now) {
    this.sync(now);
    switch (reg & 15) {
      case 0: this.clearIFR(IFR_CB1 | ((this.pcr & 0xA0) === 0x20 ? 0 : IFR_CB2)); return this.pinsB();
      case 1: this.clearIFR(IFR_CA1 | ((this.pcr & 0x0A) === 0x02 ? 0 : IFR_CA2)); return this.pinsA();
      case 2: return this.ddrb;
      case 3: return this.ddra;
      case 4: this.clearIFR(IFR_T1); return this.t1c & 0xFF;
      case 5: return (this.t1c >> 8) & 0xFF;
      case 6: return this.t1l & 0xFF;
      case 7: return (this.t1l >> 8) & 0xFF;
      case 8: this.clearIFR(IFR_T2); return this.t2c & 0xFF;
      case 9: return (this.t2c >> 8) & 0xFF;
      case 10: { this.clearIFR(IFR_SR); const v = this.sr; if (this.host.srRead) this.host.srRead(); return v; }
      case 11: return this.acr;
      case 12: return this.pcr;
      case 13: return (this.ifr & 0x7F) | ((this.ifr & this.ier & 0x7F) ? 0x80 : 0);
      case 14: return this.ier | 0x80;
      default: return this.pinsA();
    }
  }

  write(reg, v, now) {
    this.sync(now);
    v &= 0xFF;
    switch (reg & 15) {
      case 0: this.orb = v; this.clearIFR(IFR_CB1 | ((this.pcr & 0xA0) === 0x20 ? 0 : IFR_CB2)); if (this.host.writeB) this.host.writeB(this.outB()); break;
      case 1: this.ora = v; this.clearIFR(IFR_CA1 | ((this.pcr & 0x0A) === 0x02 ? 0 : IFR_CA2)); if (this.host.writeA) this.host.writeA(this.outA()); break;
      case 2: this.ddrb = v; if (this.host.writeB) this.host.writeB(this.outB()); break;
      case 3: this.ddra = v; if (this.host.writeA) this.host.writeA(this.outA()); break;
      case 4: case 6: this.t1l = (this.t1l & 0xFF00) | v; break;
      case 5: this.t1l = (this.t1l & 0x00FF) | (v << 8); this.t1c = this.t1l; this.t1armed = true; this.clearIFR(IFR_T1); this.changed(); break;
      case 7: this.t1l = (this.t1l & 0x00FF) | (v << 8); this.clearIFR(IFR_T1); break;
      case 8: this.t2l = v; break;
      case 9: this.t2c = (v << 8) | this.t2l; this.t2armed = true; this.clearIFR(IFR_T2); this.changed(); break;
      case 10: this.sr = v; this.clearIFR(IFR_SR); if (this.host.srWrite) this.host.srWrite(v); break;
      case 11: {
        const old = this.acr;
        this.acr = v;
        if ((v & 0x40) && !(old & 0x40)) this.t1armed = true;      // free-run keeps interrupting
        if (this.host.acrWrite) this.host.acrWrite(v, old);
        this.changed();
        break;
      }
      case 12: this.pcr = v; break;
      case 13: this.clearIFR(v & 0x7F); break;
      case 14: if (v & 0x80) this.ier |= v & 0x7F; else this.ier &= ~v & 0x7F; this.updateIRQ(); break;
      default: this.ora = v; if (this.host.writeA) this.host.writeA(this.outA()); break;
    }
  }

  changed() { if (this.host.timersChanged) this.host.timersChanged(); }
}
