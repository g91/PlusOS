"""Decode the host containers that carry Mac forks and Finder info:
MacBinary I/II/III (.bin), AppleSingle and AppleDouble (._name).
"""
import struct

APPLESINGLE_MAGIC = 0x00051600
APPLEDOUBLE_MAGIC = 0x00051607
_AD_EPOCH_DELTA = 946684800 + 2082844800      # AppleDouble dates count from 2000-01-01

# Finder flags that must not survive a transfer (MacBinary II spec):
# isOnDesk, bFOwnAppl, hasBeenInited, changed, busy.
TRANSFER_CLEAR = 0x0001 | 0x0002 | 0x0100 | 0x0200 | 0x0400


def crc16_xmodem(data):
    crc = 0
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


class Decoded:
    """Result of decoding a container."""

    def __init__(self):
        self.name = None            # MacRoman bytes, if the container names the file
        self.data = None            # data fork bytes (None: container has no data fork)
        self.rsrc = b""
        self.ftype = self.creator = None
        self.flags = None
        self.location = None
        self.fxinfo = None
        self.created = self.modified = None     # Mac epoch seconds
        self.locked = False


def _pad128(n):
    return (n + 127) & ~127


def decode_macbinary(raw):
    """-> Decoded, or None if `raw` is not a MacBinary file."""
    if len(raw) < 128 or raw[0] != 0 or raw[74] != 0 or raw[82] != 0:
        return None
    nlen = raw[1]
    if not 1 <= nlen <= 63:
        return None
    dlen, rlen = struct.unpack_from(">II", raw, 83)
    if dlen > 0x7FFFFFFF or rlen > 0x7FFFFFFF:
        return None
    sec_hdr = struct.unpack_from(">H", raw, 120)[0]
    crc_ok = crc16_xmodem(raw[:124]) == struct.unpack_from(">H", raw, 124)[0]
    is_v1 = raw[101:128] == b"\0" * 27
    if not (crc_ok or is_v1 or raw[102:106] == b"mBIN"):
        return None
    doff = 128 + (_pad128(sec_hdr) if crc_ok else 0)
    roff = doff + _pad128(dlen)
    if roff + rlen > len(raw) or len(raw) > roff + _pad128(rlen) + 65536 + 128:
        return None
    if any(b < 32 for b in raw[65:73]) and not crc_ok:
        return None
    d = Decoded()
    d.name = bytes(raw[2:2 + nlen])
    d.ftype, d.creator = bytes(raw[65:69]), bytes(raw[69:73])
    d.flags = ((raw[73] << 8) | (raw[101] if not is_v1 else 0)) & ~TRANSFER_CLEAR
    d.location = (0, 0)
    d.locked = bool(raw[81] & 1)
    d.created, d.modified = struct.unpack_from(">II", raw, 91)
    d.data = bytes(raw[doff:doff + dlen])
    d.rsrc = bytes(raw[roff:roff + rlen])
    return d


def decode_applefile(raw):
    """AppleSingle / AppleDouble -> Decoded, or None if `raw` is neither."""
    if len(raw) < 26:
        return None
    magic, version = struct.unpack_from(">II", raw)
    if magic not in (APPLESINGLE_MAGIC, APPLEDOUBLE_MAGIC) or version not in (0x00010000, 0x00020000):
        return None
    count = struct.unpack_from(">H", raw, 24)[0]
    if 26 + 12 * count > len(raw):
        return None
    d = Decoded()
    for i in range(count):
        eid, off, length = struct.unpack_from(">III", raw, 26 + 12 * i)
        body = raw[off:off + length]
        if eid == 1:
            d.data = bytes(body)
        elif eid == 2:
            d.rsrc = bytes(body)
        elif eid == 3:
            d.name = bytes(body)
        elif eid == 8 and len(body) >= 8:
            cr, md = struct.unpack_from(">ii", body)
            if cr != -0x80000000:
                d.created = (cr + _AD_EPOCH_DELTA) & 0xFFFFFFFF
            if md != -0x80000000:
                d.modified = (md + _AD_EPOCH_DELTA) & 0xFFFFFFFF
        elif eid == 9 and len(body) >= 16:
            d.ftype, d.creator = bytes(body[0:4]), bytes(body[4:8])
            d.flags = struct.unpack_from(">H", body, 8)[0] & ~TRANSFER_CLEAR
            d.location = struct.unpack_from(">hh", body, 10)
            if len(body) >= 32:
                d.fxinfo = bytes(body[16:32])
            if d.ftype == b"\0\0\0\0":
                d.ftype = None
            if d.creator == b"\0\0\0\0":
                d.creator = None
    return d


def encode_macbinary(name, data, rsrc, ftype, creator, flags=0, created=0, modified=0):
    """Build a MacBinary II file (used by the tests and `hdbuild.py` helpers)."""
    hdr = bytearray(128)
    hdr[1] = len(name)
    hdr[2:2 + len(name)] = name
    hdr[65:69], hdr[69:73] = ftype, creator
    hdr[73] = (flags >> 8) & 0xFF
    struct.pack_into(">II", hdr, 83, len(data), len(rsrc))
    struct.pack_into(">II", hdr, 91, created, modified)
    hdr[101] = flags & 0xFF
    hdr[122], hdr[123] = 129, 129
    struct.pack_into(">H", hdr, 124, crc16_xmodem(bytes(hdr[:124])))
    return (bytes(hdr) + data + b"\0" * (_pad128(len(data)) - len(data))
            + rsrc + b"\0" * (_pad128(len(rsrc)) - len(rsrc)))


def encode_appledouble(rsrc, ftype, creator, flags=0):
    """Build an AppleDouble header file (`._name`) - used by the tests."""
    finfo = ftype + creator + struct.pack(">Hhh", flags, 0, 0) + b"\0" * 22
    entries = [(9, finfo), (2, rsrc)]
    off = 26 + 12 * len(entries)
    head = struct.pack(">II16sH", APPLEDOUBLE_MAGIC, 0x00020000, b"", len(entries))
    body = b""
    for eid, blob in entries:
        head += struct.pack(">III", eid, off + len(body), len(blob))
        body += blob
    return head + body
