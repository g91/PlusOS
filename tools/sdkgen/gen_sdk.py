#!/usr/bin/env python3
"""
gen_sdk.py - generate the ROM layer of one PlusOS SDK (Mac Plus or Mac SE):
one C header and one C file per ROM manager, plus the cross-cutting headers.

    gen_sdk.py --machine plus|se --traps traps.json --split-inc DIR --split-src DIR
               --defs multiversal/defs --macrom mac_rom --inc OUT/include/<prefix>
               --src OUT/src/<prefix> --docs OUT/docs --stats stats.json

Prefixes: Mac Plus -> plusrom / PLUSROM_ ; Mac SE -> serom / SEROM_.
Outputs (in --inc):  <Manager>.h  MacPrelude.h  allmanagers.h  traps.h  lowmem.h
hw.h  equates.h  catalog.h  <prefix>.h ; (in --src): <Manager>.c  <Manager>.glue.c
catalog.c ; (in --docs): ROM_REFERENCE.md
"""
import argparse
import glob
import json
import os
import re
import shutil

import yaml

MACHINES = {
    "plus": dict(name="Macintosh Plus", short="Mac Plus", prefix="plusrom",
                 selector_limits={0xA060: 0x11, 0xA815: 10},      # FSDispatch, SCSIDispatch
                 rsrc=("RSRC", "plus_rsrc.inc"), lowmem_extra=None),
    "se":   dict(name="Macintosh SE", short="Mac SE", prefix="serom",
                 selector_limits={0xA060: 0x11, 0xA815: 13},      # + SCSISelAtn/MsgIn/MsgOut
                 rsrc=("RSRC", "se_rsrc.inc"), lowmem_extra="lowmem_se.inc"),
}
ROM_PACKAGES = {4: "SANE floating point (FP68K)", 5: "Elementary functions (Elems68K)",
                7: "Binary/decimal conversion"}                 # PACK resources in both ROMs
PACK_TRAPS = {0xA9E7 + n: n for n in range(8)}                   # _Pack0 .. _Pack7
PACK_TRAPS.update({0xA816: 8, 0xA82B: 9, 0xA82C: 10, 0xA82D: 11,  # _Pack8 .. _Pack15: the ROM
                   0xA82E: 12, 0xA82F: 13, 0xA830: 14, 0xA831: 15})  # has the entry, not the code
# In the Plus and SE ROMs, Toolbox trap numbers $A800-$A84F, $A854 and $A857 are
# aliases of OS traps (see the SE trap dispatcher). Later Systems reused some of
# them (AliasDispatch $A823, ResourceDispatch $A822, FontDispatch $A854, ...);
# on these ROMs such a call would run an unrelated OS routine.
def aliased_toolbox(t):
    return 0xA800 <= t <= 0xA84F or t in (0xA854, 0xA857)
DISPATCHER_LABELS = {"fsdispatch": ("fsdispatch", "turbofs", "hfsdispatch")}
GLUE_PROVIDED = {                                                # template/src/rom/glue
    "FSOpen", "OpenRF", "FSRead", "FSWrite", "GetFPos", "SetFPos", "GetEOF", "SetEOF",
    "FSClose", "Create", "FSDelete", "GetFInfo", "SetFInfo", "Rename", "GetVol", "SetVol",
    "FlushVol", "Eject", "GetVInfo", "InitZone"}

PROTO = re.compile(r"^pascal\s+.*?\b([A-Za-z_]\w*)\s*\(.*\)\s*(M68K_INLINE\((.*)\))?\s*;\s*$")
INLINE_DEF = re.compile(r"^__attribute__\(\(__gnu_inline__\)\) extern __inline__ pascal .*?\b([A-Za-z_]\w*)\s*\(")
GLUE_DEF = re.compile(r"^pascal\s+.*?\b([A-Za-z_]\w*)\s*\([^;]*\)\s*\{", re.M)
SHORTS = re.compile(r"0x([0-9A-Fa-f]{4})")


def norm(trap):
    return (0xA800 | (trap & 0x3FF)) if trap & 0x0800 else (0xA000 | (trap & 0xFF))


def ident(s):
    return re.sub(r"[^A-Za-z0-9_]", "_", s)


def c_comment(s):
    return s.replace("*/", "* /")


def write(path, text):
    with open(path, "w", newline="\n") as f:
        f.write(text)


