"""MacRoman file names: encoding, the HFS catalog ordering, host-name escaping.

HFS stores names as MacRoman Pascal strings (1..31 bytes, ':' is illegal) and
orders catalog keys by parent directory ID, then by name using the File
Manager's RelString ordering: case-insensitive, and with accented letters
sorted next to (but not equal to) their base letter.  That ordering is NOT
ASCII - e.g. '_' sorts after 'z', '`' sorts among the 'A' variants, and
'a-umlaut' equals 'A-umlaut' - and a catalog B-tree written in any other
order makes the Mac (and hfsutils) fail to find files.

ORDER below is the classic 256-entry table (the same one the Mac OS ROM, the
Linux hfs driver and hfsutils use; it was checked byte-for-byte against the
table embedded in the hfsutils binary).
"""
import re
import unicodedata

ORDER = bytes([
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x22, 0x23, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
    0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,
    0x47, 0x48, 0x58, 0x5A, 0x5E, 0x60, 0x67, 0x69, 0x6B, 0x6D, 0x73, 0x75, 0x77, 0x79, 0x7B, 0x7F,
    0x8D, 0x8F, 0x91, 0x93, 0x96, 0x98, 0x9F, 0xA1, 0xA3, 0xA5, 0xA8, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE,
    0x54, 0x48, 0x58, 0x5A, 0x5E, 0x60, 0x67, 0x69, 0x6B, 0x6D, 0x73, 0x75, 0x77, 0x79, 0x7B, 0x7F,
    0x8D, 0x8F, 0x91, 0x93, 0x96, 0x98, 0x9F, 0xA1, 0xA3, 0xA5, 0xA8, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3,
    0x4C, 0x50, 0x5C, 0x62, 0x7D, 0x81, 0x9A, 0x55, 0x4A, 0x56, 0x4C, 0x4E, 0x50, 0x5C, 0x62, 0x64,
    0x65, 0x66, 0x6F, 0x70, 0x71, 0x72, 0x7D, 0x89, 0x8A, 0x8B, 0x81, 0x83, 0x9C, 0x9D, 0x9E, 0x9A,
    0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0x95, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0x52, 0x85,
    0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0x57, 0x8C, 0xCC, 0x52, 0x85,
    0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0x26, 0x27, 0xD4, 0x20, 0x4A, 0x4E, 0x83, 0x87, 0x87,
    0xD5, 0xD6, 0x24, 0x25, 0x2D, 0x2E, 0xD7, 0xD8, 0xA7, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
])
assert len(ORDER) == 256

MAX_NAME = 31          # file / folder names
MAX_VOLNAME = 27       # volume names


class NameError_(ValueError):
    """A host name that cannot be turned into a legal HFS name."""


def sort_key(name):
    """Collation key of a MacRoman name: compare the results with <, ==, >."""
    return bytes(name).translate(ORDER)


def compare(a, b):
    """RelString-style three-way comparison of two MacRoman names."""
    ka, kb = sort_key(a), sort_key(b)
    return (ka > kb) - (ka < kb)


def catalog_key(parent_id, name):
    """Sort key of a catalog record: parent CNID first, then the name."""
    return (parent_id, sort_key(name))


def decode(raw):
    """MacRoman bytes -> str."""
    return bytes(raw).decode("mac_roman")


def encode(text, strict=False, limit=MAX_NAME, warn=None):
    """str -> MacRoman HFS name.

    Characters MacRoman cannot represent are transliterated (accents stripped,
    otherwise '_') and ':' becomes '-', unless `strict`, in which case they are
    an error.  Names that are empty or longer than `limit` bytes are always an
    error: silent truncation would create colliding names.
    """
    text = unicodedata.normalize("NFC", text)
    out = bytearray()
    changed = False
    for ch in text:
        if ch == ":" or ch == "\0":
            if strict:
                raise NameError_("%r: character %r is not allowed in an HFS name" % (text, ch))
            out += b"-"
            changed = True
            continue
        try:
            out += ch.encode("mac_roman")
            continue
        except UnicodeEncodeError:
            pass
        if strict:
            raise NameError_("%r: character %r (U+%04X) is not in MacRoman" % (text, ch, ord(ch)))
        changed = True
        base = "".join(c for c in unicodedata.normalize("NFKD", ch) if not unicodedata.combining(c))
        try:
            enc = base.encode("mac_roman")
            out += enc if enc and b":" not in enc else b"_"
        except UnicodeEncodeError:
            out += b"_"
    if not out:
        raise NameError_("empty name")
    if len(out) > limit:
        raise NameError_("%r is %d bytes in MacRoman; HFS allows at most %d" % (text, len(out), limit))
    if changed and warn:
        warn("name %r is not representable in MacRoman; stored as %r" % (text, decode(out)))
    return bytes(out)


# ---- host file names --------------------------------------------------------
# A Mac name may contain characters a host file system refuses ('/', and on
# Windows  < > : " \ | ? *, control characters, trailing dots/spaces, device
# names).  Such bytes are written %XX (hex of the MacRoman byte) in the staged
# folder, and %XX is turned back into the byte when the folder is imaged.

_HOST_BAD = set('<>:"/\\|?*%')
_WIN_DEVICES = {"CON", "PRN", "AUX", "NUL"} | {"COM%d" % i for i in range(1, 10)} | {"LPT%d" % i for i in range(1, 10)}
_ESC = re.compile(r"%([0-9A-Fa-f]{2})")


def _esc(ch):
    return "".join("%%%02X" % b for b in ch.encode("mac_roman"))


def to_host(raw, reserved=()):
    """MacRoman HFS name -> a host file name that is legal on Windows and Unix."""
    text = decode(raw)
    out = []
    for ch in text:
        if ch in _HOST_BAD or ord(ch) < 32 or ord(ch) == 127:
            out.append(_esc(ch))
        else:
            out.append(ch)
    if out and out[-1] in (".", " "):
        out[-1] = _esc(out[-1])
    name = "".join(out)
    stem = name.split(".")[0].upper()
    if stem in _WIN_DEVICES or name.startswith("._") or name in reserved or name in (".", ".."):
        name = _esc(name[0]) + name[1:]
    return unicodedata.normalize("NFC", name)


def from_host(name, strict=False, limit=MAX_NAME, warn=None):
    """Host file name -> MacRoman HFS name (undoes the %XX escapes of to_host)."""
    text = _ESC.sub(lambda m: bytes([int(m.group(1), 16)]).decode("mac_roman"), name)
    if ":" in text and ":" not in name:
        raise NameError_("%r: an escaped ':' is not allowed in an HFS name" % name)
    return encode(text, strict=strict, limit=limit, warn=warn)
