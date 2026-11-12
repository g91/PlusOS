// scsi.js - NCR 5380 SCSI controller with the Macintosh pseudo-DMA access
// pattern, and a SCSI direct-access (hard disk) target.
//
// The bus is modelled at the handshake level: the selected target drives
// REQ and the phase lines, the initiator (the Mac ROM's SCSI Manager) answers
// each REQ with ACK either through the Initiator Command Register or, in DMA
// mode, by reading/writing the data register with DACK (address bit 9).

// phase bits as seen in the Target Command Register / bus status: MSG<<2 | C/D<<1 | I/O
const PH_DATA_OUT = 0, PH_DATA_IN = 1, PH_COMMAND = 2, PH_STATUS = 3, PH_MSG_OUT = 6, PH_MSG_IN = 7;

export class SCSIDisk {
  // image: Uint8Array holding the whole disk (512-byte blocks). onWrite() is called after writes.
  constructor(image, opts = {}) {
    this.image = image;
    this.blocks = Math.floor(image.length / 512);
    this.readOnly = !!opts.readOnly;
    this.onWrite = opts.onWrite || null;
    this.vendor = (opts.vendor || 'PLUSOS').padEnd(8).slice(0, 8);
    this.product = (opts.product || 'VIRTUAL DISK').padEnd(16).slice(0, 16);
    this.sense = [0, 0, 0];           // key, asc, ascq
    this.log = [];                    // opcodes of the commands received (most recent 64)
    this.commands = 0;
  }

  cdbLength(op) { const g = op >> 5; return g === 0 ? 6 : g === 5 ? 12 : 10; }

  // Execute a command. Returns { status, dataIn?: Uint8Array, dataOutLen?: number, done?(data) }.
  command(cdb) {
    const op = cdb[0];
    this.commands++; this.log.push(op); if (this.log.length > 64) this.log.shift();
    const good = () => { if (op !== 0x03) this.sense = [0, 0, 0]; return { status: 0 }; };
    const fail = (key, asc) => { this.sense = [key, asc, 0]; return { status: 2 }; };
    switch (op) {
      case 0x00: case 0x01: case 0x04: case 0x0B: case 0x1B: case 0x1E: case 0x2B: case 0x2F: case 0x35: case 0x07:
        return good();
      case 0x03: {
        const n = cdb[4] || 4, d = new Uint8Array(18);
        d[0] = 0x70; d[2] = this.sense[0]; d[7] = 10; d[12] = this.sense[1]; d[13] = this.sense[2];
        this.sense = [0, 0, 0];
        return { status: 0, dataIn: d.subarray(0, Math.min(n, 18)) };
      }
      case 0x12: {
        const d = new Uint8Array(36);
        d[0] = 0; d[1] = 0; d[2] = 1; d[3] = 1; d[4] = 31;
        const s = this.vendor + this.product + '1.0 ';
        for (let i = 0; i < 28; i++) d[8 + i] = s.charCodeAt(i);
        return { status: 0, dataIn: d.subarray(0, Math.min(cdb[4] || 36, 36)) };
      }
      case 0x15: return { status: 0, dataOutLen: cdb[4], done: () => 0 };
      case 0x1A: {
        const page = cdb[2] & 0x3F, d = new Uint8Array(64);
        let n = 4;
        d[2] = this.readOnly ? 0x80 : 0;
        if (!(cdb[1] & 8)) {
          d[3] = 8;
          d[5] = (this.blocks >> 16) & 0xFF; d[6] = (this.blocks >> 8) & 0xFF; d[7] = this.blocks & 0xFF;
          d[10] = 2;                                           // block length 512
          n = 12;
        }
        if (page === 3 || page === 0x3F) { d[n] = 3; d[n + 1] = 22; d[n + 11] = 32; d[n + 12] = 2; n += 24; }
        if (page === 4 || page === 0x3F) {
          const cyl = Math.max(1, Math.ceil(this.blocks / (32 * 16)));
          d[n] = 4; d[n + 1] = 22; d[n + 2] = cyl >> 16; d[n + 3] = cyl >> 8; d[n + 4] = cyl; d[n + 5] = 16; n += 24;
        }
        if (page === 0x30) { d[n] = 0x30; d[n + 1] = 22; const s = 'APPLE COMPUTER, INC   '; for (let i = 0; i < 22; i++) d[n + 2 + i] = s.charCodeAt(i); n += 24; }
        d[0] = n - 1;
        return { status: 0, dataIn: d.subarray(0, Math.min(cdb[4], n)) };
      }
      case 0x25: {
        const d = new Uint8Array(8), last = this.blocks - 1;
        d[0] = last >>> 24; d[1] = last >>> 16; d[2] = last >>> 8; d[3] = last; d[6] = 2;
        return { status: 0, dataIn: d };
      }
      case 0x08: case 0x28: case 0x0A: case 0x2A: {
        let lba, len;
        if (op < 0x20) { lba = ((cdb[1] & 0x1F) << 16) | (cdb[2] << 8) | cdb[3]; len = cdb[4] || 256; }
        else { lba = ((cdb[2] << 24) | (cdb[3] << 16) | (cdb[4] << 8) | cdb[5]) >>> 0; len = (cdb[7] << 8) | cdb[8]; }
        if (lba + len > this.blocks) return fail(5, 0x21);          // LBA out of range
        if (len === 0) return good();
        if (op === 0x08 || op === 0x28) { this.sense = [0, 0, 0]; return { status: 0, dataIn: this.image.subarray(lba * 512, (lba + len) * 512) }; }
        if (this.readOnly) return fail(7, 0x27);
        return { status: 0, dataOutLen: len * 512, done: (data) => { this.image.set(data, lba * 512); if (this.onWrite) this.onWrite(); this.sense = [0, 0, 0]; return 0; } };
      }
      default: return fail(5, 0x20);                                 // invalid command operation code
    }
  }
}

