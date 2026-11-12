// image.js - disk image container detection.
//
// Recognised layouts:
//   'apm'  Apple Partition Map hard disk image (block 0 'ER', 'PM' entries from block 1), e.g. BlueSCSI .hda
//   'dc42' Apple DiskCopy 4.2 floppy image (84-byte header followed by the raw volume)
//   'raw'  bare HFS / MFS volume (floppy .dsk/.img of any size, or a single partition dump)
//
// parseImage(bytes) -> { kind, volumes: [{ name, type, offset, length }], hasDriver, description }
// `volumes` lists the mountable file-system areas as byte ranges inside `bytes`.

function ascii(b, off, len) {
  let s = '';
  for (let i = 0; i < len && b[off + i]; i++) s += String.fromCharCode(b[off + i]);
  return s;
}
const be32 = (b, o) => ((b[o] << 24) | (b[o + 1] << 16) | (b[o + 2] << 8) | b[o + 3]) >>> 0;
const be16 = (b, o) => (b[o] << 8) | b[o + 1];

function volumeName(b, off) {
  // HFS MDB at volume offset 1024: 'BD', name = Pascal string at +36. MFS ($D2D7) keeps it at the same place.
  if (off + 1536 > b.length) return '';
  const sig = be16(b, off + 1024);
  if (sig === 0x4244 || sig === 0xD2D7) return ascii(b, off + 1024 + 37, Math.min(b[off + 1024 + 36], 27));
  return '';
}
export function volumeKind(b, off) {
  if (off + 1536 > b.length) return 'unknown';
  const sig = be16(b, off + 1024);
  return sig === 0x4244 ? 'HFS' : sig === 0xD2D7 ? 'MFS' : sig === 0x482B ? 'HFS+' : 'unknown';
}

export function parseImage(bytes) {
  const b = bytes;
  // Apple Partition Map
  if (b.length >= 1536 && be16(b, 0) === 0x4552 && be16(b, 512) === 0x504D) {
    const volumes = [];
    let hasDriver = false;
    const count = Math.min(be32(b, 512 + 4), 64);
    for (let i = 1; i <= count; i++) {
      const o = i * 512;
      if (o + 512 > b.length || be16(b, o) !== 0x504D) break;
      const start = be32(b, o + 8), blocks = be32(b, o + 12);
      const name = ascii(b, o + 16, 32), type = ascii(b, o + 48, 32);
      if (/^Apple_Driver/i.test(type)) hasDriver = true;
      if (type === 'Apple_HFS' || type === 'Apple_MFS') {
        const offset = start * 512, length = Math.min(blocks * 512, b.length - offset);
        if (length > 2048) volumes.push({ name: volumeName(b, offset) || name, type, offset, length });
      }
    }
    return { kind: 'apm', volumes, hasDriver,
      description: `Apple Partition Map, ${volumes.length} HFS volume(s)` + (hasDriver ? ', Apple driver present' : ', no driver partition') };
  }
  // DiskCopy 4.2
  if (b.length > 84 + 1024 && b[0] < 64 && be16(b, 0x52) === 0x0100) {
    const dataSize = be32(b, 0x40);
    if (dataSize > 0 && 84 + dataSize <= b.length && dataSize % 512 === 0) {
      return { kind: 'dc42', volumes: [{ name: volumeName(b, 84) || ascii(b, 1, b[0]), type: 'floppy', offset: 84, length: dataSize }],
        hasDriver: false, description: 'DiskCopy 4.2 image' };
    }
  }
  const length = b.length - (b.length % 512);
  return { kind: 'raw', volumes: length >= 1024 ? [{ name: volumeName(b, 0), type: volumeKind(b, 0), offset: 0, length }] : [], hasDriver: false,
    description: `raw ${volumeKind(b, 0)} volume, ${(length / 1024) | 0} KB` };
}
