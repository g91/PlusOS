#!/usr/bin/env python3
"""
decode_traps.py - extract the exact set of A-line traps implemented by a
Macintosh Plus ROM, straight from the ROM image.

The ROM keeps a delta-compressed dispatch table (TrapDispatchTbl). At boot,
code in vectors.s (DECODE_DISP_LOOP) expands it into the Toolbox dispatch
table (512 entries at $0C00.. in RAM, traps $A800-$A9FF) and the OS dispatch
table (256 entries at $0400, traps $A000-$A0FF). This script runs the same
algorithm, instruction-for-instruction, over the ROM bytes, then names every
handler address using the ROM's symbol map.

Usage:
    decode_traps.py ROM_IMAGE ROM_MAP OUT_JSON

ROM_MAP is a map file such as mac_rom/maps/rom-mac_plus_v3.lst.txt
("LABEL 0xOFFSET l" per line).
"""
import json
import sys


def load_map(path):
    by_addr = {}
    by_name = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            parts = line.split()
            if len(parts) < 2 or not parts[1].startswith("0x"):
                continue
            name, off = parts[0], int(parts[1], 16)
            by_name[name] = off
            by_addr.setdefault(off, []).append(name)
    return by_addr, by_name


def s16(v):
    v &= 0xFFFF
    return v - 0x10000 if v & 0x8000 else v


def decode(rom, start):
    """Emulate DECODE_DISP_LOOP. Returns (tool[512], os[256]) of ROM offsets or None."""
    tool = [None] * 512
    ost = [None] * 256
    p = start
    a2 = 0                      # A2 = ROMBase (we work in ROM offsets)
    table, idx, remaining, in_os = tool, 0, 0x200, False
    while True:
        b = rom[p]
        p += 1
        store = True
        if b & 0x80:                        # DISP_PACKED_ENTRY
            v = b & 0x7F
            if v == 0:                      # skip -> Unimplemented
                store = False
            elif v == 0x7F:                 # 4-byte absolute ROM offset
                off = int.from_bytes(rom[p:p + 4], "big")
                p += 4
                a2 = off
            else:                           # short positive delta (words)
                a2 += s16(v * 2)
        else:                               # 2-byte signed word delta
            w = (b << 8) | rom[p]
            p += 1
            d = s16(w * 2)
            if d == 0:                      # end of table (-> DETECT_CPU_TYPE)
                break
            a2 += d
        if store:
            table[idx] = a2
        idx += 1                            # DISP_NEXT_ENTRY
        remaining -= 1
        if not in_os and remaining == 0:
            table, idx, remaining, in_os = ost, 0, 1 << 30, True
        if in_os and idx > 256:
            raise SystemExit("OS table overflow - decoder out of sync")
    return tool, ost, p - start


def main():
    rom_path, map_path, out_path = sys.argv[1:4]
    rom = open(rom_path, "rb").read()
    by_addr, by_name = load_map(map_path)
    # Plus maps call the table TrapDispatchTbl; the SE map (Apple's MacSEROM.lst
    # names) calls it DISPTABLE, and the SE also stores its offset at DISPOFF.
    if "TrapDispatchTbl" in by_name:
        start = by_name["TrapDispatchTbl"]
    elif "DISPTABLE" in by_name:
        start = by_name["DISPTABLE"]
    elif "DISPOFF" in by_name:
        start = int.from_bytes(rom[by_name["DISPOFF"]:by_name["DISPOFF"] + 4], "big")
    else:
        raise SystemExit("no dispatch table label (TrapDispatchTbl / DISPTABLE) in map")
    tool, ost, used = decode(rom, start)

    traps = []
    for base, table in ((0xA800, tool), (0xA000, ost)):
        for i, off in enumerate(table):
            if off is None:
                continue
            names = by_addr.get(off, [])
            traps.append({
                "trap": base + i,
                "rom_offset": off,
                "rom_labels": names,
            })
    checksum = int.from_bytes(rom[0:4], "big")
    version = int.from_bytes(rom[8:10], "big")
    result = {
        "rom_checksum": "0x%08X" % checksum,
        "rom_version": "0x%04X" % version,
        "rom_size": len(rom),
        "table_bytes": used,
        "toolbox_count": sum(1 for t in tool if t is not None),
        "os_count": sum(1 for t in ost if t is not None),
        "traps": traps,
    }
    with open(out_path, "w") as f:
        json.dump(result, f, indent=1)
    unnamed = [t for t in traps if not t["rom_labels"]]
    print("ROM %s v%s: %d Toolbox + %d OS traps implemented, table %d bytes, %d handlers unlabeled"
          % (result["rom_checksum"], result["rom_version"], result["toolbox_count"],
             result["os_count"], used, len(unnamed)))


if __name__ == "__main__":
    main()
