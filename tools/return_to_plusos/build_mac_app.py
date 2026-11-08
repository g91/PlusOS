#!/usr/bin/env python3
"""
build_mac_app.py - Build the complete Classic Macintosh "Return to PlusOS" application:
  - Generates 'CODE' 0, 'CODE' 1, 'ICN#', 'BNDL', 'FREF', and 'vers' resources.
  - Generates raw Resource Fork (.rsrc).
  - Generates MacBinary II container (.bin) for easy drag-and-drop or transfer.
  - Generates C++ embedded header (return_to_plusos_payload.h) for PlusOS Disk Studio.
"""
import struct
import os

CODE1_BYTES = bytes([
    0x00, 0x00, 0x00, 0x01, 0x3F, 0x3C, 0x00, 0x1E, 0xA9, 0xC8, 0x34, 0x3C, 0xFF, 0xD9, 0x42, 0x43,
    0x22, 0x78, 0x03, 0x0A, 0xB2, 0xFC, 0x00, 0x00, 0x67, 0x10, 0xB4, 0x69, 0x00, 0x08, 0x66, 0x06,
    0x36, 0x29, 0x00, 0x06, 0x60, 0x04, 0x22, 0x51, 0x60, 0xEA, 0x59, 0x8F, 0x3E, 0x83, 0x3F, 0x42,
    0x00, 0x02, 0x20, 0x4F, 0x20, 0x3C, 0x00, 0x04, 0x00, 0x78, 0xA0, 0x52, 0x58, 0x8F, 0x22, 0x3C,
    0x00, 0x02, 0x00, 0x00, 0x53, 0x81, 0x66, 0xFC, 0xA8, 0x88, 0xA9, 0xF4, 0x4E, 0x75
])

# 'CODE' 0: A5 World descriptor + jump table entry for segment 1
CODE0_BYTES = struct.pack(">IIIIHHHh",
    0x00000100,  # aboveA5: 256 bytes
    0x00000080,  # belowA5: 128 bytes
    0x00000008,  # jtSize: 8 bytes
    0x00000020,  # jtOffset: 32 bytes from A5
    0x0000,      # segment 1 entry offset
    0x3F3C,      # move.w #1, -(sp)
    0x0001,      # segment 1
    0xA9F0 - 0x10000  # _LoadSeg ($A9F0) as signed 16-bit
)

# 32x32 Icon for "Return to PlusOS": Mac with big "+" symbol in center
icon_data = bytearray(128)
icon_mask = bytearray(128)

# Draw a classic Macintosh outline with a clear '+' symbol:
# 32x32 monochrome bitmap (4 bytes per row * 32 rows)
mac_art = [
    "   ..........................   ",
    "  .                          .  ",
    " .                            . ",
    ".                              .",
    ".   ........................   .",
    ".   .                      .   .",
    ".   .                      .   .",
    ".   .          ##          .   .",
    ".   .          ##          .   .",
    ".   .          ##          .   .",
    ".   .      ##########      .   .",
    ".   .      ##########      .   .",
    ".   .          ##          .   .",
    ".   .          ##          .   .",
    ".   .          ##          .   .",
    ".   .                      .   .",
    ".   .                      .   .",
    ".   ........................   .",
    ".                              .",
    ".                              .",
    ".      ..................      .",
    ".                              .",
    "................................",
    " .                            . ",
    "  .                          .  ",
    "  .                          .  ",
    "  .                          .  ",
    "   ..........................   ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
]

