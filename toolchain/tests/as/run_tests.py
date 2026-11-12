#!/usr/bin/env python3
"""
P68 assembler tests.  Run inside WSL/Linux from anywhere:

    python3 toolchain/tests/as/run_tests.py [--cpu 68000,68020,...] [--quick] [--only corpus|repo|errors|syntax|loader]
                                             [--jobs N] [--keep] [--verbose]

GNU as (m68k-linux-gnu-as) is used ONLY as an oracle:
  corpus  every mnemonic x operand combination is assembled by both assemblers; the set of
          accepted lines must agree and the bytes + relocations of every line must be identical
  repo    the repository's .S files (C-preprocessed like the Makefile does) must be identical
  syntax  hand written files for directives, macros, conditionals, expressions, Motorola mode
  errors  inputs that must be rejected
  loader  boot/loader.S -> flat binary compared with build/boot/loader.bin
"""
import argparse, os, re, struct, subprocess, sys, shutil, itertools
from concurrent.futures import ProcessPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLCHAIN = os.path.dirname(os.path.dirname(HERE))
REPO = os.path.dirname(TOOLCHAIN)
P68AS = os.environ.get("P68AS", os.path.join(HERE, "bin", "p68-as"))
GAS = os.environ.get("GAS", "m68k-linux-gnu-as")
GCC = os.environ.get("GCC_CPP", "m68k-linux-gnu-gcc")
TMP = os.environ.get("P68_TMP", "/tmp/p68-as-tests")

CPUS = {
    "68000": ["-m68000"],
    "68010": ["-m68010"],
    "68020": ["-m68020", "-m68881"],   # GNU as additionally gets -mno-68851 (see GAS_EXTRA)
    "68030": ["-m68030", "-m68881"],
    "68040": ["-m68040"],
    "68060": ["-m68060"],
    "cpu32": ["-mcpu32"],
}

GAS_EXTRA = {"68020": ["-mno-68851"]}

