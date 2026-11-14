#!/usr/bin/env python3
"""run_uitest.py - build and run the headless PlusUI test, write each frame as a PNG.

    python tools/uidesigner/test/run_uitest.py [--out DIR] [--src FILE.c ...]

Compiles PlusGFX + PlusUI + a test program with the P68 toolchain into a flat binary,
runs it on the emulator's bare CPU core (node emulator/tools/run68k.mjs) and converts
the frames the test dumps into PNG files. Needs a finished `make.cmd apps` build (for
libc.a and libp68rt.a).
"""
import argparse
import os
import struct
import subprocess
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))


def png(path, w, h, rows, scale=2):
    raw = bytearray()
    for y in range(h):
        line = bytearray()
        for x in range(w):
            on = rows[y][x >> 3] & (0x80 >> (x & 7))
            line += (b"\x00" if on else b"\xff") * scale
        for _ in range(scale):
            raw += b"\x00" + line

    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xffffffff)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n"
                + chunk(b"IHDR", struct.pack(">IIBBBBB", w * scale, h * scale, 8, 0, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b""))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(REPO, "build_p68", "uitest"))
    ap.add_argument("--sdk", default="macplus")
    ap.add_argument("--src", nargs="*", default=[os.path.join(HERE, "uitest.c")],
                    help="test sources (default: uitest.c)")
    ap.add_argument("-I", dest="inc", action="append", default=[])
    ap.add_argument("-D", dest="defs", action="append", default=[],
                    help="FILE:NAME=VALUE defines NAME only when compiling FILE (basename)")
    a = ap.parse_args()
    os.chdir(REPO)
    os.makedirs(a.out, exist_ok=True)
    exe = ".exe" if os.name == "nt" else ""
    cc = os.path.join("toolchain", "bin", "p68-cc" + exe)
    ld = os.path.join("toolchain", "bin", "p68-ld" + exe)
    sdk = "sdks/" + a.sdk
    inc = ["-I" + sdk + "/include/libc", "-I" + sdk + "/include", "-I" + HERE] + ["-I" + i for i in a.inc]
    srcs = ["toolchain/tests/cc/crt0.S", sdk + "/src/gfx/gfx.c", sdk + "/src/gfx/gfx_asm.S",
            sdk + "/src/gfx/ui.c", sdk + "/src/runtime/font8x8.c"] + a.src
    objs = []
    for s in srcs:
        o = os.path.join(a.out, os.path.basename(s) + ".o")
        defs = ["-D" + d.split(":", 1)[1] for d in a.defs if d.split(":", 1)[0] == os.path.basename(s)]
        subprocess.check_call([cc, "-m68000", "-O2", "-w"] + inc + defs + ["-c", s, "-o", o])
        objs.append(o)
    binp = os.path.join(a.out, "uitest.bin")
    subprocess.check_call([ld, "--oformat", "binary", "-Ttext", "0x10000", "-e", "_start", "-o", binp] + objs +
                          ["--start-group", "build_p68/sdk_" + a.sdk + "/lib/libc.a",
                           "build_p68/rt/libp68rt.a", "--end-group"])
    r = subprocess.run(["node", "emulator/tools/run68k.mjs", "--max-insns", "2000000000", binp],
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    lines = r.stdout.decode("latin-1").split("\n")
    i, frames = 0, 0
    while i < len(lines):
        ln = lines[i]
        if ln.startswith("FRAME "):
            _, name, w, h, _rb = ln.split()
            w, h = int(w), int(h)
            rows = [bytes.fromhex(lines[i + 1 + y]) for y in range(h)]
            png(os.path.join(a.out, name + ".png"), w, h, rows)
            frames += 1
            i += h + 1
        else:
            if ln.strip():
                print(ln)
            i += 1
    sys.stderr.write(r.stderr.decode("latin-1"))
    print("%d frames written to %s (exit %d)" % (frames, a.out, r.returncode))
    return r.returncode


if __name__ == "__main__":
    sys.exit(main())