export class NCR5380 {
  constructor(host) {
    this.host = host || {};            // { irq(state) }
    this.targets = new Array(8).fill(null);
    this.reset();
  }
  attach(id, target) { this.targets[id] = target; }

  reset() {
    this.odr = 0; this.icr = 0; this.mode = 0; this.tcr = 0; this.ser = 0;
    this.aip = false; this.irq = false; this.dma = false; this.endDMA = false; this.busyErr = false;
    this.busFree();
  }
  busFree() {
    this.target = null; this.tBsy = false; this.req = false; this.phase = 0; this.tData = 0;
    this.selected = false; this.buf = null; this.pos = 0; this.cmdLen = 0; this.pending = null; this.stage = 'free';
  }
  setIRQ(s) { if (this.irq !== s) { this.irq = s; if (this.host.irq) this.host.irq(s); } }

  // ---- target side ------------------------------------------------------------
  enter(phase, stage) { this.phase = phase; this.stage = stage; this.pos = 0; this.request(); }
  request() {
    // present the next byte (IN phases) and raise REQ
    if (this.phase & 1) this.tData = this.buf[this.pos];
    this.req = true;
    if (this.dma && (this.tcr & 7) !== this.phase) this.setIRQ(true);      // phase mismatch during DMA
  }
  // one REQ/ACK handshake completed; `byte` is what the initiator drove (OUT phases)
  handshake(byte) {
    this.req = false;
    if (!(this.phase & 1)) this.buf[this.pos] = byte;
    this.pos++;
    switch (this.stage) {
      case 'msgout': this.startCommand(); return;
      case 'command':
        if (this.pos === 1) this.cmdLen = this.target.cdbLength(this.buf[0]);
        if (this.pos < this.cmdLen) { this.request(); return; }
        this.execute();
        return;
      case 'datain':
        if (this.pos < this.buf.length) { this.request(); return; }
        this.finish(this.pending.status);
        return;
      case 'dataout':
        if (this.pos < this.buf.length) { this.request(); return; }
        this.finish(this.pending.done(this.buf) || this.pending.status);
        return;
      case 'status': this.buf = new Uint8Array([0]); this.enter(PH_MSG_IN, 'msgin'); return;
      default: this.busFree(); return;                                    // message in done -> bus free
    }
  }
  startCommand() { this.buf = new Uint8Array(12); this.enter(PH_COMMAND, 'command'); }
  execute() {
    const r = this.target.command(this.buf.subarray(0, this.cmdLen));
    this.pending = r;
    if (r.dataIn && r.dataIn.length) { this.buf = r.dataIn; this.enter(PH_DATA_IN, 'datain'); }
    else if (r.dataOutLen) { this.buf = new Uint8Array(r.dataOutLen); this.enter(PH_DATA_OUT, 'dataout'); }
    else this.finish(r.status);
  }
  finish(status) { this.buf = new Uint8Array([status]); this.enter(PH_STATUS, 'status'); }