# ----------------------------------------------------------------------------- ELF reader
class Obj:
    def __init__(self, path):
        d = open(path, "rb").read()
        self.data = d
        assert d[:4] == b"\x7fELF" and d[4] == 1 and d[5] == 2, path
        shoff, = struct.unpack_from(">I", d, 0x20)
        shentsize, shnum, shstrndx = struct.unpack_from(">HHH", d, 0x2E)
        secs = []
        for i in range(shnum):
            f = struct.unpack_from(">IIIIIIIIII", d, shoff + i * shentsize)
            secs.append(dict(name_off=f[0], type=f[1], flags=f[2], addr=f[3], offset=f[4], size=f[5], link=f[6], info=f[7], align=f[8], entsize=f[9]))
        st = secs[shstrndx]
        for s in secs:
            o = st["offset"] + s["name_off"]
            s["name"] = d[o:d.index(b"\0", o)].decode()
        self.secs = secs
        self.syms = []
        for s in secs:
            if s["type"] != 2:
                continue
            strs = secs[s["link"]]
            for i in range(s["size"] // 16):
                no, value, size, info, other, shndx = struct.unpack_from(">IIIBBH", d, s["offset"] + i * 16)
                o = strs["offset"] + no
                name = d[o:d.index(b"\0", o)].decode("latin-1")
                self.syms.append(dict(name=name, value=value, size=size, bind=info >> 4, type=info & 15, shndx=shndx))

    def section(self, name):
        for i, s in enumerate(self.secs):
            if s["name"] == name:
                return i, s
        return None, None

    def bytes_of(self, name):
        i, s = self.section(name)
        if s is None or s["type"] == 8:
            return b""
        return self.data[s["offset"]:s["offset"] + s["size"]]

    def relocs_of(self, name):
        """normalised relocations of section `name`: (offset, type, target, addend)"""
        idx, _ = self.section(name)
        out = []
        if idx is None:
            return out
        for s in self.secs:
            if s["type"] != 4 or s["info"] != idx:
                continue
            for i in range(s["size"] // 12):
                off, info, add = struct.unpack_from(">IIi", self.data, s["offset"] + i * 12)
                sym = self.syms[info >> 8]
                if sym["type"] == 3:
                    tgt = "sec:" + self.secs[sym["shndx"]]["name"]
                elif (info >> 8) == 0:
                    tgt = "*ABS*"
                else:
                    tgt = sym["name"]
                out.append((off, info & 0xFF, tgt, add))
        out.sort()
        return out

    def alloc_sections(self):
        return [s["name"] for s in self.secs if s["flags"] & 2 and s["type"] in (1, 8, 14, 15, 16)]


def run(cmd, **kw):
    return subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True, **kw)


def err_lines(stderr, fname):
    out = set()
    for l in stderr.splitlines():
        m = re.match(r"^(.*?):(\d+): (Error|error|Fatal error)", l)
        if m:
            out.add(int(m.group(2)))
    return out


def compare_objects(g, p, what):
    """whole-object comparison: section contents and relocations"""
    problems = []
    names = sorted(set(g.alloc_sections()) | set(p.alloc_sections()))
    for n in names:
        gi, gs = g.section(n)
        pi, ps = p.section(n)
        gsize = gs["size"] if gs else 0
        psize = ps["size"] if ps else 0
        if gsize != psize:
            problems.append("%s: section %s size %d (gas) vs %d (p68)" % (what, n, gsize, psize))
            continue
        gb, pb = g.bytes_of(n), p.bytes_of(n)
        if gb != pb:
            i = next(i for i in range(min(len(gb), len(pb))) if gb[i] != pb[i])
            problems.append("%s: section %s differs at 0x%x: gas %s p68 %s" % (what, n, i, gb[i:i + 8].hex(), pb[i:i + 8].hex()))
        gr, pr = g.relocs_of(n), p.relocs_of(n)
        if gr != pr:
            d = [x for x in gr if x not in pr][:3], [x for x in pr if x not in gr][:3]
            problems.append("%s: relocations of %s differ: only gas %s / only p68 %s" % (what, n, d[0], d[1]))
    return problems

# ----------------------------------------------------------------------------- corpus generation
CC = ["hi", "ls", "cc", "cs", "ne", "eq", "vc", "vs", "pl", "mi", "ge", "lt", "gt", "le"]
FCC = ["f", "eq", "ogt", "oge", "olt", "ole", "ogl", "or", "un", "ueq", "ugt", "uge", "ult", "ule", "ne", "t",
       "sf", "seq", "gt", "ge", "lt", "le", "gl", "gle", "ngle", "ngl", "nle", "nlt", "nge", "ngt", "sne", "st"]

INT_BASE = ("abcd add adda addi addq addx and andi asl asr bchg bclr bset btst bftst bfextu bfchg bfexts bfclr bfffo bfset bfins "
            "bkpt bgnd callm cas cas2 chk chk2 clr cmp cmpa cmpi cmpm cmp2 divs divu divsl divul eor eori exg ext extb illegal "
            "jmp jsr lea link lsl lsr move movea movec movem movep moveq moves move16 muls mulu nbcd neg negx nop not or ori "
            "pack pea reset rol ror roxl roxr rtd rte rtm rtr rts sbcd stop sub suba subi subq subx swap tas trap trapv tst unlk unpk "
            "cinvl cinvp cinva cpushl cpushp cpusha pflush pflushn pflusha pflushan ptestr ptestw ploadr ploadw pmove pmovefd plpar plpaw "
            "lpstop tblu tblun tbls tblsn bra bsr jra jbsr jbra dbra bhs blo dbhs dblo shs slo dbt dbf st sf trapt trapf").split()
for c in CC:
    INT_BASE += ["b" + c, "j" + c, "db" + c, "s" + c, "trap" + c]
INT_SUFFIX = ["", ".b", ".w", ".l", ".s"]

FP_MON = ("fmove fint fsinh fintrz fsqrt flognp1 fetoxm1 ftanh fatan fasin fatanh fsin ftan fetox ftwotox ftentox flogn flog10 flog2 "
          "fabs fcosh fneg facos fcos fgetexp fgetman fdiv fmod fadd fmul fsgldiv frem fscale fsglmul fsub fcmp ftst "
          "fsmove fdmove fssqrt fdsqrt fsabs fdabs fsneg fdneg fsadd fdadd fssub fdsub fsmul fdmul fsdiv fddiv").split()
FP_SUFFIX = ["", ".b", ".w", ".l", ".s", ".d", ".x", ".p"]

OPS_BASE = [
    "%d1", "%a2", "(%a3)", "(%a4)+", "-(%a5)", "4(%a6)", "(-8,%a0)", "(0,%a1)", "6(%a1,%d2.w)", "(-2,%a3,%a4.l)", "(%a0,%d0)",
    "0x1234.w", "0x12345678", "0x7000", "0x9000", "lab", "ext", "(ext).w", "ext+4", "ext.l", "1b", "2f",
    "1b(%pc)", "ext(%pc)", "8(%pc)", "lab(%pc)", "1b(%pc,%d3.w)", "ext(%pc,%a1.l)", "(4,%pc,%d0.w)",
    "#1", "#8", "#0", "#-1", "#100", "#200", "#0x7fff", "#0xffff", "#0x12345678", "#ext", "#ext+2",
    "%sr", "%ccr", "%usp", "%d0-%d3/%a0-%a2", "%d3/%a6", "%sp", "%pc",
]
OPS_020 = [
    "(8,%a0,%d1.l*4)", "(0x12345,%a1)", "(0x1234,%a0,%d0.w*2)", "([8,%a0],%d1.w*2,16)", "([%a0,%d0.l],ext)", "([lab,%pc],%d0,4)",
    "([ext])", "(ext,%a0)", "(%d0.w*8)", "(1b,%pc,%d0.l*2)", "([0x12345,%a2,%d3.w*4],0x54321)", "(4,%zpc)", "(%za0,%d0.w)",
    "(ext,%a0,%d0.l*4)", "([4,%a0])", "(0x8000,%pc)", "(ext.w,%a0)", "(4.l,%a0)", "([ext.w,%pc],8.l)", "(%a0,%d0.w*2)",
]
OPS_CTRL = ["%sfc", "%dfc", "%vbr", "%cacr", "%caar", "%msp", "%isp", "%tc", "%itt0", "%itt1", "%dtt0", "%dtt1", "%mmusr", "%urp",
            "%srp", "%buscr", "%pcr", "%usp", "%crp", "%tt0", "%tt1", "%psr"]
OPS_FP = ["%fp0", "%fp3", "%d1", "%a2", "(%a3)", "(%a4)+", "-(%a5)", "4(%a6)", "6(%a1,%d2.w)", "0x1234.w", "ext", "1b", "lab(%pc)",
          "1b(%pc,%d3.w)", "#1", "#100", "#0x12345678", "#0e1.5", "#0e-2.25e10", "%fpcr", "%fpsr", "%fpiar", "%fp0-%fp3/%fp7", "%fp2/%fp5",
          "%fpcr/%fpsr", "%fpcr/%fpsr/%fpiar", "%d0-%d3"]

SPECIAL = """
bfextu %d0{#1:#2},%d1
bfextu (%a0){%d2:%d3},%d1
bfextu 4(%a0){#31:#32},%d7
bfexts lab(%pc){#3:%d1},%d2
bfffo (8,%a1,%d0.w){%d1:#5},%d3
bfins %d1,(%a0){%d2:%d3}
bfins %d1,%d2{#0:#8}
bfins %d1,lab(%pc){#0:#8}
bftst (%a0){#1:#2}
bftst lab(%pc){#1:#2}
bfchg %d0{#1:#2}
bfclr -(%a0){#1:#2}
bfset (%a0)+{#1:#2}
bfset ext{#1:#2}
bfchg lab{1:2}
mulu.l %d0,%d1
mulu.l %d0,%d1:%d2
muls.l (%a0),%d3:%d4
muls.l #1234,%d1
mulu.l lab,%d1
divu.l %d0,%d1
divu.l %d0,%d1:%d2
divs.l (%a0),%d3:%d4
divsl.l %d0,%d1:%d2
divul.l 4(%a0),%d1:%d2
divsl %d0,%d1:%d2
divul %d0,%d1
divs.l %d0,%d1
muls %d0,%d1:%d2
mulu.w #4,%d2
divs.w lab(%pc),%d2
cas.b %d0,%d1,(%a0)
cas.w %d0,%d1,4(%a0)
cas.l %d0,%d1,ext
cas %d0,%d1,(%a0)
cas2.w %d0:%d1,%d2:%d3,(%a0):(%a1)
cas2.l %d0:%d1,%d2:%d3,(%d4):(%a1)
cas2 %d0:%d1,%d2:%d3,(%a0):(%a1)
chk2.b (%a0),%d1
chk2.w 4(%a0),%a1
chk2.l lab(%pc),%d1
cmp2.b (%a0),%d1
cmp2.w ext,%a1
cmp2.l (%a0,%d0.w),%d1
chk.l %d0,%d1
chk.w #4,%d1
pack %d0,%d1,#4
pack -(%a0),-(%a1),#0x1234
unpk %d0,%d1,#4
unpk -(%a0),-(%a1),#ext
link %a6,#-8
link.w %a6,#-8
link.l %a6,#-8
link %a6,#-70000
link %a6,#ext
unlk %a6
movem.l %d0-%d7/%a0-%a6,-(%sp)
movem.l (%sp)+,%d0-%d7/%a0-%a6
movem.w %d0/%d2/%a1,(%a0)
movem.w 4(%a0),%d0/%d2/%a1
movem.l %d0,-(%sp)
movem.l (%sp)+,%a0
movem.l #0x1234,-(%sp)
movem.l #0x1234,(%a0)
movem.l (%sp)+,#0x1234
movem.l lab(%pc),%d0-%d1
movem.l %d0-%d1,ext
movem %d0-%d1,-(%sp)
movem.l %d0-%d1,(%a0)+
movem.l -(%a0),%d0-%d1
movem.l %a7/%d0,lab
movep.w 4(%a0),%d1
movep.l %d1,4(%a0)
movep.w (%a0),%d1
movep %d1,(%a0)
movep.l %d1,ext(%a0)
moves.b (%a0),%d1
moves.w %a1,(%a0)+
moves.l %d1,4(%a0)
moves.l ext,%d1
moveq #127,%d0
moveq #-128,%d0
moveq #128,%d0
moveq #255,%d0
moveq #ext,%d0
moveq.l #1,%d0
move.l #127,%d1
move.l #128,%d1
move.l #-128,%d1
move.l #-129,%d1
move.l #0,%a1
move.w #0,%d1
move.b #255,%d1
move.b #-1,%d1
move.b #256,%d1
move.b #-129,%d1
move.w #65535,%d1
move.w #-32768,%d1
move.w #65536,%d1
move #1,%d1
move %sr,%d0
move %d0,%sr
move %ccr,%d0
move %d0,%ccr
move.l %a0,%usp
move.l %usp,%a0
move %usp,%a1
move %a1,%usp
and #0xf,%ccr
andi #0xf,%ccr
andi.b #0xf,%ccr
and.b #0xf,%ccr
andi #0xf00,%sr
andi.w #0xf00,%sr
and.w #0xf00,%sr
or #1,%ccr
ori #1,%ccr
ori.b #1,%ccr
ori #0x700,%sr
ori.w #0x700,%sr
eor #1,%ccr
eori #1,%ccr
eori.b #1,%ccr
eori #0x700,%sr
eori.w #0x700,%sr
add.b #1,%d0
add.w #9,%d0
add.l #8,%a0
add.w #0,%d0
sub.l #1,(%a0)
addq.l #ext,%d0
asl.w #1,%d0
asl.w #8,%d0
asl.w #9,%d0
asl.w #0,%d0
asl.l %d1,%d0
asr.b #3,%d0
lsl (%a0)
lsr.w 4(%a0)
rol %d0
roxr.w #1,%d1
asl %d0
asl #1,%d0
asl %d1,%d0
btst #7,%d0
btst #31,%d0
btst #32,%d0
btst #7,(%a0)
btst #8,(%a0)
btst %d1,#5
btst #1,lab(%pc)
btst #1,lab
bset #1,lab
btst %d1,lab
btst.b #1,(%a0)
btst.l #1,%d0
bchg %d0,%d1
trap #0
trap #15
trap #16
bkpt #0
bkpt #7
bkpt #8
rtd #8
rtd #-4
stop #0x2700
lpstop #0x2000
trapcc.w #4
trapcc.l #4
trapcc #4
trapne.w #ext
trapeq
exg %d0,%d1
exg %a0,%a1
exg %d0,%a1
exg %a0,%d1
ext.w %d0
ext.l %d0
extb.l %d0
extb %d0
ext %d0
swap %d0
swap.w %d0
lea 4(%a0),%a1
lea.l 4(%a0),%a1
lea lab,%a0
lea ext,%a0
lea lab(%pc),%a0
lea ext.w,%a0
lea 0x1000,%a0
lea 0x10000,%a0
lea (0x1000).l,%a0
lea 0x1000.l,%a0
pea lab
pea ext
pea 4.w
pea 100000
jmp lab
jsr lab
jmp ext
jsr (%a0)
jsr 4(%pc)
jmp (%pc)
jmp 0(%pc)
jmp 2(%pc,%d0.w)
jsr 0x4000
jsr 0xf80000
tst.b lab
tst.w ext
tst.l lab(%pc)
tst.w #4
tst.l %a0
cmp.b #4,lab
cmp.w #4,lab(%pc)
cmpi.l #4,ext
cmp.l lab,%d0
cmp.l #ext,%a0
cmp.w %d0,(%a0)
cmpm.b (%a0)+,(%a1)+
cmp.b (%a0)+,(%a1)+
move.l lab,%d0
move.l ext,%d0
move.l lab,lab
move.l lab,ext
move.l #lab,-(%sp)
move.l #lab-ext,-(%sp)
move.w #lab-1b,%d0
move.l lab-1b(%a0),%d0
move.l lab+4,%d0
move.l lab-4(%pc),%d0
move.b 1b+1(%pc,%d0.w),%d0
move.w 0(%a0),%d0
move.w 0.w(%a0),%d0
move.w (0).w,%d0
move.w 0,%d0
move.w -1,%d0
move.w 32767,%d0
move.w 32768,%d0
move.w -32768,%d0
move.w -32769,%d0
move.w 0xffff8000,%d0
move.w (%a0,%d0.w),%d1
move.w (%a0,%d0.l),%d1
move.w (%a0,%a1),%d1
move.w (%d0,%a0),%d1
move.w 127(%a0,%d0.w),%d1
move.w -128(%a0,%d0.w),%d1
move.w 128(%a0,%d0.w),%d1
move.w ext(%a0,%d0.w),%d1
move.w ext(%a0),%d1
move.w 32767(%a0),%d1
move.w 32768(%a0),%d1
move.w -32768(%a0),%d1
move.w -32769(%a0),%d1
move.l %d0,(%a0,%d0.w*2)
move.l %d0,(4,%a0,%d0.w*1)
move.l ([%a0]),%d0
move.l ([4,%a0],8),%d0
move.l ([4,%a0,%d1.w],8),%d0
move.l ([4,%a0],%d1.w,8),%d0
move.l ([4,%a0],%d1.w),%d0
move.l ([%pc]),%d0
move.l ([lab,%pc]),%d0
move.l ([lab,%pc,%d1.w]),%d0
move.l ([lab,%zpc,%d1.w]),%d0
move.l ([lab]),%d0
move.l ([lab],%d0.w),%d0
move.l ([lab.w]),%d0
move.l ([0x8000]),%d0
move.l (lab,%d0.w),%d0
move.l (lab.w,%d0.w),%d0
move.l (0x8000,%a0,%d0.w),%d0
move.l (-0x8001,%a0,%d0.w),%d0
move.l (4.w,%a0,%d0.w),%d0
move.l (4.l,%a0,%d0.w),%d0
move.l (0x7fff,%a0),%d0
move.l (0x8000,%a0),%d0
move.l (ext,%a0),%d0
move.l (ext.w,%a0),%d0
move.l (ext.l,%a0),%d0
move.l (ext,%pc),%d0
move.l (ext.l,%pc),%d0
move.l (ext.w,%pc),%d0
move.l (lab.l,%pc),%d0
move.l (4,%pc),%d0
move.l (0x10000,%pc),%d0
move.l (ext,%pc,%d0.w),%d0
move.l (lab.w,%pc,%d0.w),%d0
move.l (lab.l,%pc,%d0.w),%d0
move.l (4,%pc,%d0.w*4),%d0
move.l (%pc,%d0.w),%d0
move.l (%pc),%d0
move.l (%zpc),%d0
move.l (%zpc,%d0.w),%d0
move.l (4,%za1),%d0
move.l (%za1),%d0
move.l (%a0,%zd1),%d0
move.l (4,%a0,%zd1.w),%d0
move16 (%a0)+,(%a1)+
move16 (%a0)+,ext
move16 ext,(%a0)+
move16 (%a0),0x1234
move16 0x12345678,(%a0)
movec %d0,%vbr
movec %vbr,%d0
movec %a0,%sfc
movec %dfc,%a1
movec %cacr,%d0
movec %d0,%cacr
movec %caar,%d0
movec %msp,%d0
movec %isp,%d0
movec %usp,%d0
movec %tc,%d0
movec %itt0,%d0
movec %itt1,%d0
movec %dtt0,%d0
movec %dtt1,%d0
movec %mmusr,%d0
movec %urp,%d0
movec %srp,%d0
movec %buscr,%d0
movec %pcr,%d0
movec %d0,%pcr
movec %iacr0,%d0
movec %dacr1,%d0
callm #4,(%a0)
callm #4,lab
rtm %d0
rtm %a1
cinvl %dc,(%a0)
cinvp %ic,(%a1)
cinva %bc
cinvl %nc,(%a0)
cpushl %dc,(%a0)
cpushp %bc,(%a1)
cpusha %ic
cpusha %dc
pflush (%a0)
pflushn (%a1)
pflusha
pflushan
ptestr (%a0)
ptestw (%a0)
plpar (%a0)
plpaw (%a1)
pmove (%a0),%tc
pmove %tc,(%a0)
pmove (%a0),%srp
pmove %crp,4(%a0)
pmove (%a0),%tt0
pmove %tt1,(%a0)
pmove (%a0),%mmusr
pmove %mmusr,(%a0)
pmove %psr,(%a0)
pmove (%a0),%psr
pmovefd (%a0),%tc
pmovefd (%a0),%tt0
pmovefd (%a0),%crp
pmove ext,%tc
pflusha
pflush #1,#4
pflush %d0,#4
pflush %sfc,#4
pflush %dfc,#4,(%a0)
pflush #7,#7,4(%a0)
ploadr #1,(%a0)
ploadw %d1,(%a0)
ploadr %sfc,4(%a0)
ploadw %dfc,ext
ptestr #1,(%a0),#7
ptestw %d0,(%a0),#3
ptestr %sfc,(%a0),#7,%a1
ptestw %dfc,4(%a0),#0,%a2
tblu.b (%a0),%d1
tblu.w 4(%a0),%d1
tblu.l lab(%pc),%d1
tblun.b (%a0),%d1
tbls.w (%a0),%d1
tblsn.l (%a0),%d1
tblu.b %d1:%d2,%d3
tblsn.w %d1:%d2,%d3
tbls.l %d1:%d2,%d3
tblun.l %d1:%d2,%d3
bgnd
dbra %d0,lab
dbra %d0,ext
dbra %d0,1b
dbf %d1,2f
dbne %d0,1b
jbsr lab
jbsr ext
jbsr (%a0)
jbsr 4(%a0)
jra (%a0)
jra lab
jra ext
jra 2f
jne 2f
jeq ext
jbra lab
bra.s 1b
bra.b 1b
bra.w 1b
bra.l 1b
bra 1b
bra 2f
bsr.s 2f
bsr.w ext
bsr.l ext
bsr ext
bne.s ext
beq.w 1b+2
bhs 1b
blo 1b
bra lab
jra 0x1000
jne 0x1000
jbsr 0x1000
bra 0x1000
"""

SPECIAL_FP = """
fmove.x %fp0,%fp1
fmove %fp0,%fp1
fmove.l %d0,%fp1
fmove.l %fp1,%d0
fmove.s (%a0),%fp1
fmove.s %fp1,(%a0)
fmove.d (%a0)+,%fp1
fmove.d %fp1,-(%a0)
fmove.x 4(%a0),%fp1
fmove.x %fp1,4(%a0)
fmove.p (%a0),%fp1
fmove.p %fp1,(%a0)
fmove.p %fp1,(%a0){#3}
fmove.p %fp1,(%a0){#-3}
fmove.p %fp1,(%a0){%d2}
fmove.b #1,%fp0
fmove.w #-1,%fp0
fmove.l #100000,%fp0
fmove.s #0e1.5,%fp0
fmove.d #0e1.5,%fp0
fmove.s #0e-0.1,%fp0
fmove.d #0e0.1,%fp0
fmove.d #0e1e300,%fp0
fmove.s #0e1e-40,%fp0
fmove.d #0e3.14159265358979,%fp0
fmove.s #0x3fc00000,%fp0
fmove.d #1,%fp0
fmove.s #1,%fp0
fmove.l %fpcr,%d0
fmove.l %d0,%fpcr
fmove.l %fpsr,(%a0)
fmove.l (%a0),%fpsr
fmove.l %fpiar,%a0
fmove.l %a0,%fpiar
fmove.l %fpcr,%a0
fmove.l #4,%fpcr
fmove %fpcr,%d0
fmove %d0,%fpsr
fmovem.l %fpcr/%fpsr,(%a0)
fmovem.l (%a0),%fpcr/%fpsr/%fpiar
fmovem.l %fpcr/%fpsr/%fpiar,-(%sp)
fmovem.l (%sp)+,%fpcr/%fpiar
fmovem.l %fpcr,%d0
fmovem %fpcr/%fpsr,(%a0)
fmovem.x %fp0-%fp7,-(%sp)
fmovem.x (%sp)+,%fp0-%fp7
fmovem.x %fp0/%fp2,(%a0)
fmovem.x 4(%a0),%fp1-%fp3
fmovem %fp0-%fp7,-(%sp)
fmovem (%sp)+,%fp0-%fp1
fmovem.x %fp2,-(%sp)
fmovem.x (%sp)+,%fp2
fmovem.x %d0,-(%sp)
fmovem.x (%sp)+,%d1
fmovem.x %d1,(%a0)
fmovem.x #0xf0,-(%sp)
fmovem.x (%a0),#0x0f
fmovem.x lab(%pc),%fp0-%fp1
fmovecr #0,%fp0
fmovecr #0x3f,%fp1
fmovecr.x #1,%fp1
fsincos.x %fp0,%fp1:%fp2
fsincos.d (%a0),%fp1:%fp2
fsincos.s #0e1.0,%fp1:%fp2
fsincos %fp0,%fp1:%fp2
ftst.x %fp0
ftst %fp0
ftst.l %d0
ftst.s (%a0)
ftst.d (%a0)
ftst.x (%a0)
ftst.p (%a0)
ftst.w #4
fcmp.x %fp0,%fp1
fcmp %fp0,%fp1
fcmp.d (%a0),%fp1
fabs.x %fp0
fabs %fp0
fabs %fp0,%fp1
fadd %fp0
fadd.d %d0,%fp0
fadd.s %d0,%fp0
fadd.x %d0,%fp0
fadd.b lab(%pc),%fp0
fadd.w ext,%fp0
fnop
fsave (%a0)
fsave -(%a0)
fsave (%a0)+
fsave 4(%a0)
fsave lab
frestore (%a0)
frestore (%a0)+
frestore -(%a0)
frestore lab(%pc)
fbeq lab
fbne ext
fbeq.w lab
fbeq.l lab
fbne.l ext
fbgt 1b
fbf 2f
fbngle lab
fbra lab
fjeq lab
fdbeq %d0,1b
fdbne %d1,ext
fdbf %d1,lab
fseq %d0
fsne (%a0)
fsf 4(%a0)
fst ext
ftrapeq
ftrapne.w #4
ftrapgt.l #400000
ftrapeq #4
fsmove.x %fp0,%fp1
fdmove.d (%a0),%fp1
fsadd.s (%a0),%fp1
fdmul.x %fp1,%fp2
fssqrt.l %d0,%fp1
fdabs %fp0
fsneg %fp1,%fp2
fddiv.w #4,%fp2
"""

SPECIAL_MOVEC_PAIRS = None  # generated below


def gen_lines(cpu, quick):
    """yield assembly source lines (no labels) for the given CPU"""
    lines = []
    ops = list(OPS_BASE)
    if cpu not in ("68000", "68010"):
        ops += OPS_020
    step = 7 if quick else 1
    k = 0
    for base in INT_BASE:
        for suf in INT_SUFFIX:
            m = base + suf
            lines.append(m)
            for a in ops:
                lines.append("%s %s" % (m, a))
            for a in ops:
                for b in ops:
                    k += 1
                    if k % step:
                        continue
                    lines.append("%s %s,%s" % (m, a, b))
    # control registers
    for r in OPS_CTRL:
        for g in ("%d1", "%a2", "(%a0)", "#1"):
            for m in ("movec", "move", "move.l", "move.w", "pmove", "pmovefd"):
                lines.append("%s %s,%s" % (m, r, g))
                lines.append("%s %s,%s" % (m, g, r))
    lines += [l for l in SPECIAL.strip().split("\n") if l.strip()]
    # FPU
    fl = []
    k = 0
    for base in FP_MON:
        for suf in FP_SUFFIX:
            m = base + suf
            fl.append(m)
            for a in OPS_FP:
                fl.append("%s %s" % (m, a))
            for a in OPS_FP:
                for b in OPS_FP:
                    k += 1
                    if k % step:
                        continue
                    fl.append("%s %s,%s" % (m, a, b))
    for base in ("fmovem", "fmovecr", "fsincos", "fsave", "frestore", "fnop"):
        for suf in ("", ".l", ".x"):
            m = base + suf
            fl.append(m)
            for a in OPS_FP:
                fl.append("%s %s" % (m, a))
                for b in OPS_FP:
                    fl.append("%s %s,%s" % (m, a, b))
    for c in FCC:
        for suf in ("", ".w", ".l"):
            for t in ("lab", "ext", "1b", "2f", "(%a0)", "#1"):
                fl.append("fb%s%s %s" % (c, suf, t))
        for t in ("lab", "ext", "1b"):
            fl.append("fdb%s %%d1,%s" % (c, t))
        for a in ("%d0", "(%a0)", "(%a0)+", "-(%a0)", "4(%a0)", "ext", "%a0", "lab(%pc)", "#1"):
            fl.append("fs%s %s" % (c, a))
        fl.append("ftrap%s" % c)
        fl.append("ftrap%s.w #4" % c)
        fl.append("ftrap%s.l #4" % c)
        fl.append("ftrap%s #4" % c)
    fl += [l for l in SPECIAL_FP.strip().split("\n") if l.strip()]
    lines += fl
    # GNU as crashes ("Internal error in m68k_ip") on these
    lines = [l for l in lines if not re.match(r"fmovem #\S+,%fp(cr|sr|iar)", l)]
    return lines


CHUNK = 30000
PREAMBLE = "\t.text\nlab:\tnop\n"


def write_chunk(path, lines, skip=()):
    with open(path, "w") as f:
        f.write(PREAMBLE)
        # line numbers: source line i (0-based) is at file line 3 + 3*i + 1 (label, insn share a line)
        for i, l in enumerate(lines):
            if i in skip:
                f.write("\n\n\n")
            else:
                f.write("1:\n2:\nT%d:\t%s\n" % (i, l))
        f.write("Tend:\n1:\n2:\n\tnop\n")


def line_of(i):
    return 2 + 3 * i + 3


def idx_of(line):
    return (line - 3) // 3


def err_idx(stderr, src, n):
    return set(i for i in (idx_of(x) for x in err_lines(stderr, src)) if 0 <= i < n)


# Deliberate, documented divergences from GNU as (see toolchain/src/as/NOTES.md)
KNOWN = [
    ("gas_only", r"^trap #", "GNU as only warns and assembles 'trap #0' for an out-of-range or symbolic vector; p68-as rejects it"),
    ("p68_only", r"^(b|fb)\w+\.[sb] .*ext|ext(\+\d+)?\(%pc,%|\(ext,%pc,%", "GNU as wrongly range-checks 8-bit pc-relative references to external symbols; p68-as emits R_68K_PC8"),
    ("gas_only", r"^movec(\.l)? .*#", "GNU as accepts a number instead of a control register for movec (and encodes unknown ones as 0)"),
    ("diff", r"^f\w+\.x #0e", "GNU as emits a malformed extended-precision immediate (no pad word, no explicit integer bit); p68-as emits the 68881 format"),
    ("gas_only", r"^f\w+\.[bwl] #0e", "GNU as accepts a floating-point literal as an integer-format FPU immediate"),
    ("gas_only", r"^movep.*0x12345|^fmovecr.*#(0x12345678|0e)|^fmovem.*,#0e", "GNU as silently truncates / zeroes out-of-range or floating operands here; p68-as rejects them"),
    ("diff", r"^movep.*(ext|0x12345)", "GNU as silently drops the relocation of a symbolic movep displacement; p68-as emits R_68K_16"),
]


def known(cat, line):
    for c, rx, _why in KNOWN:
        if c == cat and re.search(rx, line):
            return True
    return False


def corpus_chunk(args):
    cpu, ci, lines, keep = args
    d = os.path.join(TMP, cpu)
    os.makedirs(d, exist_ok=True)
    src = os.path.join(d, "c%d.s" % ci)
    flags = CPUS[cpu]
    gflags = flags + GAS_EXTRA.get(cpu, [])
    res = dict(total=len(lines), ok=0, both_reject=0, gas_only=[], p68_only=[], diff=[], fatal=[])
    write_chunk(src, lines)
    g = run([GAS] + gflags + [src, "-o", src + ".g.o"])
    p = run([P68AS] + flags + [src, "-o", src + ".p.o"])
    gerr = err_idx(g.stderr, src, len(lines))
    perr = err_idx(p.stderr, src, len(lines))
    if p.returncode != 0 and not perr:
        res["fatal"].append("p68-as failed without line errors: " + p.stderr[:300])
    for i in sorted(gerr - perr):
        res["p68_only"].append(lines[i])       # gas rejects, p68 accepts
    for i in sorted(perr - gerr):
        res["gas_only"].append(lines[i])       # gas accepts, p68 rejects
    res["both_reject"] = len(gerr & perr)
    bad = gerr | perr
    # second pass on the lines both accept (several rounds: late errors such as range checks)
    for _round in range(6):
        write_chunk(src, lines, bad)
        g = run([GAS] + gflags + [src, "-o", src + ".g.o"])
        p = run([P68AS] + flags + [src, "-o", src + ".p.o"])
        if g.returncode == 0 and p.returncode == 0:
            break
        ge = err_idx(g.stderr, src, len(lines))
        pe = err_idx(p.stderr, src, len(lines))
        if not ge and not pe:
            res["fatal"].append("assembler failed: gas=%d p68=%d %s %s" % (g.returncode, p.returncode, g.stderr[:200], p.stderr[:200]))
            return res
        for i in sorted(ge - pe):
            res["p68_only"].append(lines[i])
        for i in sorted(pe - ge):
            res["gas_only"].append(lines[i])
        res["both_reject"] += len(ge & pe)
        bad |= ge | pe
    else:
        res["fatal"].append("no convergence")
        return res
    go, po = Obj(src + ".g.o"), Obj(src + ".p.o")
    gt, pt = go.bytes_of(".text"), po.bytes_of(".text")
    gs = {s["name"]: s["value"] for s in go.syms if s["name"].startswith("T")}
    psy = {s["name"]: s["value"] for s in po.syms if s["name"].startswith("T")}
    gr, pr = go.relocs_of(".text"), po.relocs_of(".text")
    live = [i for i in range(len(lines)) if i not in bad]

    def slices(text, syms, rel):
        out = {}
        ri = 0
        for n, i in enumerate(live):
            a = syms["T%d" % i]
            b = syms["T%d" % live[n + 1]] if n + 1 < len(live) else syms["Tend"]
            rs = []
            while ri < len(rel) and rel[ri][0] < b:
                if rel[ri][0] >= a:
                    rs.append((rel[ri][0] - a,) + rel[ri][1:])
                ri += 1
            out[i] = (text[a:b], rs)
        return out
    try:
        gsl, psl = slices(gt, gs, gr), slices(pt, psy, pr)
    except KeyError as e:
        res["fatal"].append("missing label %s" % e)
        return res
    for i in live:
        if gsl[i] == psl[i]:
            res["ok"] += 1
        else:
            res["diff"].append("%-40s gas %s %s | p68 %s %s" % (lines[i], gsl[i][0].hex(), gsl[i][1] or "", psl[i][0].hex(), psl[i][1] or ""))
    if not keep:
        for ext in ("", ".g.o", ".p.o"):
            try:
                os.remove(src + ext)
            except OSError:
                pass
    return res


def run_corpus(cpus, quick, jobs, keep, verbose):
    allok = True
    report = open(os.path.join(TMP, "corpus_report.txt"), "w")
    for cpu in cpus:
        lines = gen_lines(cpu, quick)
        chunks = [(cpu, i // CHUNK, lines[i:i + CHUNK], keep) for i in range(0, len(lines), CHUNK)]
        tot = dict(total=0, ok=0, both_reject=0, gas_only=[], p68_only=[], diff=[], fatal=[])
        with ProcessPoolExecutor(max_workers=jobs) as ex:
            for r in ex.map(corpus_chunk, chunks):
                for k in tot:
                    tot[k] += r[k]
        nknown = 0
        for cat in ("gas_only", "p68_only", "diff"):
            keep_ = [l for l in tot[cat] if not known(cat, l)]
            nknown += len(tot[cat]) - len(keep_)
            tot[cat] = keep_
        bad = len(tot["gas_only"]) + len(tot["p68_only"]) + len(tot["diff"]) + len(tot["fatal"])
        print("corpus %-6s lines %8d  identical %8d  both-reject %8d  byte-diff %5d  gas-only-accepts %5d  p68-only-accepts %5d  known-divergences %d  fatal %d  %s"
              % (cpu, tot["total"], tot["ok"], tot["both_reject"], len(tot["diff"]), len(tot["gas_only"]), len(tot["p68_only"]), nknown,
                 len(tot["fatal"]), "PASS" if not bad else "FAIL"))
        sys.stdout.flush()
        for k, title in (("fatal", "FATAL"), ("diff", "BYTE DIFFERENCES"), ("gas_only", "GAS ACCEPTS, P68 REJECTS"), ("p68_only", "P68 ACCEPTS, GAS REJECTS")):
            if tot[k]:
                report.write("==== %s %s (%d)\n" % (cpu, title, len(tot[k])))
                for l in tot[k]:
                    report.write(l + "\n")
                if verbose:
                    print("  ---- %s (%d), first 15:" % (title, len(tot[k])))
                    for l in tot[k][:15]:
                        print("   ", l)
        if bad:
            allok = False
    report.close()
    print("corpus report: %s" % os.path.join(TMP, "corpus_report.txt"))
    return allok

# ----------------------------------------------------------------------------- repository sources
MACHINE_FLAGS = {
    "se": "-DTARGET_MACSE -DPLUSOS_MACHINE_SE -DFB_DEFAULT_WIDTH=512 -DFB_DEFAULT_HEIGHT=342 -DFB_DEFAULT_ROWBYTES=64",
    "plus": "-DTARGET_MACPLUS -DPLUSOS_MACHINE_PLUS -DFB_DEFAULT_WIDTH=512 -DFB_DEFAULT_HEIGHT=342 -DFB_DEFAULT_ROWBYTES=64",
    "classic": "-DTARGET_MACCLASSIC -DPLUSOS_MACHINE_CLASSIC -DFB_DEFAULT_WIDTH=512 -DFB_DEFAULT_HEIGHT=342 -DFB_DEFAULT_ROWBYTES=64",
    "lc3": "-DTARGET_MACLC3 -DPLUSOS_MACHINE_LC3 -DFB_DEFAULT_WIDTH=640 -DFB_DEFAULT_HEIGHT=480 -DFB_DEFAULT_ROWBYTES=80",
    "universal": "-DTARGET_UNIVERSAL -DPLUSOS_MACHINE_UNIVERSAL -DFB_DEFAULT_WIDTH=512 -DFB_DEFAULT_HEIGHT=342 -DFB_DEFAULT_ROWBYTES=64",
}
LOADER_DEFS = ["-DKERNEL_OFFSET=(60000*512)", "-DKERNEL_BYTES=(512*512)"]
KINC = ["-Ikernel", "-Ikernel/core", "-Ikernel/cpu", "-Ikernel/proc", "-Ikernel/drivers", "-Ikernel/drivers/video", "-Ikernel/drivers/input"]


def preprocess(src, defs, out):
    r = run([GCC, "-E", "-x", "assembler-with-cpp", "-nostdinc"] + defs + [src, "-o", out], cwd=REPO)
    if r.returncode:
        raise RuntimeError("cpp failed for %s: %s" % (src, r.stderr))


def repo_jobs():
    jobs = []
    kern = ["kernel/cpu/aline.S", "kernel/cpu/exc.S", "kernel/cpu/irq.S", "kernel/cpu/trap.S", "kernel/proc/switch.S", "kernel/proc/tick.S",
            "kernel/drivers/video/vblit.S", "kernel/drivers/input/keytrans.S"]
    for m, mf in MACHINE_FLAGS.items():
        for k in kern:
            if os.path.exists(os.path.join(REPO, k)):
                jobs.append((k, m, mf.split() + KINC + ["-Isdks/macplus/include"], ["-m68000"], "build/" + k + ".o"))
        jobs.append(("boot/loader.S", m, mf.split() + LOADER_DEFS, ["-m68000"], "build/boot/loader.o"))
    for sdk in ("macplus", "macse"):
        inc = ["-Isdks/%s/include" % sdk, "-Isdks/%s/include/libc" % sdk]
        for f, extra, tag in (("src/runtime/crt0.S", [], "app"), ("src/runtime/crt0.S", ["-DPLUSOS_KERNEL"], "kernel"),
                              ("src/gfx/gfx_asm.S", [], ""), ("src/libc/setjmp.S", [], "")):
            p = "sdks/%s/%s" % (sdk, f)
            if os.path.exists(os.path.join(REPO, p)):
                built = None
                if "crt0" in f:
                    built = "sdks/%s/build/lib/crt0_%s.o" % (sdk, tag)
                jobs.append((p, tag or sdk, inc + extra, ["-m68000"], built))
    for extra in ("ROM/SE/cpp/src/rom_start.S", "tools/return_to_plusos/return_to_plusos.S"):
        if os.path.exists(os.path.join(REPO, extra)):
            jobs.append((extra, "", [], ["-m68000"], None))
    return jobs


def run_repo(verbose):
    d = os.path.join(TMP, "repo")
    os.makedirs(d, exist_ok=True)
    ok = fail = 0
    builtsame = builtdiff = 0
    current_machine = None
    for f in os.listdir(os.path.join(REPO, "build")) if os.path.isdir(os.path.join(REPO, "build")) else []:
        if f.startswith(".machine."):
            current_machine = f[len(".machine."):]
    for src, tag, defs, flags, built in repo_jobs():
        base = os.path.join(d, src.replace("/", "_") + "." + (tag or "x"))
        try:
            preprocess(src, defs, base + ".s")
        except RuntimeError as e:
            print("  SKIP %s (%s): %s" % (src, tag, str(e).strip().splitlines()[-1][:120]))
            continue
        g = run([GAS] + flags + [base + ".s", "-o", base + ".g.o"])
        p = run([P68AS] + flags + [base + ".s", "-o", base + ".p.o"])
        if g.returncode != 0:
            print("  SKIP %s (%s): GNU as rejects it: %s" % (src, tag, g.stderr.strip().splitlines()[-1][:120]))
            continue
        if p.returncode != 0:
            fail += 1
            print("  FAIL %s (%s): p68-as: %s" % (src, tag, p.stderr.strip()[:400]))
            continue
        probs = compare_objects(Obj(base + ".g.o"), Obj(base + ".p.o"), "%s (%s)" % (src, tag))
        if probs:
            fail += 1
            for x in probs[:5]:
                print("  FAIL", x)
        else:
            ok += 1
            if verbose:
                print("  ok   %s (%s)" % (src, tag))
        # objects built by the real GCC build (same MACHINE only)
        if built and os.path.exists(os.path.join(REPO, built)) and (tag in (current_machine, "app", "kernel")):
            probs = compare_objects(Obj(os.path.join(REPO, built)), Obj(base + ".p.o"), "%s vs %s" % (src, built))
            if probs:
                builtdiff += 1
                print("  NOTE differs from GCC-built %s: %s" % (built, probs[0][:200]))
            else:
                builtsame += 1
    print("repo   files identical to GNU as: %d, failing: %d; identical to existing GCC-built objects: %d, differing: %d  %s"
          % (ok, fail, builtsame, builtdiff, "PASS" if not fail else "FAIL"))
    return fail == 0


def run_loader(verbose):
    d = os.path.join(TMP, "loader")
    os.makedirs(d, exist_ok=True)
    ref_path = os.path.join(REPO, "build/boot/loader.bin")
    if not os.path.exists(ref_path):
        print("loader build/boot/loader.bin not present: SKIP")
        return True
    ref = open(ref_path, "rb").read()
    match = []
    for m, mf in MACHINE_FLAGS.items():
        base = os.path.join(d, "loader." + m)
        preprocess("boot/loader.S", mf.split() + LOADER_DEFS, base + ".s")
        p = run([P68AS, "-m68000", base + ".s", "-o", base + ".o"])
        if p.returncode:
            print("loader FAIL (%s): %s" % (m, p.stderr[:300]))
            return False
        o = Obj(base + ".o")
        if o.relocs_of(".text"):
            print("loader FAIL (%s): unexpected relocations %s" % (m, o.relocs_of(".text")[:3]))
            return False
        text = o.bytes_of(".text")
        if text == ref:
            match.append(m)
        if verbose:
            print("  loader MACHINE=%s: %d bytes %s" % (m, len(text), "== build/boot/loader.bin" if text == ref else "(differs)"))
    print("loader build/boot/loader.bin (%d bytes) reproduced with MACHINE=%s  %s" % (len(ref), "/".join(match) or "none", "PASS" if match else "FAIL"))
    return bool(match)

# ----------------------------------------------------------------------------- syntax / error files
def run_dir_compare(sub, verbose):
    """every *.s in tests/as/<sub>: first line may hold '| flags: ...'; compare with GNU as"""
    d = os.path.join(HERE, sub)
    out = os.path.join(TMP, sub)
    os.makedirs(out, exist_ok=True)
    ok = fail = 0
    for f in sorted(os.listdir(d)) if os.path.isdir(d) else []:
        if not f.endswith(".s"):
            continue
        src = os.path.join(d, f)
        first = open(src).readline()
        m = re.search(r"flags:\s*(.*)", first)
        flags = m.group(1).split() if m else ["-m68000"]
        m = re.search(r"p68flags:\s*(.*)", open(src).read(2000))
        pflags = flags + (m.group(1).split() if m else [])
        gsrc = src
        m2 = re.search(r"gas-equivalent:\s*(\S+)", open(src).read(2000))
        if m2:
            gsrc = os.path.join(d, m2.group(1))
            pflags = [x for x in pflags]
        base = os.path.join(out, f)
        g = run([GAS] + [x for x in flags if x != "--motorola"] + ["-I", d, gsrc, "-o", base + ".g.o"])
        p = run([P68AS] + pflags + ["-I", d, src, "-o", base + ".p.o"])
        if g.returncode:
            print("  FAIL %s: GNU as: %s" % (f, g.stderr.strip()[:300])); fail += 1; continue
        if p.returncode:
            print("  FAIL %s: p68-as: %s" % (f, p.stderr.strip()[:300])); fail += 1; continue
        probs = compare_objects(Obj(base + ".g.o"), Obj(base + ".p.o"), f)
        # global symbol values must agree as well
        go, po = Obj(base + ".g.o"), Obj(base + ".p.o")
        gsy = {s["name"]: (s["value"], s["shndx"] in (0, 0xfff1, 0xfff2) and s["shndx"]) for s in go.syms if s["bind"] and s["name"]}
        psy = {s["name"]: (s["value"], s["shndx"] in (0, 0xfff1, 0xfff2) and s["shndx"]) for s in po.syms if s["bind"] and s["name"]}
        if gsy != psy:
            probs.append("%s: global symbols differ: %s" % (f, sorted(set(gsy.items()) ^ set(psy.items()))[:4]))
        if probs:
            fail += 1
            for x in probs[:4]:
                print("  FAIL", x)
        else:
            ok += 1
            if verbose:
                print("  ok  ", f)
    print("%-6s files identical: %d, failing: %d  %s" % (sub, ok, fail, "PASS" if not fail else "FAIL"))
    return fail == 0


def run_errors(verbose):
    d = os.path.join(HERE, "errors")
    ok = fail = 0
    for f in sorted(os.listdir(d)) if os.path.isdir(d) else []:
        if not f.endswith(".s"):
            continue
        src = os.path.join(d, f)
        text = open(src).read()
        m = re.search(r"flags:\s*(.*)", text.splitlines()[0])
        flags = m.group(1).split() if m else ["-m68000"]
        want = [i + 1 for i, l in enumerate(text.splitlines()) if "ERROR" in l]
        p = run([P68AS] + flags + [src, "-o", os.path.join(TMP, "err.o")])
        got = sorted(err_lines(p.stderr, src))
        if p.returncode != 0 and got == want:
            ok += 1
        else:
            fail += 1
            print("  FAIL %s: expected errors on lines %s, got %s (exit %d)" % (f, want, got, p.returncode))
    print("errors files behaving: %d, failing: %d  %s" % (ok, fail, "PASS" if not fail else "FAIL"))
    return fail == 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cpu", default=",".join(CPUS))
    ap.add_argument("--quick", action="store_true", help="corpus: only every 7th operand pair")
    ap.add_argument("--only", default="")
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--verbose", "-v", action="store_true")
    a = ap.parse_args()
    if not os.path.exists(P68AS):
        sys.exit("p68-as not found at %s (run toolchain/src/as/build.sh)" % P68AS)
    os.makedirs(TMP, exist_ok=True)
    only = a.only.split(",") if a.only else ["repo", "loader", "syntax", "errors", "corpus"]
    ok = True
    if "repo" in only:
        ok &= run_repo(a.verbose)
    if "loader" in only:
        ok &= run_loader(a.verbose)
    if "syntax" in only:
        ok &= run_dir_compare("syntax", a.verbose)
    if "errors" in only:
        ok &= run_errors(a.verbose)
    if "corpus" in only:
        ok &= run_corpus(a.cpu.split(","), a.quick, a.jobs, a.keep, a.verbose)
    print("OVERALL: %s" % ("PASS" if ok else "FAIL"))
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