class Gen:
    def __init__(self, a):
        self.a = a
        self.m = MACHINES[a.machine]
        self.p = self.m["prefix"]            # plusrom / serom
        self.P = self.p.upper()              # PLUSROM / SEROM
        j = json.load(open(a.traps))
        self.rj = j
        self.rom = {t["trap"]: t for t in j["traps"]}
        self.funcs, self.dispatchers, self.lowmems = {}, {}, []
        self.results = {}                    # name -> (ok, trap, sel, reason, mgr)

    # ------------------------------------------------------------ inputs
    def load_defs(self):
        for path in sorted(glob.glob(os.path.join(self.a.defs, "*.yaml"))):
            for item in yaml.safe_load(open(path)) or []:
                if not isinstance(item, dict):
                    continue
                if "dispatcher" in item:
                    self.dispatchers[item["dispatcher"]["name"]] = item["dispatcher"]
                elif "lowmem" in item:
                    self.lowmems.append(dict(item["lowmem"]))
                elif "function" in item:
                    f = dict(item["function"])
                    for n in [f["name"]] + [v for v in f.get("variants") or [] if isinstance(v, str)]:
                        self.funcs.setdefault(n, f)

    def labels(self, t):
        return [l.lower() for l in self.rom[t]["rom_labels"]]

    def classify(self, t, sel, name=None, disp=None):
        rom, short = self.rom, self.m["short"]
        if t is None:
            return False, "glue routine with no ROM trap"
        if t not in rom:
            return False, "trap 0x%04X is not in the %s ROM" % (t, short)
        if t in PACK_TRAPS and PACK_TRAPS[t] not in ROM_PACKAGES:
            pk = PACK_TRAPS[t]
            return False, "Package %d (PACK %d) is not in ROM; it loads from a System file" % (pk, pk)
        # A Toolbox slot in the aliased range whose handler is the very same ROM
        # routine as the matching OS trap is an alias, not the call's own code.
        os_twin = 0xA000 | (t & 0xFF)
        if aliased_toolbox(t) and os_twin in rom and rom[os_twin]["rom_offset"] == rom[t]["rom_offset"]:
            what = (rom[os_twin]["rom_labels"] or ["an OS trap"])[-1]
            return False, "trap 0x%04X is only an alias of the OS trap %s in the %s ROM" % (t, what, short)
        if disp and t not in PACK_TRAPS:
            want = DISPATCHER_LABELS.get(disp.lower(), (disp.lower(),))
            if not any(l in want for l in self.labels(t)):
                return False, "trap 0x%04X runs %s in the %s ROM, not the %s dispatcher" % (
                    t, rom[t]["rom_labels"][0] if rom[t]["rom_labels"] else "another routine", short, disp)
        lim = self.m["selector_limits"]
        if t in lim and sel is not None and sel > lim[t]:
            return False, "selector %d of trap 0x%04X is not implemented by the %s ROM" % (sel, t, short)
        return True, ""

    # ------------------------------------------------------------ manager headers
    def prelude(self):
        P = self.P
        return """/* MacPrelude.h - base definitions every %(name)s ROM header starts from.
 * GENERATED by tools/sdkgen. */
#ifndef %(P)s_MACPRELUDE_H
#define %(P)s_MACPRELUDE_H
#pragma GCC system_header
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef __m68k__
#error "the PlusOS SDK targets the 68000 (m68k-apple-macos-gcc)"
#endif
#define TARGET_CPU_68K 1
#define TARGET_CPU_PPC 0
#define TARGET_RT_MAC_CFM 0
#define M68K_INLINE(...) = { __VA_ARGS__ }
#define GetCurrentArchitecture() ((int8_t)0)
#ifndef TARGET_API_MAC_CARBON
#define TARGET_API_MAC_CARBON 0
#endif
#ifndef OLDROUTINENAMES
#define OLDROUTINENAMES 0
#endif

typedef struct RoutineDescriptor *ProcPtr;
#define nil NULL
#define STACK_ROUTINE_PARAMETER(n, sz) ((sz) << (kStackParameterPhase + ((n)-1) * kStackParameterWidth))

/* Calls the %(name)s ROM does not implement are declared with this, so using one
 * is a compile-time error that names the reason. Define %(P)s_ALLOW_UNAVAILABLE
 * when running under a System file that adds them. */
#if defined(%(P)s_ALLOW_UNAVAILABLE)
#define %(P)s_UNAVAILABLE(msg)
#else
#define %(P)s_UNAVAILABLE(msg) __attribute__((unavailable(msg)))
#endif
#endif
""" % dict(name=self.m["name"], P=P)

    def annotate_header(self, text, mgr):
        # the split headers use PLUSROM_ guards/macros as placeholders
        text = text.replace("PLUSROM_", self.P + "_")
        lines = text.split("\n")
        inline_words = {}
        for i, ln in enumerate(lines):
            m = INLINE_DEF.match(ln)
            if m:
                body = "\n".join(lines[i:i + 12])
                inline_words[m.group(1)] = [int(w, 16) for chunk in re.findall(r"\.short\s+([0-9xXa-fA-F, ]+)", body)
                                            for w in SHORTS.findall(chunk)]
        out = []
        for ln in lines:
            m = PROTO.match(ln)
            if m:
                name = m.group(1)
                f = self.funcs.get(name)
                disp = None
                if f is not None:
                    trap, sel, disp = f.get("trap"), f.get("selector"), f.get("dispatcher")
                    if disp and disp in self.dispatchers:
                        trap = trap or self.dispatchers[disp].get("trap")
                    else:
                        disp = None
                    t = norm(trap) if trap is not None else None
                else:
                    words = [int(w, 16) for w in SHORTS.findall(m.group(3) or "")] + inline_words.get(name, [])
                    traps = [w for w in words if 0xA000 <= w <= 0xAFFF]
                    t, sel = (norm(traps[-1]) if traps else None), None
                ok, why = self.classify(t, sel, name, disp)
                if not ok and t is None and (m.group(2) or name in inline_words):
                    ok, why = True, ""
                if not ok and name in GLUE_PROVIDED:
                    ok, why = True, ""
                self.results[name] = (ok, t, sel, why, mgr)
                if not ok:
                    ln = '%s_UNAVAILABLE("%s: %s") %s' % (self.P, name, why, ln)
            out.append(ln)
        return "\n".join(out)

    # ------------------------------------------------------------ catalog
    def catalog_c(self, mgr, calls):
        L = ["/* %s.c - run-time catalog of the %s calls on the %s. GENERATED by tools/sdkgen */"
             % (mgr, mgr, self.m["name"]),
             "#include <%s/catalog.h>" % self.p, "",
             "const %sCall %s_calls_%s[] = {" % (self.cap(), self.p, ident(mgr))]
        for name, (ok, t, sel, _w, _m) in sorted(calls):
            off = self.rom[t]["rom_offset"] if (t is not None and t in self.rom) else 0
            L.append('    { "%s", 0x%04X, %d, %d, 0x%05X },' % (name, t or 0, sel if sel is not None else -1,
                                                              1 if ok else 0, off))
        L += ["};", "const uint16_t %s_count_%s = %d;" % (self.p, ident(mgr), len(calls)), ""]
        return "\n".join(L)

    def cap(self):
        return "PlusRom" if self.p == "plusrom" else "SeRom"

    def catalog_h(self):
        d = dict(P=self.P, p=self.p, C=self.cap(), name=self.m["name"])
        return """/* catalog.h - run-time catalog of every Mac Toolbox/OS call and whether the
 * %(name)s ROM implements it. GENERATED by tools/sdkgen; the tables live in
 * src/%(p)s/<Manager>.c (one per manager) and catalog.c. */
#ifndef %(P)s_CATALOG_H
#define %(P)s_CATALOG_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct %(C)sCall {
    const char *name;
    uint16_t    trap;         /* trap word (0 = no trap) */
    int32_t     selector;     /* dispatch selector, -1 if none */
    uint8_t     in_rom;       /* 1 = callable on the %(name)s */
    uint32_t    rom_offset;   /* handler offset in the %(name)s ROM */
} %(C)sCall;

typedef struct %(C)sManager {
    const char       *name;
    const %(C)sCall  *calls;
    const uint16_t   *count;
} %(C)sManager;

extern const %(C)sManager %(p)s_managers[];
extern const uint16_t     %(p)s_manager_count;

/* Find a call by name (case-insensitive). Returns 0 if unknown; *mgr gets the manager. */
const %(C)sCall *%(p)s_find_call(const char *name, const %(C)sManager **mgr);

#ifdef __cplusplus
}
#endif
#endif
""" % d

    def catalog_index(self, mgrs):
        p, C = self.p, self.cap()
        L = ["/* catalog.c - index of the per-manager call catalogs. GENERATED by tools/sdkgen */",
             "#include <%s/catalog.h>" % p, "#include <string.h>", ""]
        L += ["extern const %sCall %s_calls_%s[]; extern const uint16_t %s_count_%s;"
              % (C, p, ident(m), p, ident(m)) for m in mgrs]
        L += ["", "const %sManager %s_managers[] = {" % (C, p)]
        L += ['    { "%s", %s_calls_%s, &%s_count_%s },' % (m, p, ident(m), p, ident(m)) for m in mgrs]
        L += ["};", "const uint16_t %s_manager_count = %d;" % (p, len(mgrs)), "",
              "const %sCall *%s_find_call(const char *name, const %sManager **mgr)" % (C, p, C),
              "{",
              "    for (unsigned m = 0; m < %s_manager_count; m++) {" % p,
              "        const %sManager *pm = &%s_managers[m];" % (C, p),
              "        for (unsigned i = 0; i < *pm->count; i++)",
              "            if (strcasecmp(pm->calls[i].name, name) == 0) {",
              "                if (mgr) *mgr = pm;",
              "                return &pm->calls[i];",
              "            }",
              "    }",
              "    return 0;",
              "}", ""]
        return "\n".join(L)

    # ------------------------------------------------------------ .inc headers
    EQU = re.compile(r"^\s*\.(equ|set)\s+([A-Za-z_.$][\w.$]*)\s*,\s*([^|]+?)\s*(?:\|\s*(.*))?$")

    def parse_inc(self, fn):
        out = []
        path = os.path.join(self.a.macrom, "include", fn)
        for ln in open(path, encoding="utf-8", errors="replace"):
            m = self.EQU.match(ln)
            if m:
                out.append((m.group(2), m.group(3).strip(), (m.group(4) or "").strip(), m.group(1)))
        return out

    @staticmethod
    def evaluate(expr, env):
        e = expr.replace("$", "0x")
        e = re.sub(r"\b0x([0-9A-Fa-f]+)\b", lambda m: str(int(m.group(1), 16)), e)
        for n in set(re.findall(r"[A-Za-z_][\w.]*", e)):
            if n not in env:
                return None
            e = re.sub(r"(?<![\w.])%s(?![\w.])" % re.escape(n), str(env[n]), e)
        if not re.fullmatch(r"[\d\s+\-*/()<>|&~^%]+", e):
            return None
        try:
            return int(eval(e.replace("/", "//")))
        except Exception:
            return None

    def resolve(self, entries, env=None, override=False):
        env, vals = dict(env or {}), {}
        for _ in range(4):
            for name, expr, _c, kind in entries:
                if name in vals:
                    continue
                v = self.evaluate(expr, env)
                if v is not None:
                    vals[name] = env[name] = v
        return vals, env

    def lowmem_h(self):
        base = self.parse_inc("lowmem.inc")
        entries = list(base)
        se_note = {}
        if self.m["lowmem_extra"]:
            extra = self.parse_inc(self.m["lowmem_extra"])
            names = {e[0] for e in extra}
            entries = [e for e in base if e[0] not in names] + extra   # SE values override
            se_note = {e[0]: ("moved on the Mac SE" if e[3] == "set" else "Mac SE") for e in extra}
        vals, _ = self.resolve(entries)
        comments = {e[0]: re.sub(r"\s*\d+/\d+\s+\d+%.*$", "", e[2]) for e in entries}
        ytype = {lm["name"]: lm.get("type") for lm in self.lowmems}
        out = ["/* lowmem.h - %s low-memory globals. GENERATED by tools/sdkgen from" % self.m["name"],
               " * mac_rom/include/lowmem.inc%s (the ROM source's own list)." %
               (" + " + self.m["lowmem_extra"] if self.m["lowmem_extra"] else ""),
               " *   LM_ADDR_<name>  address on the %s" % self.m["name"],
               " *   LM_<name>       the global as an lvalue: typed when the multiversal",
               " *                   definitions know the type, else long (even) / byte (odd) */",
               "#ifndef %s_LOWMEM_H" % self.P, "#define %s_LOWMEM_H" % self.P, '#include "allmanagers.h"', "",
               "#define LM_BYTE(a)  (*(volatile uint8_t  *)(uintptr_t)(a))",
               "#define LM_WORD(a)  (*(volatile uint16_t *)(uintptr_t)(a))",
               "#define LM_LONG(a)  (*(volatile uint32_t *)(uintptr_t)(a))",
               "#define LM_PTR(a)   (*(void * volatile *)(uintptr_t)(a))", ""]
        n = 0
        for name in sorted(vals, key=lambda k: (vals[k], k)):
            a = vals[name]
            if a >= 0x10000:
                continue
            cid, t = ident(name), ytype.get(name)
            note = " ".join(x for x in (comments.get(name, ""), "(%s)" % se_note[name] if name in se_note else "") if x)
            out.append("#define LM_ADDR_%-24s 0x%04X%s" % (cid, a, ("  /* %s */" % c_comment(note)) if note else ""))
            if t and re.fullmatch(r"[A-Za-z_]\w*\s*\**", t):
                out.append("#define LM_%-29s (*(%s volatile *)0x%04X)" % (cid, t, a))
            else:
                out.append("#define LM_%-29s %s(0x%04X)" % (cid, "LM_BYTE" if a & 1 else "LM_LONG", a))
            n += 1
        return "\n".join(out + ["", "#endif", ""]), n

    def hw_h(self):
        entries = self.parse_inc("hwregs.inc")
        vals, _ = self.resolve(entries)
        comments = {e[0]: e[2] for e in entries}
        path = os.path.join(self.a.macrom, "include", "hwregs.inc")
        doc = [" *" + ln.rstrip("\n")[1:] for ln in open(path, encoding="utf-8", errors="replace")
               if ln.startswith("|")]
        extra = (" * The SE keeps the Plus addresses for VIA, SCC, IWM and the SCSI 5380; its",
                 " * keyboard and mouse are on ADB (use the ADB Manager).") if self.a.machine == "se" else ()
        out = ["/* hw.h - %s hardware registers. GENERATED by tools/sdkgen from" % self.m["name"],
               " * mac_rom/include/hwregs.inc.   HW_ADDR_<name> = address, HW_<name> = register",
               " * (volatile byte). VIA/SCC/IWM respond on byte lanes: use byte access only."] + list(extra) + [
               " *", " * Notes from the ROM source:"] + [c_comment(d) for d in doc] + [" */",
               "#ifndef %s_HW_H" % self.P, "#define %s_HW_H" % self.P, "#include <stdint.h>", "",
               "#define HW_REG8(a)  (*(volatile uint8_t *)(uintptr_t)(a))", ""]
        for name, _e, _c, _k in entries:
            if name in vals:
                v, cid, cm = vals[name], ident(name), comments.get(name, "")
                out.append("#define HW_ADDR_%-20s 0x%06X%s" % (cid, v, ("  /* %s */" % c_comment(cm)) if cm else ""))
                if v >= 0x400000:
                    out.append("#define HW_%-25s HW_REG8(0x%06X)" % (cid, v))
        return "\n".join(out + ["", "#endif", ""]), len(vals)

    def equates_h(self):
        files = [("SYS", "sysequ.inc"), ("TOOL", "toolequ.inc"), ("FS", "fsequ.inc"), ("SONY", "sonyequ.inc"),
                 ("PRIV", "private.inc"), self.m["rsrc"], ("RSRC", "romrsrc.inc")]
        out = ["/* equates.h - every equate from the %s ROM source. GENERATED by tools/sdkgen" % self.m["name"],
               " * SYS_ SysEqu (parameter blocks, queues), TOOL_ ToolEqu (window/control/menu/",
               " * dialog/TextEdit offsets), FS_ file system, SONY_ floppy driver, PRIV_ ROM",
               " * private, RSRC_ ROM resources. */",
               "#ifndef %s_EQUATES_H" % self.P, "#define %s_EQUATES_H" % self.P, ""]
        env, _ = self.resolve(self.parse_inc("lowmem.inc"))
        env = dict(env)
        total = 0
        for prefix, fn in files:
            if not os.path.exists(os.path.join(self.a.macrom, "include", fn)):
                continue
            entries = self.parse_inc(fn)
            vals, env = self.resolve(entries, env)
            out.append("/* ---- %s ---- */" % fn)
            seen = set()
            for name, _e, cm, _k in entries:
                if name in vals and name not in seen:
                    seen.add(name)
                    v = vals[name]
                    out.append("#define %s_%-30s %-12s%s" % (prefix, ident(name), ("0x%X" % v) if v >= 0 else str(v),
                                                            ("/* %s */" % c_comment(cm)) if cm else ""))
                    total += 1
            out.append("")
        return "\n".join(out + ["#endif", ""]), total

    # ------------------------------------------------------------ traps.h
    def traps_h(self):
        p, P, rj, rom = self.p, self.P, self.rj, self.rom
        calls_by_trap = {}
        for name, (ok, t, _s, _w, _m) in self.results.items():
            if t is not None and ok:
                calls_by_trap.setdefault(t, []).append(name)
        bm = [0] * 96
        for t in rom:
            i = (t - 0xA800) if t >= 0xA800 else 512 + (t - 0xA000)
            bm[i >> 3] |= 0x80 >> (i & 7)
        out = ["/* traps.h - the A-line traps of the %s ROM. GENERATED by tools/sdkgen" % self.m["name"],
               " * Decoded from the ROM's own dispatch table: ROM %s version %s," % (rj["rom_checksum"], rj["rom_version"]),
               " * %d Toolbox + %d OS traps." % (rj["toolbox_count"], rj["os_count"]),
               " *   %s_TRAP_<name>        trap word (named after the API call when there is one)" % P,
               " *   %s_ROMOFF_<name>      handler offset in this ROM (moves between ROM versions:" % P,
               " *                           always call through the trap)",
               " *   %s_trap_in_rom(t)     1 if the %s ROM implements trap t" % (p, self.m["name"]),
               " *   %s_machine()          which machine we run on (run time)" % p,
               " *   %s_trap_address(t)    current handler, from the dispatch table" % p,
               " *   %s_CALL_TRAP(word)    emit a raw trap instruction */" % P,
               "#ifndef %s_TRAPS_H" % P, "#define %s_TRAPS_H" % P, "#include <stdint.h>", "",
               "#define %s_TOOLBOX_TRAPS %d" % (P, rj["toolbox_count"]),
               "#define %s_OS_TRAPS      %d" % (P, rj["os_count"]),
               '#define %s_CALL_TRAP(word) __asm__ volatile(".short " #word ::: "d0","d1","d2","a0","a1","cc","memory")' % P,
               ""]
        used = set()
        for t in sorted(rom):
            labels = rom[t]["rom_labels"] or ["Trap_%04X" % t]
            calls = sorted(calls_by_trap.get(t, []))
            name = labels[0]
            for c in calls:                       # prefer the API spelling (FrameRect, not FRAMERECT)
                if c.lower() == labels[0].lower():
                    name = c
                    break
            primary = ident(name)
            if primary in used:
                primary = "%s_%04X" % (primary, t)
            used.add(primary)
            cm = "ROM: %s" % ", ".join(labels) + (("; calls: " + ", ".join(calls[:6]) + (" ..." if len(calls) > 6 else ""))
                                                 if calls else "")
            out.append("#define %s_TRAP_%-26s 0x%04X  /* %s */" % (P, primary, t, c_comment(cm)))
            out.append("#define %s_ROMOFF_%-24s 0x%05X" % (P, primary, rom[t]["rom_offset"]))
        out += ["", "static const uint8_t %s_trap_bitmap[96] = {" % p]
        out += ["    " + ", ".join("0x%02X" % b for b in bm[i:i + 12]) + "," for i in range(0, 96, 12)]
        out += ["};", "",
                "/* 1 if the %s ROM implements this trap (flag bits are ignored). */" % self.m["name"],
                "static inline int %s_trap_in_rom(uint16_t trap)" % p,
                "{",
                "    unsigned i;",
                "    if ((trap & 0xF000) != 0xA000) return 0;",
                "    if (trap & 0x0800) { i = trap & 0x3FF; if (i >= 512) return 0; }",
                "    else i = 512 + (trap & 0xFF);",
                "    return (%s_trap_bitmap[i >> 3] >> (7 - (i & 7))) & 1;" % p,
                "}", "",
                "/* Which machine are we on? ROM version word at ROMBase+8: $0075 Plus, $0276 SE. */",
                "#define %s_MACHINE_PLUS 1" % P,
                "#define %s_MACHINE_SE   2" % P,
                "static inline int %s_machine(void)" % p,
                "{",
                "    const uint8_t *rom = *(const uint8_t * const *)0x2AE;      /* ROMBase */",
                "    return rom[9] == 0x76 ? %s_MACHINE_SE : %s_MACHINE_PLUS;" % (P, P),
                "}",
                "static inline const char *%s_machine_name(void)" % p,
                "{",
                "    return %s_machine() == %s_MACHINE_SE ? \"Macintosh SE\" : \"Macintosh Plus\";" % (p, P),
                "}", "",
                "/* The Toolbox trap table is at $0C00 on the Plus and $0E00 on the SE (where",
                " * $0C00 holds the SCSI globals). OS traps are at $0400 on both. */",
                "static inline void **%s_tool_table(void)" % p,
                "{",
                "    return (void **)(uintptr_t)(%s_machine() == %s_MACHINE_SE ? 0x0E00 : 0x0C00);" % (p, P),
                "}",
                "static inline void **%s_trap_slot(uint16_t trap)" % p,
                "{",
                "    if (trap & 0x0800) return %s_tool_table() + (trap & 0x1FF);" % p,
                "    return (void **)(uintptr_t)0x0400 + (trap & 0xFF);",
                "}",
                "static inline void *%s_trap_address(uint16_t trap) { return *%s_trap_slot(trap); }" % (p, p),
                "", "#endif", ""]
        return "\n".join(out)

    def umbrella(self, mgrs_in_rom):
        P, p = self.P, self.p
        return "\n".join([
            "/* %s.h - the complete %s ROM SDK. GENERATED by tools/sdkgen" % (p, self.m["name"]),
            " *   <Manager>.h    one header per ROM manager (include just the ones you need,",
            " *                  e.g. #include <%s/QuickDraw.h>), or this umbrella for all" % p,
            " *   traps.h        trap numbers, ROM offsets, machine detection",
            " *   lowmem.h       low-memory globals",
            " *   hw.h           hardware registers",
            " *   equates.h      ROM equates (record offsets, constants)",
            " *   boot.h         boot-time OS calls (InitEvents, InitFS, InitZone, ...)",
            " *   catalog.h      run-time table of every call and whether this ROM has it",
            " * Managers with calls in ROM: %s */" % ", ".join(mgrs_in_rom),
            "#ifndef %s_%s_H" % (P, P), "#define %s_%s_H" % (P, P),
            '#include "allmanagers.h"', '#include "traps.h"', '#include "lowmem.h"', '#include "hw.h"',
            '#include "equates.h"', '#include "boot.h"', '#include "catalog.h"', "#endif", ""])

    def docs(self, counts):
        per = {}
        for name, r in self.results.items():
            per.setdefault(r[4], []).append((name,) + r)
        n_ok = sum(1 for r in self.results.values() if r[0])
        L = ["# %s ROM reference (PlusOS SDK)" % self.m["name"], "",
             "Generated by `tools/sdkgen` from the %s ROM (checksum `%s`, version `%s`), the Mac ROM source tree "
             "and Retro68's multiversal interface definitions." % (self.m["name"], self.rj["rom_checksum"],
                                                                   self.rj["rom_version"]), "",
             "| | |", "|---|---|",
             "| Traps implemented by the ROM | %d Toolbox + %d OS = **%d** |" % (
                 self.rj["toolbox_count"], self.rj["os_count"], len(self.rom)),
             "| Calls available | **%d** |" % n_ok,
             "| Calls not in this ROM (compile error if used) | %d |" % (len(self.results) - n_ok),
             "| Manager headers / C files | %d / %d |" % (counts["headers"], counts["cfiles"]),
             "| Low-memory globals (`lowmem.h`) | %d |" % counts["lowmem"],
             "| Hardware registers (`hw.h`) | %d |" % counts["hw"],
             "| ROM equates (`equates.h`) | %d |" % counts["equ"], "",
             "Packages in ROM: " + ", ".join("PACK %d %s" % (k, v) for k, v in ROM_PACKAGES.items()) +
             ". The List Manager, Standard File, Disk Init and International Utilities packages load from a "
             "System file and are unavailable under PlusOS.", "",
             "ROM offsets are for this ROM version; always call through the trap.", "", "## Calls by manager", ""]
        for mgr in sorted(per):
            items = sorted(per[mgr])
            L += ["### %s.h (%d of %d in ROM)" % (mgr, sum(1 for i in items if i[1]), len(items)), "",
                  "| Call | Trap | Selector | In ROM | ROM routine | Note |", "|---|---|---|---|---|---|"]
            for name, ok, t, sel, why, _m in items:
                lab = ""
                if t is not None and t in self.rom:
                    labs = self.rom[t]["rom_labels"]
                    lab = "`%s` @ 0x%05X" % (labs[0] if labs else "?", self.rom[t]["rom_offset"])
                L.append("| `%s` | %s | %s | %s | %s | %s |" % (
                    name, ("0x%04X" % t) if t is not None else "-", sel if sel is not None else "",
                    "yes" if ok else "**no**", lab, why))
            L.append("")
        L += ["## Every trap in the ROM", "", "| Trap | ROM routine(s) | Offset | Calls |", "|---|---|---|---|"]
        by_trap = {}
        for name, (ok, t, _s, _w, _m) in self.results.items():
            if t is not None and ok:
                by_trap.setdefault(t, []).append(name)
        for t in sorted(self.rom):
            e = self.rom[t]
            L.append("| 0x%04X | %s | 0x%05X | %s |" % (t, ", ".join("`%s`" % x for x in e["rom_labels"]),
                                                        e["rom_offset"], ", ".join(sorted(by_trap.get(t, [])))))
        return "\n".join(L) + "\n"

    # ------------------------------------------------------------ run
    def run(self):
        a = self.a
        for d in (a.inc, a.src, a.docs):
            os.makedirs(d, exist_ok=True)
        for pth in glob.glob(os.path.join(a.inc, "*.h")):
            if os.path.basename(pth) != "boot.h":
                os.remove(pth)
        for pth in glob.glob(os.path.join(a.src, "*.c")):
            os.remove(pth)
        self.load_defs()
        mgrs = [m for m in open(os.path.join(a.split_inc, "managers.txt")).read().split() if m]
        write(os.path.join(a.inc, "MacPrelude.h"), self.prelude())
        for m in mgrs:
            text = open(os.path.join(a.split_inc, m + ".h")).read()
            write(os.path.join(a.inc, m + ".h"), self.annotate_header(text, m))
        write(os.path.join(a.inc, "allmanagers.h"),
              "/* allmanagers.h - every ROM manager header, in dependency order. GENERATED */\n"
              "#ifndef %s_ALLMANAGERS_H\n#define %s_ALLMANAGERS_H\n" % (self.P, self.P) +
              "".join('#include "%s.h"\n' % m for m in mgrs) + "#endif\n")
        per = {}
        for name, r in self.results.items():
            per.setdefault(r[4], []).append((name, r))
        cfiles, cat_mgrs, glue_kept = 0, [], []
        for m in mgrs:
            if m in per:
                write(os.path.join(a.src, m + ".c"), self.catalog_c(m, per[m]))
                cat_mgrs.append(m)
                cfiles += 1
            gpath = os.path.join(a.split_src, m + ".glue.c")
            if os.path.exists(gpath):
                gtext = open(gpath).read().replace("<plusrom/", "<%s/" % self.p)
                defined = GLUE_DEF.findall(gtext)
                if defined and all(self.results.get(n, (False,))[0] for n in defined):
                    write(os.path.join(a.src, m + ".glue.c"), gtext)
                    glue_kept.append(m)
                    cfiles += 1
        write(os.path.join(a.inc, "catalog.h"), self.catalog_h())
        write(os.path.join(a.src, "catalog.c"), self.catalog_index(cat_mgrs))
        write(os.path.join(a.inc, "traps.h"), self.traps_h())
        lm, n_lm = self.lowmem_h()
        write(os.path.join(a.inc, "lowmem.h"), lm)
        hw, n_hw = self.hw_h()
        write(os.path.join(a.inc, "hw.h"), hw)
        eq, n_eq = self.equates_h()
        write(os.path.join(a.inc, "equates.h"), eq)
        in_rom = [m for m in cat_mgrs if any(r[0] for _n, r in per[m])]
        write(os.path.join(a.inc, self.p + ".h"), self.umbrella(in_rom))
        counts = {"lowmem": n_lm, "hw": n_hw, "equ": n_eq, "headers": len(mgrs), "cfiles": cfiles + 1}
        write(os.path.join(a.docs, "ROM_REFERENCE.md"), self.docs(counts))
        n_ok = sum(1 for r in self.results.values() if r[0])
        stats = dict(headers=len(mgrs), cfiles=cfiles + 1, calls_ok=n_ok, calls_no=len(self.results) - n_ok,
                     traps=len(self.rom), toolbox=self.rj["toolbox_count"], os=self.rj["os_count"],
                     lowmem=n_lm, hw=n_hw, equ=n_eq, glue=glue_kept)
        if a.stats:
            json.dump(stats, open(a.stats, "w"), indent=1)
        print("gen_sdk %-5s: %d headers, %d C files, %d traps, %d calls in ROM, %d not; "
              "%d low-memory globals, %d registers, %d equates"
              % (a.machine, len(mgrs), cfiles + 1, len(self.rom), n_ok, len(self.results) - n_ok, n_lm, n_hw, n_eq))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--machine", required=True, choices=sorted(MACHINES))
    for x in ("--traps", "--split-inc", "--split-src", "--defs", "--macrom", "--inc", "--src", "--docs"):
        ap.add_argument(x, required=True)
    ap.add_argument("--stats")
    Gen(ap.parse_args()).run()


if __name__ == "__main__":
    main()