for y in range(32):
    row = mac_art[y]
    for x in range(32):
        ch = row[x]
        byte_idx = y * 4 + (x // 8)
        bit = 7 - (x % 8)
        if ch in ('.', '#'):
            icon_data[byte_idx] |= (1 << bit)
        if ch != ' ':
            icon_mask[byte_idx] |= (1 << bit)

ICON_BYTES = bytes(icon_data) + bytes(icon_mask)

# 'BNDL' ID 128: signature 'PLOS', ID 0, 2 type mappings ('ICN#' and 'FREF')
BNDL_BYTES = struct.pack(">4shh4shhh4shhh",
    b"PLOS", 0, 1,
    b"ICN#", 0, 0, 128,
    b"FREF", 0, 0, 128
)

# 'FREF' ID 128: file type 'APPL', icon local ID 0, empty filename
FREF_BYTES = struct.pack(">4shh", b"APPL", 0, 0)

# 'vers' ID 1: version 1.0.0, release, country 0, "1.0", "Return to PlusOS 1.0"
VERS_BYTES = (
    b"\x01\x00\x80\x00\x00\x00" +
    b"\x03" + b"1.0" +
    b"\x13" + b"Return to PlusOS 1.0"
)

def build_resource_fork():
    resources = [
        (b"CODE", 0, 0x20, CODE0_BYTES),
        (b"CODE", 1, 0x20, CODE1_BYTES),
        (b"ICN#", 128, 0x00, ICON_BYTES),
        (b"BNDL", 128, 0x00, BNDL_BYTES),
        (b"FREF", 128, 0x00, FREF_BYTES),
        (b"vers", 1, 0x00, VERS_BYTES),
    ]

    # Build data section
    # Header is 256 bytes
    data_section = bytearray()
    offsets = []
    for rtype, rid, attr, rdata in resources:
        off = len(data_section)
        offsets.append(off)
        data_section += struct.pack(">I", len(rdata))
        data_section += rdata

    data_len = len(data_section)
    data_offset = 256

    # Group resources by type
    types = {}
    for idx, (rtype, rid, attr, rdata) in enumerate(resources):
        if rtype not in types:
            types[rtype] = []
        types[rtype].append((rid, attr, offsets[idx]))

    # Build map section
    # Map header (30 bytes):
    # copy of header (16 bytes) + next_map(4) + file_ref(2) + attrs(2) + type_list_off(2) + name_list_off(2)
    type_list_off = 30
    num_types = len(types)

    # Type list starts with: num_types - 1 (2 bytes)
    # Then for each type: type (4 bytes) + count - 1 (2 bytes) + ref_list_offset (2 bytes from type_list start)
    # Reference list for that type follows
    type_entries = bytearray()
    ref_entries = bytearray()

    current_ref_off = 2 + num_types * 8

    for rtype, items in types.items():
        type_entries += struct.pack(">4sHH", rtype, len(items) - 1, current_ref_off)
        for rid, attr, data_off in items:
            # ref entry: resID(2) + name_off(2) + attr(1) + data_off(3) + reserved(4)
            ref_entries += struct.pack(">HhB", rid, -1, attr)
            ref_entries += bytes([(data_off >> 16) & 0xFF, (data_off >> 8) & 0xFF, data_off & 0xFF])
            ref_entries += struct.pack(">I", 0)
        current_ref_off += len(items) * 12

    map_section = bytearray(30)
    # copy of header filled later
    struct.pack_into(">HH", map_section, 24, 0, 0) # file ref, attrs
    struct.pack_into(">HH", map_section, 26, type_list_off, type_list_off + len(type_entries) + len(ref_entries))

    map_section += struct.pack(">H", num_types - 1)
    map_section += type_entries
    map_section += ref_entries
    # Name list (empty, just 0 length)
    map_section += b"\x00\x00"

    map_len = len(map_section)
    map_offset = data_offset + data_len

    # Put map header
    header = struct.pack(">IIII", data_offset, map_offset, data_len, map_len)
    map_section[0:16] = header

    # Pad header to 256 bytes
    full_rf = header.ljust(256, b"\x00") + data_section + map_section
    return full_rf

def build_macbinary_ii(name, rf_data, df_data=b""):
    """Wrap data fork and resource fork into standard MacBinary II container."""
    hdr = bytearray(128)
    hdr[0] = 0 # zero
    hdr[1] = len(name)
    hdr[2:2+len(name)] = name.encode("mac_roman")
    hdr[65:69] = b"APPL" # file type
    hdr[69:73] = b"PLOS" # creator
    hdr[73] = 0x00 # Finder flags high (has bundle bit 0x20)
    hdr[73] |= 0x20 # has bundle
    hdr[74] = 0x00
    struct.pack_into(">I", hdr, 83, len(df_data)) # data fork length
    struct.pack_into(">I", hdr, 87, len(rf_data)) # rsrc fork length
    hdr[122] = 129 # MacBinary II version
    hdr[123] = 129 # MacBinary II min version

    # CRC-16 checksum of bytes 0..123
    def calc_crc(data):
        crc = 0
        for b in data:
            crc ^= (b << 8)
            for _ in range(8):
                if crc & 0x8000:
                    crc = ((crc << 1) ^ 0x1021) & 0xFFFF
                else:
                    crc = (crc << 1) & 0xFFFF
        return crc

    crc = calc_crc(hdr[0:124])
    struct.pack_into(">H", hdr, 124, crc)

    # Pad data fork and resource fork to 128-byte boundaries
    df_pad = (len(df_data) + 127) // 128 * 128
    rf_pad = (len(rf_data) + 127) // 128 * 128

    return bytes(hdr) + df_data.ljust(df_pad, b"\x00") + rf_data.ljust(rf_pad, b"\x00")

def main():
    rf = build_resource_fork()
    bin_file = build_macbinary_ii("Return to PlusOS", rf)

    out_dir = os.path.dirname(os.path.abspath(__file__))
    rsrc_path = os.path.join(out_dir, "ReturnToPlusOS.rsrc")
    bin_path = os.path.join(out_dir, "Return to PlusOS.bin")
    header_path = os.path.join(out_dir, "return_to_plusos_payload.h")

    with open(rsrc_path, "wb") as f:
        f.write(rf)
    with open(bin_path, "wb") as f:
        f.write(bin_file)

    # Generate C++ header
    with open(header_path, "w") as f:
        f.write("// Auto-generated by build_mac_app.py - DO NOT EDIT MANUALLY\n")
        f.write("#pragma once\n#include <cstdint>\n#include <cstddef>\n\n")
        f.write("namespace MacAppPayload {\n")
        f.write('    inline const char* APP_NAME = "Return to PlusOS";\n')
        f.write('    inline const char* APP_TYPE = "APPL";\n')
        f.write('    inline const char* APP_CREATOR = "PLOS";\n\n')

        f.write(f"    inline const size_t RSRC_FORK_SIZE = {len(rf)};\n")
        f.write("    inline const uint8_t RSRC_FORK_DATA[] = {\n")
        for i in range(0, len(rf), 16):
            chunk = rf[i:i+16]
            f.write("        " + ", ".join(f"0x{b:02X}" for b in chunk) + ",\n")
        f.write("    };\n\n")

        f.write(f"    inline const size_t MACBINARY_SIZE = {len(bin_file)};\n")
        f.write("    inline const uint8_t MACBINARY_DATA[] = {\n")
        for i in range(0, len(bin_file), 16):
            chunk = bin_file[i:i+16]
            f.write("        " + ", ".join(f"0x{b:02X}" for b in chunk) + ",\n")
        f.write("    };\n")
        f.write("} // namespace MacAppPayload\n")

    print(f"Generated {rsrc_path} ({len(rf)} bytes)")
    print(f"Generated {bin_path} ({len(bin_file)} bytes)")
    print(f"Generated {header_path}")

if __name__ == "__main__":
    main()
