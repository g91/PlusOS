#!/usr/bin/env python3
"""plusdump - read a PlusOS 'PlusOS CrashDump' (PDMP) file.

A post-mortem viewer for PlusOS crashes.  The kernel writes a binary dump when
a program or the kernel faults; this decodes it: the exception, the full
register file, the process, a disassembly of the code around the crash PC, the
stack, the memory at the fault address, and the tail of the kernel log.

Usage:
    python3 plusdump.py <dumpfile>            # full report
    python3 plusdump.py <dumpfile> --json     # machine-readable (for the GUI)
    python3 plusdump.py <dumpfile> --code 48  # disassemble N bytes of CODE

The dump is big-endian (68k).  See kernel/cpu/crashdump.c for the format.
"""
import sys
import os
import json
import struct

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dis68k

VECTORS = {2: "BUS ERROR", 3: "ADDRESS ERROR", 4: "ILLEGAL INSTRUCTION",
           5: "DIVIDE BY ZERO", 6: "CHK EXCEPTION", 7: "TRAPV EXCEPTION",
           8: "PRIVILEGE VIOLATION", 9: "TRACE", 10: "LINE-A (1010)",
           11: "LINE-F (1111)", 0xFFFF: "SYSTEM ERROR (_SysError)"}
SYSERR = {1: "bus error", 2: "address error", 3: "illegal instruction",
          12: "unimplemented trap", 15: "segment loader", 25: "out of memory",
          26: "bad program launch", 28: "stack ran into heap"}
MACHINE = {0: "unknown", 1: "Mac Plus", 2: "Mac SE", 3: "Mac Classic",
           4: "Mac LC III"}
# traps.h may differ; show the raw number too.


def be16(b, o): return (b[o] << 8) | b[o + 1]
def be32(b, o): return (b[o] << 24) | (b[o + 1] << 16) | (b[o + 2] << 8) | b[o + 3]


