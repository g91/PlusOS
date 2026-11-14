#!/usr/bin/env python3
"""undef.py - list symbols referenced by the Python objects that no object/library defines."""
import os, struct, sys, glob
REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "..", ".."))

def elf_syms(data):
    if data[:4] != b"\x7fELF": return set(), set()
    shoff, = struct.unpack(">I", data[0x20:0x24]); shentsize, shnum = struct.unpack(">HH", data[0x2e:0x32])
    secs = []
    for i in range(shnum):
        o = shoff + i * shentsize
        name, typ, flags, addr, off, size, link, info, align, entsize = struct.unpack(">IIIIIIIIII", data[o:o+40])
        secs.append((typ, off, size, link, entsize))
    d, u = set(), set()
    for typ, off, size, link, entsize in secs:
        if typ != 2: continue
        stroff = secs[link][1]
        for j in range(size // 16):
            nm, val, sz, info, other, shndx = struct.unpack(">IIIBBH", data[off+j*16:off+j*16+16])
            if nm == 0: continue
            e = data.index(b"\0", stroff + nm); name = data[stroff+nm:e].decode()
            bind = info >> 4
            if bind == 0: continue
            if shndx == 0: u.add(name)
            else: d.add(name)
    return d, u

def ar_members(path):
    data = open(path, "rb").read(); p = 8
    while p < len(data):
        hdr = data[p:p+60]; size = int(hdr[48:58]); p += 60
        yield data[p:p+size]; p += size + (size & 1)

def main():
    objs = glob.glob(os.path.join(REPO, "build_p68", "python", "*.o"))
    defs, undef = set(), set()
    for o in objs:
        d, u = elf_syms(open(o, "rb").read()); defs |= d; undef |= u
    libdir = os.path.join(REPO, "build_p68", "sdk_macplus", "lib")
    for lib in glob.glob(libdir + "/*.a") + [os.path.join(REPO, "build_p68", "rt", "libp68rt.a")]:
        for m in ar_members(lib):
            d, u = elf_syms(m); defs |= d
    miss = sorted(undef - defs)
    print(len(miss), "undefined"); print(" ".join(miss))

main()
