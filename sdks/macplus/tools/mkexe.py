#!/usr/bin/env python3
"""
mkexe.py - turn a PlusOS ELF (linked at 0 with --emit-relocs) into a .pex.

    mkexe.py input.elf output.pex [--name NAME]

Collects every absolute 32-bit relocation (R_68K_32) that points into the
image and appends their offsets as the relocation table that crt0 applies at
load time. PC-relative relocations need nothing; absolute 8/16-bit references
to relocatable symbols cannot be fixed up and are reported as errors.
"""
import argparse
import struct
import sys

SHT_SYMTAB, SHT_RELA, SHT_REL, SHT_NOBITS = 2, 4, 9, 8
SHF_ALLOC = 2
SHN_UNDEF, SHN_ABS = 0, 0xFFF1
STB_WEAK = 2
R_NAMES = {0: "NONE", 1: "32", 2: "16", 3: "8", 4: "PC32", 5: "PC16", 6: "PC8"}


def read_elf(data):
    if data[:4] != b"\x7fELF" or data[4] != 1 or data[5] != 2:
        raise SystemExit("not a 32-bit big-endian ELF")
    (e_shoff,) = struct.unpack_from(">I", data, 0x20)
    e_shentsize, e_shnum, e_shstrndx = struct.unpack_from(">HHH", data, 0x2E)
    secs = []
    for i in range(e_shnum):
        f = struct.unpack_from(">IIIIIIIIII", data, e_shoff + i * e_shentsize)
        secs.append(dict(name_off=f[0], type=f[1], flags=f[2], addr=f[3], offset=f[4],
                         size=f[5], link=f[6], info=f[7], align=f[8], entsize=f[9]))
    strtab = secs[e_shstrndx]
    for s in secs:
        o = strtab["offset"] + s["name_off"]
        s["name"] = data[o:data.index(b"\0", o)].decode()
    return secs


def symbols(data, secs):
    out = []
    for s in secs:
        if s["type"] != SHT_SYMTAB:
            continue
        strs = secs[s["link"]]
        for i in range(s["size"] // 16):
            name_off, value, size, info, other, shndx = struct.unpack_from(">IIIBBH", data, s["offset"] + i * 16)
            o = strs["offset"] + name_off
            name = data[o:data.index(b"\0", o)].decode()
            out.append(dict(name=name, value=value, bind=info >> 4, shndx=shndx))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("elf")
    ap.add_argument("out")
    ap.add_argument("--name", default="")
    a = ap.parse_args()

    data = open(a.elf, "rb").read()
    secs = read_elf(data)
    syms = symbols(data, secs)
    by_name = {s["name"]: s["value"] for s in syms if s["name"]}
    for need in ("__image_end", "__bss_start", "__bss_end"):
        if need not in by_name:
            raise SystemExit("%s: missing linker symbol %s (wrong linker script?)" % (a.elf, need))
    image_end = by_name["__image_end"]
    bss_end = by_name["__bss_end"]

    # lay out the loadable, file-backed sections
    image = bytearray(image_end)
    for s in secs:
        if s["flags"] & SHF_ALLOC and s["type"] != SHT_NOBITS and s["size"]:
            if s["addr"] + s["size"] > image_end:
                raise SystemExit("section %s lies beyond __image_end" % s["name"])
            image[s["addr"]:s["addr"] + s["size"]] = data[s["offset"]:s["offset"] + s["size"]]

    if image[4:8] != b"PLOS":
        raise SystemExit("%s: no PLOS header at offset 4 (crt0 must be linked first)" % a.elf)

    relocs, errors = [], []
    symtab_sec = None
    for s in secs:
        if s["type"] == SHT_REL:
            raise SystemExit("REL sections are not supported (expected RELA)")
        if s["type"] != SHT_RELA:
            continue
        target = secs[s["info"]]
        if not (target["flags"] & SHF_ALLOC) or target["type"] == SHT_NOBITS:
            continue                                  # debug info etc.
        symtab_sec = secs[s["link"]]
        strs = secs[symtab_sec["link"]]
        for i in range(s["size"] // 12):
            r_offset, r_info, r_addend = struct.unpack_from(">IIi", data, s["offset"] + i * 12)
            rtype, rsym = r_info & 0xFF, r_info >> 8
            if rtype in (0, 4, 5, 6):                 # NONE / PC-relative: position independent
                continue
            name_off, value, size, info, other, shndx = struct.unpack_from(
                ">IIIBBH", data, symtab_sec["offset"] + rsym * 16)
            o = strs["offset"] + name_off
            symname = data[o:data.index(b"\0", o)].decode() or "<section>"
            if shndx == SHN_ABS:
                continue                              # absolute address (e.g. low memory)
            if shndx == SHN_UNDEF:
                if (info >> 4) == STB_WEAK:
                    continue                          # undefined weak = 0, stays 0
                errors.append("undefined symbol %s" % symname)
                continue
            if rtype == 1:                            # R_68K_32
                if r_offset & 1:
                    errors.append("odd relocation address 0x%X (%s)" % (r_offset, symname))
                relocs.append(r_offset)
            else:
                errors.append("R_68K_%s to relocatable symbol %s at 0x%X cannot be relocated"
                              % (R_NAMES.get(rtype, rtype), symname, r_offset))
    if errors:
        for e in errors[:30]:
            print("mkexe: error:", e, file=sys.stderr)
        raise SystemExit(1)

    relocs = sorted(set(relocs))
    reloc_offset = len(image)
    file_size = reloc_offset + 4 * len(relocs)
    mem_size = max(file_size, bss_end)
    mem_size = (mem_size + 3) & ~3
    struct.pack_into(">IIII", image, 0x0C, file_size, mem_size, reloc_offset, len(relocs))
    name = (a.name or "").encode()[:11]
    image[0x24:0x30] = name.ljust(12, b"\0")

    with open(a.out, "wb") as f:
        f.write(image)
        for r in relocs:
            f.write(struct.pack(">I", r))
    print("mkexe: %-12s image %6d  relocs %5d  bss %6d  file %6d  mem %6d"
          % (a.name or a.out, reloc_offset, len(relocs), bss_end - image_end, file_size, mem_size))


if __name__ == "__main__":
    main()