def parse(data):
    if data[:4] != b"PDMP":
        raise ValueError("not a PDMP crash dump (bad magic %r)" % data[:4])
    d = {
        "version": be16(data, 4),
        "machine": be16(data, 6),
        "vector": be16(data, 8),
        "syserr": be16(data, 10),
        "ticks": be32(data, 12),
        "sr": be16(data, 16),
        "pc": be32(data, 20),
        "fault_addr": be32(data, 24),
        "load_base": be32(data, 28),
        "memtop": be32(data, 32),
        "chunks": {},
    }
    o = 36
    while o + 8 <= len(data):
        tag = data[o:o + 4].decode("latin1")
        ln = be32(data, o + 4)
        payload = data[o + 8:o + 8 + ln]
        o += 8 + ln
        if tag.strip() == "END":
            break
        d["chunks"][tag] = payload
    # registers
    if "REGS" in d["chunks"]:
        p = d["chunks"]["REGS"]
        regs = [be32(p, i * 4) for i in range(len(p) // 4)]
        d["regs"] = regs
    # process
    if "PROC" in d["chunks"]:
        p = d["chunks"]["PROC"]
        name = p[4:20].split(b"\0")[0].decode("latin1", "replace")
        d["proc"] = {
            "pid": be16(p, 0), "ring": p[2], "name": name,
            "image": be32(p, 20), "kstack": be32(p, 24),
            "kstack_end": be32(p, 28), "ustack": be32(p, 32),
        }
    for tag in ("STAK", "CODE", "FALT"):
        if tag in d["chunks"]:
            p = d["chunks"][tag]
            d[tag] = {"addr": be32(p, 0), "len": be32(p, 4), "bytes": p[8:]}
    if "MMAP" in d["chunks"] and len(d["chunks"]["MMAP"]) >= 28:
        p = d["chunks"]["MMAP"]
        d["mmap"] = {
            "syszone": be32(p, 0), "applzone": be32(p, 4), "appllimit": be32(p, 8),
            "heapend": be32(p, 12), "memtop": be32(p, 16),
            "load_base": be32(p, 20), "kstack_bottom": be32(p, 24),
        }
    if "KLOG" in d["chunks"]:
        p = d["chunks"]["KLOG"]
        d["klog"] = p[8:].decode("latin1", "replace")
    return d


def hexdump(addr, b, mark=None, width=16):
    out = []
    for i in range(0, len(b), width):
        row = b[i:i + width]
        hexs = " ".join("%02x" % x for x in row)
        asci = "".join(chr(x) if 32 <= x < 127 else "." for x in row)
        a = addr + i
        flag = " <-- fault" if (mark is not None and a <= mark < a + width) else ""
        out.append("  %08x  %-47s  %s%s" % (a, hexs, asci, flag))
    return "\n".join(out)


def report(d):
    L = []
    vec = d["vector"]
    vname = VECTORS.get(vec, "EXCEPTION vector %d" % vec)
    L.append("=" * 68)
    L.append("  PlusOS Crash Dump")
    L.append("=" * 68)
    mname = MACHINE.get(d["machine"], "machine %d" % d["machine"])
    L.append("  Machine : %s        t=%u ticks (%.1fs)" %
             (mname, d["ticks"], d["ticks"] / 60.0))
    if vec == 0xFFFF:
        L.append("  Fault   : %s  code %d (%s)" %
                 (vname, d["syserr"], SYSERR.get(d["syserr"], "?")))
    else:
        L.append("  Fault   : %s" % vname)
    pc = d["pc"]
    base = d["load_base"]
    where = ""
    if base and base <= pc < base + 0x40000:
        where = "  (kernel + 0x%05x)" % (pc - base)
    elif d.get("proc") and d["proc"]["image"] and d["proc"]["image"] <= pc < d["proc"]["image"] + 0x80000:
        where = "  (%s + 0x%x)" % (d["proc"]["name"], pc - d["proc"]["image"])
    L.append("  PC      : %08x%s" % (pc, where))
    L.append("  SR      : %04x" % d["sr"])
    if vec in (2, 3):
        L.append("  Access  : %08x   <-- faulting address" % d["fault_addr"])
    if d.get("proc"):
        p = d["proc"]
        ring = {0: "kernel", 2: "user"}.get(p["ring"], str(p["ring"]))
        L.append("  Process : pid %d  '%s'  ring %s" % (p["pid"], p["name"], ring))
        L.append("            image=%08x kstack=%08x..%08x ustack=%08x" %
                 (p["image"], p["kstack"], p["kstack_end"], p["ustack"]))

    if d.get("mmap"):
        m = d["mmap"]
        L.append("-" * 68)
        L.append("  Memory map")
        L.append("   SysZone   %08x" % m["syszone"])
        L.append("   ApplZone  %08x   (app heap base)" % m["applzone"])
        L.append("   ApplLimit %08x   (app heap ceiling)" % m["appllimit"])
        L.append("   HeapEnd   %08x" % m["heapend"])
        L.append("   kernel    %08x   (stack bottom %08x)" % (m["load_base"], m["kstack_bottom"]))
        L.append("   MemTop    %08x" % m["memtop"])
        if d.get("proc"):
            pr = d["proc"]
            for label, addr in (("image", pr["image"]), ("ustack", pr["ustack"]), ("kstack", pr["kstack"])):
                if addr and not (m["applzone"] <= addr < m["appllimit"]):
                    L.append("   ! %-6s %08x is OUTSIDE the app heap [%08x,%08x) — possible overlap" %
                             (label, addr, m["applzone"], m["appllimit"]))

    if "regs" in d and len(d["regs"]) >= 17:
        rg = d["regs"]
        L.append("-" * 68)
        L.append("  Registers")
        for i in range(0, 8, 4):
            L.append("   " + "  ".join("D%d=%08x" % (j, rg[j]) for j in range(i, i + 4)))
        for i in range(0, 8, 4):
            L.append("   " + "  ".join("A%d=%08x" % (j, rg[8 + j]) for j in range(i, i + 4)))
        if len(d["regs"]) >= 18:
            L.append("   USP=%08x  PC=%08x  SR=%04x" % (rg[17], rg[16], d["sr"]))

    if "CODE" in d:
        c = d["CODE"]
        L.append("-" * 68)
        L.append("  Disassembly around PC  (code @ %08x)" % c["addr"])
        L.append(disasm_window(c["addr"], c["bytes"], pc))

    if "STAK" in d:
        s = d["STAK"]
        L.append("-" * 68)
        L.append("  Supervisor stack  @ %08x" % s["addr"])
        L.append(stack_view(s["addr"], s["bytes"], base, d.get("proc")))

    if "FALT" in d:
        f = d["FALT"]
        L.append("-" * 68)
        L.append("  Memory at fault address  @ %08x" % f["addr"])
        L.append(hexdump(f["addr"], f["bytes"], mark=d["fault_addr"]))

    if d.get("klog"):
        L.append("-" * 68)
        L.append("  Kernel log (tail)")
        for line in d["klog"].rstrip().splitlines()[-24:]:
            L.append("  | " + line)
    L.append("=" * 68)
    return "\n".join(L)


def disasm_window(addr, code, pc):
    out = []
    off = 0
    while off < len(code):
        a = addr + off
        try:
            ln, txt = dis68k.disasm(code, off)
        except Exception as e:
            ln, txt = 2, "dc.w  ???? (%s)" % e
        if ln < 2:
            ln = 2
        raw = " ".join("%02x" % x for x in code[off:off + ln])
        arrow = ">>" if a <= pc < a + ln else "  "
        out.append("  %s %08x: %-14s %s" % (arrow, a, raw, txt))
        off += ln
    return "\n".join(out)


def stack_view(addr, b, base, proc):
    out = []
    for i in range(0, len(b) - 3, 4):
        v = be32(b, i)
        note = ""
        if base and base <= v < base + 0x40000:
            note = "  kernel+0x%05x" % (v - base)
        elif proc and proc["image"] and proc["image"] <= v < proc["image"] + 0x80000:
            note = "  %s+0x%x" % (proc["name"], v - proc["image"])
        elif proc and proc["kstack"] and proc["kstack"] <= v < proc["kstack_end"]:
            note = "  (kstack)"
        out.append("  %08x: %08x%s" % (addr + i, v, note))
    return "\n".join(out)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    opts = [a for a in sys.argv[1:] if a.startswith("--")]
    if not args:
        print(__doc__)
        return 2
    with open(args[0], "rb") as f:
        data = f.read()
    d = parse(data)
    if "--json" in opts:
        def enc(o):
            if isinstance(o, (bytes, bytearray)):
                return list(o)
            raise TypeError
        print(json.dumps(d, default=enc, indent=1))
        return 0
    print(report(d))
    return 0


if __name__ == "__main__":
    sys.exit(main())
