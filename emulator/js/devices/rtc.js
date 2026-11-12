// rtc.js - Macintosh real-time clock chip with 256 bytes of parameter RAM (the
// "new" 343-0042 part; it also answers the 20-byte commands of the original chip).
//
// Serial protocol on three VIA port B lines: /enable (PB2), clock (PB1), data (PB0).
// With /enable low, bits are shifted MSB first on rising clock edges. A command
// byte is followed by one data byte (written by the Mac or driven by the chip).
//   z00aaa01  seconds register a (4 bytes, little endian), z = 1 read
//   z0110001  test register          z0110101  write-protect register
//   z010aa01  PRAM $08-$0B           z1aaaa01  PRAM $10-$1F
//   z0111aaa  0aaaaa00               extended command: PRAM address aaa:aaaaa (0-255)

const MAC_EPOCH_OFFSET = 2082844800;       // seconds between 1904-01-01 and 1970-01-01

export class RTC {
  constructor() {
    this.pram = new Uint8Array(256);
    this.seconds = 0;
    this.writeProtect = false;
    this.onPramWrite = null;
    this.setFromHost();
    this.resetSerial();
  }

  setFromHost(date = new Date()) {
    // the Mac clock runs in local time
    this.seconds = (Math.floor(date.getTime() / 1000) - date.getTimezoneOffset() * 60 + MAC_EPOCH_OFFSET) >>> 0;
  }
  tick() { this.seconds = (this.seconds + 1) >>> 0; }

  resetSerial() {
    this.enabled = false; this.clk = 0; this.dataOut = 1;
    this.bitCount = 0; this.shift = 0; this.state = 0; this.cmd = 0; this.xaddr = 0; this.outByte = 0; this.reading = false;
  }

  // Called when port B outputs change. `pins` = PB levels, `ddrb` tells whether the Mac drives the data line.
  update(pins, ddrb) {
    const enable = !(pins & 4), clk = (pins >> 1) & 1, data = pins & 1;
    if (!enable) { if (this.enabled) this.resetSerial(); this.clk = clk; return; }
    if (!this.enabled) { this.enabled = true; this.bitCount = 0; this.state = 0; this.reading = false; this.clk = clk; return; }
    if (clk === this.clk) return;
    this.clk = clk;
    if (clk) {
      // rising edge: sample the bit the Mac wrote (when it is sending)
      if (!this.reading) {
        this.shift = ((this.shift << 1) | data) & 0xFF;
        if (++this.bitCount === 8) { this.bitCount = 0; this.byteIn(this.shift); }
      }
    } else if (this.reading) {
      // falling edge: present the next bit to the Mac
      this.dataOut = (this.outByte >> 7) & 1;
      this.outByte = (this.outByte << 1) & 0xFF;
      if (++this.bitCount === 8) { this.bitCount = 0; this.reading = false; this.state = 0; }
    }
  }
  // level of the data line when the chip drives it
  dataBit() { return this.dataOut; }

  byteIn(b) {
    switch (this.state) {
      case 0:                                   // command byte
        this.cmd = b;
        if ((b & 0x78) === 0x38) { this.state = 2; return; }       // extended command, second address byte follows
        if (b & 0x80) { this.outByte = this.readReg(b); this.reading = true; this.bitCount = 0; }
        else this.state = 1;
        return;
      case 1: this.writeReg(this.cmd, b); this.state = 0; return;
      case 2:
        this.xaddr = ((this.cmd & 7) << 5) | ((b >> 2) & 0x1F);
        if (this.cmd & 0x80) { this.outByte = this.pram[this.xaddr]; this.reading = true; this.bitCount = 0; this.state = 0; }
        else this.state = 3;
        return;
      default:
        if (!this.writeProtect) { this.pram[this.xaddr] = b; if (this.onPramWrite) this.onPramWrite(); }
        this.state = 0;
    }
  }

  regAddr(cmd) { return (cmd >> 2) & 0x1F; }
  readReg(cmd) {
    const a = this.regAddr(cmd);
    if (a < 8) return (this.seconds >>> (8 * (a & 3))) & 0xFF;
    if (a < 12) return this.pram[a];                 // $08-$0B
    if (a >= 16) return this.pram[a];                // $10-$1F
    return 0;
  }
  writeReg(cmd, v) {
    const a = this.regAddr(cmd);
    if (a === 13) { this.writeProtect = (v & 0x80) !== 0; return; }
    if (a === 12 || this.writeProtect) return;       // test register
    if (a < 8) { const sh = 8 * (a & 3); this.seconds = ((this.seconds & ~(0xFF << sh)) | (v << sh)) >>> 0; return; }
    if (a < 12 || a >= 16) { this.pram[a] = v; if (this.onPramWrite) this.onPramWrite(); }
  }
}
