// iwm.js - Integrated Woz Machine floppy controller, with no drive attached.
//
// Only the chip itself is emulated (state latches, mode/status/handshake
// registers) so that the ROM's controller initialisation and the .Sony driver's
// drive probing work. Every drive sense line reads 1, which the .Sony driver
// interprets as "no drive installed". Disk images are attached through the
// paravirtual driver or the SCSI bus instead (see pvdisk.js / scsi.js).

export class IWM {
  constructor() { this.reset(); }
  reset() { this.lines = 0; this.mode = 0; }

  // reg = 0..15 (CPU address bits 9-12). Every access sets or clears one state line.
  access(reg, isWrite, v) {
    const bit = 1 << (reg >> 1);
    if (reg & 1) this.lines |= bit; else this.lines &= ~bit;
    const q6 = this.lines & 0x40, q7 = this.lines & 0x80, motor = this.lines & 0x10;
    if (isWrite) {
      if (q7 && q6 && !motor) this.mode = v & 0x1F;
      return 0;
    }
    if (!q7 && !q6) return motor ? 0x00 : 0xFF;                          // data register (no flux: nothing ever arrives)
    if (!q7 && q6) return (this.mode & 0x1F) | (motor ? 0x20 : 0) | 0x80;   // status: sense = 1
    if (q7 && !q6) return 0xFF;                                          // write handshake: ready, no underrun
    return 0;
  }
}