  // ---- registers -----------------------------------------------------------------
  busData() {
    if (this.target && this.req && (this.phase & 1)) return this.tData;
    if (this.icr & 0x01) return this.odr;
    if (this.aip) return this.odr;
    return this.target && (this.phase & 1) ? this.tData : 0;
  }
  phaseMatch() { return !this.target || this.stage === 'selected' || (this.tcr & 7) === this.phase; }
  // DRQ follows REQ while DMA mode is active, also after the target has changed phase (the Plus ROM
  // waits for it after the last byte of a blind write); the transfer itself needs a matching phase.
  drq() { return this.dma && this.req; }

  read(reg, dack) {
    if (dack) return this.dmaRead();
    switch (reg & 7) {
      case 0: return this.busData();
      case 1: return (this.icr & 0x9F) | (this.aip ? 0x40 : 0);
      case 2: return this.mode;
      case 3: return this.tcr;
      case 4: {
        let v = 0;
        if (this.icr & 0x80) v |= 0x80;
        if (this.tBsy || (this.icr & 0x08)) v |= 0x40;
        if (this.req) v |= 0x20;
        if (this.target && this.stage !== 'selected') v |= (this.phase & 4 ? 0x10 : 0) | (this.phase & 2 ? 0x08 : 0) | (this.phase & 1 ? 0x04 : 0);
        if (this.icr & 0x04) v |= 0x02;
        return v;
      }
      case 5: {
        let v = 0;
        if (this.endDMA) v |= 0x80;
        if (this.drq()) v |= 0x40;
        if (this.irq) v |= 0x10;
        if (this.phaseMatch()) v |= 0x08;
        if (this.busyErr) v |= 0x04;
        if (this.icr & 0x02) v |= 0x02;
        if (this.icr & 0x10) v |= 0x01;
        return v;
      }
      case 6: return this.busData();
      default: this.setIRQ(false); this.busyErr = false; return 0;
    }
  }

  write(reg, v, dack) {
    if (dack) { this.dmaWrite(v); return; }
    switch (reg & 7) {
      case 0: this.odr = v; break;
      case 1: this.writeICR(v); break;
      case 2: {
        const old = this.mode;
        this.mode = v;
        if ((v & 1) && !(old & 1)) this.aip = !this.target && !this.tBsy;      // arbitration: always won on a free bus
        if (!(v & 1)) this.aip = false;
        if (!(v & 2)) { this.dma = false; this.endDMA = false; }
        break;
      }
      case 3: this.tcr = v & 0x0F; break;
      case 4: this.ser = v; break;
      default: if (this.mode & 2) { this.dma = true; if (this.req && (this.tcr & 7) !== this.phase) this.setIRQ(true); } break;     // start DMA
    }
  }

  writeICR(v) {
    const old = this.icr;
    this.icr = v;
    if (v & 0x80) {                                           // RST asserted: every device releases the bus
      this.busFree(); this.aip = false; this.dma = false;
      this.setIRQ(true);
      return;
    }
    // selection: SEL asserted with the target's ID on the data bus and BSY released
    if ((v & 0x04) && !(v & 0x08) && (v & 0x01) && !this.target) {
      for (let id = 6; id >= 0; id--) {
        if ((this.odr & (1 << id)) && this.targets[id]) {
          this.target = this.targets[id]; this.tBsy = true; this.stage = 'selected'; this.atnAtSel = (v & 0x02) !== 0;
          break;
        }
      }
    }
    // selection phase ends when the initiator drops SEL
    if (this.stage === 'selected' && !(v & 0x04) && (old & 0x04)) {
      if (v & 0x02) { this.buf = new Uint8Array(1); this.enter(PH_MSG_OUT, 'msgout'); }
      else this.startCommand();
    }
    // manual handshake: ACK rising edge takes the byte, falling edge lets the target continue
    if (this.target && this.stage !== 'selected') {
      if ((v & 0x10) && !(old & 0x10) && this.req) { this.ackByte = (v & 0x01) ? this.odr : 0; this.req = false; this.ackHeld = true; }
      else if (!(v & 0x10) && (old & 0x10) && this.ackHeld) { this.ackHeld = false; this.req = true; this.handshake(this.ackByte); }
    }
  }

  // pseudo-DMA: one access = one complete handshake
  dmaRead() {
    if (!this.target || !this.req || !(this.phase & 1) || (this.tcr & 7) !== this.phase) return 0;
    const v = this.tData;
    this.handshake(0);
    return v;
  }
  dmaWrite(v) {
    if (!this.target || !this.req || (this.phase & 1) || (this.tcr & 7) !== this.phase) return;
    this.handshake(v);
  }
}
