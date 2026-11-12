// kbd_plus.js - Macintosh Plus keyboard (M0110A) on the VIA shift register.
//
// The Mac clocks a command byte out of the VIA shift register; the keyboard
// answers with one byte.
//   $10 Inquiry  - key transition, or $7B (null) after 0.25 s without one
//   $14 Instant  - key transition or $7B immediately
//   $16 Model    - $0B (M0110A, no further devices)
//   $36 Test     - $7D (ACK)
// Key transition byte: bit 7 = key up, bits 6-1 = key number, bit 0 = 1.
// Keypad and arrow keys are preceded by a $79 byte (fetched with Instant).
//
// keyDown/keyUp take ADB-style virtual key codes (see keycodes.js).

const NULL_RESPONSE = 0x7B;

export class PlusKeyboard {
  constructor(machine, via) {
    this.m = machine;
    this.via = via;
    this.queue = [];
    this.pendingCmd = -1;         // command waiting for an answer
    this.cmdTime = 0;
    this.busy = false;
  }
  reset() { this.queue.length = 0; this.pendingCmd = -1; this.busy = false; }

  // VIA host hook: CPU wrote the shift register.
  srWrite(v) {
    if (this.via.srMode !== 7) return;                   // only "shift out under external clock" is a command
    this.pendingCmd = -1;
    const cmd = v;
    this.m.schedule(8000, () => {                        // 8 bits clocked out (~1 ms)
      this.via.shiftDone();
      this.pendingCmd = cmd;
      this.cmdTime = this.m.now();
      this.m.schedule(8000, () => this.poll());
    });
  }

  // Try to answer the pending command.
  poll() {
    if (this.pendingCmd < 0) return;
    if (this.via.srOutput) { this.m.schedule(4000, () => this.poll()); return; }     // Mac not listening yet
    let resp = -1;
    switch (this.pendingCmd) {
      case 0x10:
        if (this.queue.length) resp = this.queue.shift();
        else if (this.m.now() - this.cmdTime > this.m.cyclesPerSecond / 4) resp = NULL_RESPONSE;
        break;
      case 0x14: resp = this.queue.length ? this.queue.shift() : NULL_RESPONSE; break;
      case 0x16: resp = 0x0B; break;
      case 0x36: resp = 0x7D; break;
      default: resp = NULL_RESPONSE;
    }
    if (resp < 0) { this.m.schedule(20000, () => this.poll()); return; }
    this.pendingCmd = -1;
    this.via.shiftIn(resp);
  }

  rawFor(code) {
    // -> array of bytes (without the up bit)
    switch (code) {
      case 0x3B: return [0x79, 0x0D];        // left
      case 0x3C: return [0x79, 0x05];        // right
      case 0x3D: return [0x79, 0x11];        // down
      case 0x3E: return [0x79, 0x1B];        // up
      case 0x36: return [(0x37 << 1) | 1];   // Control -> Command (the Plus keyboard has no Control key)
    }
    if (code >= 0x40 && code < 0x60) return [0x79, ((code - 0x40) << 1) | 1];
    if (code < 0x40) return [(code << 1) | 1];
    return null;
  }
  keyDown(code) { const r = this.rawFor(code); if (r) { this.queue.push(...r); this.kick(); } }
  keyUp(code) { const r = this.rawFor(code); if (r) { r[r.length - 1] |= 0x80; this.queue.push(...r); this.kick(); } }
  kick() { if (this.pendingCmd >= 0) this.m.schedule(100, () => this.poll()); }
}
