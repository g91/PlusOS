// table.js - machine definitions.
//
// status: 'working'  boots in this emulator (see README for what was verified)
//         'untested' implemented from the definition, no ROM available to verify
//         'planned'  listed for reference only; the hardware is not emulated yet
//
// `roms` lists ROM images that exist in the PlusOS repository (paths relative to the repo root).
// A user-supplied ROM is matched to a machine by size (and checksum when known).

export const MACHINES = {
  plus: {
    id: 'plus', name: 'Macintosh Plus', status: 'working', family: 'compact',
    cpu: '68000', clock: 7833600, ramOptions: [1, 2, 4], ramDefault: 4,
    romSize: 0x20000, romBase: 0x400000,
    roms: [{ path: 'tools/emulator/vMac.ROM', label: 'Mac Plus ROM v3 (4D1F8172)' }],
    checksums: { '4D1EEEE1': 'Plus v1 (Lonely Hearts)', '4D1EEAE1': 'Plus v2 (Lonely Heifers)', '4D1F8172': 'Plus v3 (Loud Harmonicas)' },
    screen: { width: 512, height: 342 },
    input: 'plus-keyboard+quadrature-mouse', overlay: 'via-pa4',
    devices: ['VIA 6522 @ $EFE1FE', 'SCC 8530 @ $9FFFF8/$BFFFF9', 'IWM @ $DFE1FF', 'NCR 5380 @ $580000', 'RTC/PRAM', 'paravirtual disk @ $F40000'],
    memoryMap: [
      ['$000000-$3FFFFF', 'RAM (mirrored below 4 MB); ROM while the overlay is on'],
      ['$400000-$57FFFF', 'ROM (128 KB, mirrored)'],
      ['$580000-$5FFFFF', 'NCR 5380 SCSI (A4-A6 = register, A9 = DACK)'],
      ['$600000-$7FFFFF', 'RAM while the overlay is on'],
      ['$800000-$9FFFFF', 'SCC read'], ['$A00000-$BFFFFF', 'SCC write'],
      ['$C00000-$DFFFFF', 'IWM'], ['$E80000-$EFFFFF', 'VIA'],
      ['$F00000-$F7FFFF', 'phase read; emulator port at $F40000'],
    ],
  },
  se: {
    id: 'se', name: 'Macintosh SE', status: 'working', family: 'compact',
    cpu: '68000', clock: 7833600, ramOptions: [1, 2, 4], ramDefault: 4,
    romSize: 0x40000, romBase: 0x400000,
    roms: [{ path: 'ROM/SE/MacSE.ROM', label: 'Mac SE ROM (repo build, B2E36294)' }, { path: 'ROM/SE/MacSE-stock.ROM', label: 'Mac SE ROM (stock, B2E362A8)' }],
    checksums: { 'B2E362A8': 'SE', 'B2E36294': 'SE (PlusOS repo build)', 'B306E171': 'SE FDHD' },
    screen: { width: 512, height: 342 },
    input: 'adb', overlay: 'auto',
    devices: ['VIA 6522', 'SCC 8530', 'IWM', 'NCR 5380 (IRQ on VIA CB2)', 'RTC/PRAM (256 bytes)', 'ADB transceiver, keyboard, mouse', 'paravirtual disk @ $F40000'],
    memoryMap: [
      ['$000000-$3FFFFF', 'RAM; ROM until the first access to $400000 after reset'],
      ['$400000-$57FFFF', 'ROM (256 KB, mirrored)'],
      ['$580000-$5FFFFF', 'NCR 5380 SCSI'],
      ['$800000-$BFFFFF', 'SCC'], ['$C00000-$DFFFFF', 'IWM'], ['$E80000-$EFFFFF', 'VIA'],
      ['$F00000-$F7FFFF', 'emulator port at $F40000'],
    ],
  },
  classic: {
    id: 'classic', name: 'Macintosh Classic', status: 'untested', family: 'compact',
    cpu: '68000', clock: 7833600, ramOptions: [1, 2, 4], ramDefault: 4,
    romSize: 0x80000, romBase: 0x400000,
    roms: [],
    checksums: { 'A49F9914': 'Classic' },
    screen: { width: 512, height: 342 },
    input: 'adb', overlay: 'auto',
    note: 'SE-compatible logic board (512 KB ROM, SWIM). No ROM in the repository: untested, supply your own ROM.',
    devices: ['as Macintosh SE; SWIM floppy controller presented as IWM'],
    memoryMap: [['$000000-$3FFFFF', 'RAM'], ['$400000-$57FFFF', 'ROM (512 KB)'], ['other', 'as Macintosh SE']],
  },
  // ---- not emulated yet: listed with what is known about them ---------------------------------
  mac512ke: { id: 'mac512ke', name: 'Macintosh 512Ke', status: 'planned', family: 'compact', cpu: '68000', clock: 7833600, ramOptions: [0.5], romSize: 0x20000,
    note: 'Plus ROM, no SCSI, 512 KB RAM.' },
  classic2: { id: 'classic2', name: 'Macintosh Classic II', status: 'planned', family: 'mac2', cpu: '68030', clock: 15667200, ramOptions: [2, 4, 10], romSize: 0x80000,
    note: '68030 @ 16 MHz, 512x342 1-bit video, Egret ADB, 24/32-bit addressing.' },
  se30: { id: 'se30', name: 'Macintosh SE/30', status: 'planned', family: 'mac2', cpu: '68030', clock: 15667200, ramOptions: [1, 4, 8, 16, 32], romSize: 0x40000,
    note: 'IIx architecture in a compact case: 2 VIAs, ASC sound, 512x342 video at $FEE00000 (pseudo-slot $E).' },
  mac2: { id: 'mac2', name: 'Macintosh II', status: 'planned', family: 'mac2', cpu: '68020', clock: 15667200, ramOptions: [1, 2, 4, 8], romSize: 0x40000,
    note: '68020 + 68881 (+ optional 68851), NuBus video card, 2 VIAs, ASC. ROM at $40800000, I/O at $50F00000.' },
  mac2x: { id: 'mac2x', name: 'Macintosh IIx', status: 'planned', family: 'mac2', cpu: '68030', clock: 15667200, ramOptions: [1, 4, 8], romSize: 0x40000,
    note: '68030 + 68882, SWIM, otherwise Macintosh II.' },
  mac2cx: { id: 'mac2cx', name: 'Macintosh IIcx', status: 'planned', family: 'mac2', cpu: '68030', clock: 15667200, ramOptions: [1, 4, 8], romSize: 0x40000,
    note: 'Three-slot IIx.' },
  lc: { id: 'lc', name: 'Macintosh LC', status: 'planned', family: 'v8', cpu: '68EC020', clock: 15667200, ramOptions: [2, 4, 10], romSize: 0x80000,
    note: '68020 @ 16 MHz, V8 gate array (built-in video), Egret. ROM at $00A00000 in 24-bit mode.' },
  lc2: { id: 'lc2', name: 'Macintosh LC II', status: 'planned', family: 'v8', cpu: '68030', clock: 15667200, ramOptions: [4, 10], romSize: 0x80000,
    note: 'LC with a 68030.' },
  lc3: { id: 'lc3', name: 'Macintosh LC III', status: 'planned', family: 'sonora', cpu: '68030', clock: 25000000, ramOptions: [4, 8, 36], romSize: 0x100000,
    note: '68030 @ 25 MHz, Sonora video/memory controller, Egret/Cuda, 1 MB ROM at $40800000.' },
  quadra700: { id: 'quadra700', name: 'Macintosh Quadra 700', status: 'planned', family: 'quadra', cpu: '68040', clock: 25000000, ramOptions: [4, 8, 20, 68], romSize: 0x100000,
    note: '68040 @ 25 MHz, DAFB video, 53C96 SCSI, SONIC Ethernet, 2 VIAs.' },
};

export const MACHINE_ORDER = ['plus', 'se', 'classic', 'mac512ke', 'classic2', 'se30', 'mac2', 'mac2x', 'mac2cx', 'lc', 'lc2', 'lc3', 'quadra700'];

export function romChecksum(rom) {
  return (((rom[0] << 24) | (rom[1] << 16) | (rom[2] << 8) | rom[3]) >>> 0).toString(16).toUpperCase().padStart(8, '0');
}

// Guess the machine a ROM image belongs to (by checksum, then by size).
export function identifyRom(rom) {
  const sum = romChecksum(rom);
  for (const id of MACHINE_ORDER) { const m = MACHINES[id]; if (m.checksums && m.checksums[sum]) return { id, label: m.checksums[sum], checksum: sum }; }
  if (rom.length === 0x20000) return { id: 'plus', label: 'unknown 128 KB ROM', checksum: sum };
  if (rom.length === 0x40000 && rom[0] === 0xB2) return { id: 'se', label: 'unknown 256 KB ROM', checksum: sum };
  if (rom.length === 0x80000 && rom[0] === 0xA4) return { id: 'classic', label: 'unknown 512 KB ROM', checksum: sum };
  return { id: null, label: 'unrecognised ROM', checksum: sum };
}
