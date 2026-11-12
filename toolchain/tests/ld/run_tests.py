#!/usr/bin/env python3
"""
P68 linker / archiver tests.  Run inside WSL/Linux:

    python3 toolchain/tests/ld/run_tests.py [--only kernel,apps,ar,elf,errors] [--apps a,b] [-v] [--libgcc PATH]

The repository's build/ directory (objects and .pex files produced by the GCC/GNU ld build) is
the oracle: the kernel and every app are re-linked from the same objects in the same order
and must come out byte-identical.  libgcc.a comes from the Retro68 toolchain; pass --libgcc
or set P68_LIBGCC (default: ~/.cache/p68-tests/libgcc.a, or a Retro68 tree under /home).
"""
import argparse, glob, os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLCHAIN = os.path.dirname(os.path.dirname(HERE))
REPO = os.path.dirname(TOOLCHAIN)
LD = os.environ.get("P68LD", os.path.join(HERE, "bin", "p68-ld"))
AR = os.environ.get("P68AR", os.path.join(HERE, "bin", "p68-ar"))
AS = os.environ.get("P68AS", os.path.join(TOOLCHAIN, "tests", "as", "bin", "p68-as"))
TMP = os.environ.get("P68_TMP", "/tmp/p68-ld-tests")
GNU = "m68k-linux-gnu-"


def run(cmd, **kw):
    return subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True, **kw)


def describe(ref, got):
    s = "same size" if len(ref) == len(got) else "size %d vs %d (ours)" % (len(ref), len(got))
    n = min(len(ref), len(got))
    i = next((k for k in range(n) if ref[k] != got[k]), n)
    nd = sum(1 for x, y in zip(ref, got) if x != y)
    return "%s, first difference at 0x%x, %d differing bytes; header ref %s ours %s" % (s, i, nd, ref[0x0c:0x1c].hex(), got[0x0c:0x1c].hex())


def find_libgcc(arg):
    for c in (arg, os.environ.get("P68_LIBGCC"), os.path.join(TMP, "libgcc.a"), os.path.expanduser("~/.cache/p68-tests/libgcc.a")):
        if c and os.path.exists(c):
            return c
    for c in glob.glob("/home/*/Retro68-build/toolchain/lib/gcc/m68k-apple-macos/*/libgcc.a"):
        return c
    return None


def test_kernel(libgcc, verbose):
    mapf = os.path.join(REPO, "build/kernel/kernel.map")
    ref = os.path.join(REPO, "build/kernel/kernel.pex")
    if not os.path.exists(mapf) or not os.path.exists(ref):
        print("kernel: build/kernel/kernel.{map,pex} missing: SKIP")
        return True
    inputs = []
    for l in open(mapf):
        m = re.match(r"^LOAD (.*)$", l.rstrip())
        if m:
            p = m.group(1)
            if "libgcc.a" in p:
                p = libgcc
            p = re.sub(r"^/mnt/c/Users/[^/]+/Desktop/mac/PlusOS/", "", p)
            inputs.append(p)
    objs = [p for p in inputs if p.endswith(".o")]
    libs = [p for p in inputs if p.endswith(".a")]
    out = os.path.join(TMP, "kernel.pex")
    cmd = [LD, "-T", "sdks/macplus/ld/plusos.ld", "-q", "--gc-sections", "--no-warn-rwx-segments", "-Map=" + os.path.join(TMP, "kernel.map"),
           "-o", out] + objs + ["--start-group"] + libs + ["--end-group", "--name", "PLUSOS"]
    r0 = run([x for x in cmd if not x.startswith("-Map")][:-0 or None], cwd=REPO)
    retro = r0.returncode == 0 and open(out, "rb").read() == open(ref, "rb").read()
    print("kernel: default link vs existing build/kernel/kernel.pex (Retro68 ld): %s" % ("byte-identical  PASS" if retro else "DIFFERENT  FAIL"))
    cmd.insert(1, "--no-gc-keep-macsbug")
    r = run(cmd, cwd=REPO)
    if r.returncode:
        print("kernel: FAIL: p68-ld: %s" % r.stderr.strip()[:600])
        return False
    b = open(out, "rb").read()
    gmap = os.path.join(TMP, "kernel.gnu.map")
    a, why = gnu_reference("sdks/macplus", objs[0], objs[1:], libs, None, "PLUSOS",
                           os.path.join(TMP, "kernel"), ["-Map=" + gmap])
    if a is None:
        print("kernel: FAIL: reference link failed: %s" % why)
        return False
    ok = a == b
    if ok:
        print("kernel: --no-gc-keep-macsbug link (%d bytes) byte-identical to stock GNU ld+mkexe.py  PASS" % len(a))
    else:
        print("kernel: FAIL: %s" % describe(a, b))
    mapf = gmap
    want = {}
    for l in open(mapf):
        m = re.match(r"^\s+0x([0-9a-f]{8,16})\s+([A-Za-z_][\w.$]*)$", l.rstrip())
        if m:
            want[m.group(2)] = int(m.group(1), 16)
    got = {}
    for l in open(os.path.join(TMP, "kernel.map")):
        m = re.match(r"^0x([0-9a-f]{8}) \S (\S+)$", l.rstrip())
        if m:
            got[m.group(2)] = int(m.group(1), 16)
    bad = [(k, hex(v), got.get(k) is not None and hex(got.get(k))) for k, v in want.items() if got.get(k) != v]
    print("kernel: %d map symbols compared with GNU ld's kernel.map, %d differ  %s" % (len(want), len(bad), "PASS" if not bad else "FAIL"))
    if bad and verbose:
        for x in bad[:10]:
            print("   ", x)
    return ok and not bad and retro


def app_dir(app):
    c = sorted(glob.glob(os.path.join(REPO, "apps/*/" + app))) + sorted(glob.glob(os.path.join(REPO, "apps/" + app)))
    return os.path.relpath(c[0], REPO) if c else None


def app_link(app):
    """(sdk, crt0, objects, libs-without-libgcc) of an app, or None"""
    d = app_dir(app)
    if not d:
        return None
    srcs = []
    for pat in ("*.c", "*.cpp", "*.S"):
        srcs += sorted(glob.glob(os.path.join(REPO, d, pat)))
    objs = ["build/" + os.path.relpath(s, REPO) + ".o" for s in srcs]
    if not objs or any(not os.path.exists(os.path.join(REPO, o)) for o in objs):
        return None
    se = os.path.exists(os.path.join(REPO, d, "SE"))
    sdk = "sdks/macse" if se else "sdks/macplus"
    lib = sdk + "/build/lib/"
    libs = [lib + "librt_app.a", lib + "libplusgfx.a", lib + "libplusnet.a", lib + "libplusos.a", lib + ("libserom.a" if se else "libplusrom.a"), lib + "libc.a"]
    return sdk, lib + "crt0_app.o", objs, libs


def gnu_reference(sdk, crt0, objs, libs, libgcc, name, base, extra=()):
    """link with GNU ld + mkexe.py (the flow p68-ld replaces); returns the .pex bytes or None"""
    elf = base + ".gnu.elf"
    r = run([GNU + "ld", "-T", sdk + "/ld/plusos.ld", "-q", "--gc-sections", "--no-warn-rwx-segments"] + list(extra) + ["-o", elf, crt0] + objs +
            ["--start-group"] + libs + ([libgcc] if libgcc else []) + ["--end-group"], cwd=REPO)
    if r.returncode:
        return None, "GNU ld: " + r.stderr.strip()[:200]
    r = run([sys.executable, os.path.join(REPO, sdk, "tools/mkexe.py"), elf, base + ".gnu.pex", "--name", name], cwd=REPO)
    if r.returncode:
        return None, "mkexe.py: " + (r.stderr + r.stdout).strip()[:200]
    return open(base + ".gnu.pex", "rb").read(), ""


def test_apps(libgcc, verbose, only=None):
    """every app: p68-ld output must equal GNU ld + mkexe.py run on the same objects"""
    ok = fail = skip = same_as_build = 0
    build_diff = []
    for ref in sorted(glob.glob(os.path.join(REPO, "build/apps/*.pex"))):
        app = os.path.basename(ref)[:-4]
        if only and app not in only:
            continue
        info = app_link(app)
        if not info:
            skip += 1
            continue
        sdk, crt0, objs, libs = info
        base = os.path.join(TMP, app)
        want, why = gnu_reference(sdk, crt0, objs, libs, libgcc, app.upper(), base)
        out = base + ".pex"
        cmd = [LD, "-T", sdk + "/ld/plusos.ld", "-q", "--gc-sections", "--no-warn-rwx-segments", "-o", out, crt0] + objs +               ["--start-group"] + libs + [libgcc, "--end-group", "--name", app.upper()]
        r = run(cmd, cwd=REPO)
        fresh = not any(os.path.getmtime(os.path.join(REPO, o)) > os.path.getmtime(ref) + 2 for o in objs)
        if r.returncode == 0 and fresh:
            if open(out, "rb").read() == open(ref, "rb").read():
                same_as_build += 1
            else:
                build_diff.append(app)
        r = run(cmd[:1] + ["--no-gc-keep-macsbug"] + cmd[1:], cwd=REPO)
        if want is None:
            if r.returncode:
                skip += 1          # neither linker accepts it (stale objects)
                if verbose:
                    print("  skip %s (%s)" % (app, why))
            else:
                fail += 1
                print("  FAIL %s: GNU flow fails (%s) but p68-ld succeeds" % (app, why))
            continue
        if r.returncode:
            fail += 1
            print("  FAIL %s: p68-ld: %s" % (app, r.stderr.strip()[:300]))
            continue
        b = open(out, "rb").read()
        if want == b:
            ok += 1
            if verbose:
                print("  ok   %s (%d bytes)" % (app, len(b)))
        else:
            fail += 1
            print("  FAIL %s: %s" % (app, describe(want, b)))
    print("apps: --no-gc-keep-macsbug link byte-identical to stock GNU ld+mkexe.py: %d, failing %d, skipped %d  %s"
          % (ok, fail, skip, "PASS" if not fail else "FAIL"))
    print("apps: default link byte-identical to the existing build/apps/*.pex (Retro68 ld): %d, different %d %s  %s"
          % (same_as_build, len(build_diff), build_diff[:8] if build_diff else "", "PASS" if not build_diff else "FAIL"))
    return fail == 0 and not build_diff


def test_ar(verbose):
    d = os.path.join(TMP, "ar")
    os.makedirs(d, exist_ok=True)
    objs = sorted(glob.glob(os.path.join(REPO, "build/kernel/core/*.c.o")))[:12]
    if not objs:
        print("ar: no objects in build/: SKIP")
        return True
    ours, gnu = os.path.join(d, "ours.a"), os.path.join(d, "gnu.a")
    for f in (ours, gnu):
        if os.path.exists(f):
            os.remove(f)
    problems = []
    r = run([AR, "rcs", ours] + objs)
    if r.returncode:
        problems.append("p68-ar rcs failed: " + r.stderr)
    run([GNU + "ar", "rcs", gnu] + objs)
    names = [os.path.basename(o) for o in objs]
    t1 = run([AR, "t", ours]).stdout.split()
    t2 = run([GNU + "ar", "t", ours]).stdout.split()
    t3 = run([AR, "t", gnu]).stdout.split()
    if t1 != names or t2 != names or t3 != names:
        problems.append("member listing differs: %s / %s / %s" % (t1[:3], t2[:3], t3[:3]))

    def index(path):
        return run([GNU + "nm", "-s", path]).stdout.split("\n\n")[0].replace(os.path.basename(path), "X")
    if index(ours) != index(gnu):
        problems.append("archive symbol index differs from GNU ar")
    ex = os.path.join(d, "x")
    os.makedirs(ex, exist_ok=True)
    run([AR, "x", gnu], cwd=ex)
    for o in objs:
        p = os.path.join(ex, os.path.basename(o))
        if not os.path.exists(p) or open(p, "rb").read() != open(o, "rb").read():
            problems.append("extracted member %s differs" % os.path.basename(o))
            break
    run([AR, "rcs", ours, objs[0]])
    if run([AR, "t", ours]).stdout.split() != names:
        problems.append("replacing a member changed the member list")
    for p in problems:
        print("  FAIL ar:", p)
    print("ar: create/list/extract/replace, index identical to GNU ar, archives interchangeable  %s" % ("PASS" if not problems else "FAIL"))
    return not problems


SMALL = r"""
	.text
	.globl _start
_start:	lea msg(%pc),%a0
	move.l #table,%d0
	jsr func
	move.w counter,%d1
	bsr.w func
	rts
	.section .rodata
msg:	.asciz "hello"
	.data
table:	.long _start, func, counter, 0x1234, wk
	.weak wk
	.comm counter,4,2
"""
SMALL2 = r"""
	.text
	.globl func
func:	addq.l #1,counter
	rts
	.globl unused
	.section .text.unused,"ax"
unused:	jsr nowhere
"""
FLAT_LD = """SECTIONS { . = 0x4000;
 .text : { *(.text .text.*) *(.rodata .rodata.*) . = ALIGN(4); }
 .data : { *(.data .data.*) . = ALIGN(4); }
 .bss : { *(.bss .bss.* COMMON) . = ALIGN(4); }
 /DISCARD/ : { *(.comment) *(.note*) } }
"""


def assemble(d, name, text):
    s = os.path.join(d, name + ".s")
    open(s, "w").write(text)
    for tool, suffix in ((GNU + "as", ".g.o"), (AS, ".p.o")):
        if tool == AS and not os.path.exists(AS):
            continue
        r = run([tool, "-m68000", s, "-o", os.path.join(d, name + suffix)])
        if r.returncode:
            raise RuntimeError("%s failed: %s" % (tool, r.stderr))


def test_elf(libgcc, verbose):
    d = os.path.join(TMP, "small")
    os.makedirs(d, exist_ok=True)
    problems = []
    assemble(d, "a", SMALL)
    assemble(d, "b", SMALL2)
    open(os.path.join(d, "flat.ld"), "w").write(FLAT_LD)
    variants = [".g.o"] + ([".p.o"] if os.path.exists(AS) else [])
    for v in variants:
        a, b = os.path.join(d, "a" + v), os.path.join(d, "b" + v)
        ours = os.path.join(d, "ours" + v + ".bin")
        r = run([LD, "--oformat", "binary", "-Ttext", "0x4000", "--gc-sections", "-o", ours, a, b])
        if r.returncode:
            problems.append("binary link failed (%s): %s" % (v, r.stderr))
            continue
        r = run([GNU + "ld", "-T", os.path.join(d, "flat.ld"), "--gc-sections", "-e", "_start", "-o", os.path.join(d, "gnu.elf"), a, b])
        if r.returncode:
            problems.append("GNU ld failed: " + r.stderr)
            continue
        run([GNU + "objcopy", "-O", "binary", os.path.join(d, "gnu.elf"), os.path.join(d, "gnu.bin")])
        x, y = open(os.path.join(d, "gnu.bin"), "rb").read(), open(ours, "rb").read()
        if x != y:
            problems.append("flat binary differs from GNU ld+objcopy (%s): %s vs %s" % (v, x.hex()[:160], y.hex()[:160]))
        r = run([LD, "--oformat", "binary", "-Ttext", "0x4000", "-o", ours + ".nogc", a, b])
        if r.returncode == 0 or "undefined reference to `nowhere'" not in r.stderr:
            problems.append("missing undefined-symbol error without --gc-sections (%s): %s" % (v, r.stderr[:200]))
    # boot block: flat binary of boot/loader.o must equal build/boot/loader.bin
    lo, lb = os.path.join(REPO, "build/boot/loader.o"), os.path.join(REPO, "build/boot/loader.bin")
    if os.path.exists(lo) and os.path.exists(lb):
        out = os.path.join(d, "loader.bin")
        r = run([LD, "--oformat", "binary", "-Ttext", "0", "-o", out, lo])
        if r.returncode or open(out, "rb").read() != open(lb, "rb").read():
            problems.append("boot block flat binary differs from build/boot/loader.bin: " + r.stderr[:200])
    mkexe = os.path.join(REPO, "sdks/macplus/tools/mkexe.py")
    done = 0
    for ref in sorted(glob.glob(os.path.join(REPO, "build/apps/*.pex")))[:8] if libgcc else []:
        app = os.path.basename(ref)[:-4]
        info = app_link(app)
        if not info:
            continue
        sdk, crt0, objs, libs = info
        elf = os.path.join(TMP, app + ".elf")
        tail = ["--start-group"] + libs + [libgcc, "--end-group"]
        r = run([LD, "-T", sdk + "/ld/plusos.ld", "-q", "--gc-sections", "--oformat", "elf", "-o", elf, crt0] + objs + tail, cwd=REPO)
        if r.returncode:
            problems.append("%s: ELF link failed: %s" % (app, r.stderr[:200]))
            continue
        r = run([sys.executable, mkexe, elf, elf + ".pex", "--name", app.upper()])
        if r.returncode:
            problems.append("%s: mkexe.py rejects our ELF: %s" % (app, (r.stderr + r.stdout)[:300]))
            continue
        direct = os.path.join(TMP, app + ".direct.pex")
        run([LD, "-T", sdk + "/ld/plusos.ld", "-q", "--gc-sections", "-o", direct, crt0] + objs + tail + ["--name", app.upper()], cwd=REPO)
        if open(elf + ".pex", "rb").read() != open(direct, "rb").read():
            problems.append("%s: ELF+mkexe.py output differs from direct .pex output" % app)
        done += 1
    for p in problems:
        print("  FAIL", p)
    print("binary/elf: flat binary vs GNU ld+objcopy (GNU and p68 objects), boot block, ELF -q through mkexe.py for %d apps  %s"
          % (done, "PASS" if not problems else "FAIL"))
    return not problems


def test_errors(verbose):
    d = os.path.join(TMP, "err")
    os.makedirs(d, exist_ok=True)
    problems = []

    def expect(name, srcs, args, needle):
        objs = []
        for i, s in enumerate(srcs):
            p = os.path.join(d, "%s%d.s" % (name, i))
            open(p, "w").write(s)
            r = run([GNU + "as", "-m68000", p, "-o", p + ".o"])
            if r.returncode:
                problems.append("%s: as failed %s" % (name, r.stderr))
                return
            objs.append(p + ".o")
        r = run([LD] + args + ["-o", os.path.join(d, name + ".out")] + objs)
        if r.returncode == 0 or needle not in r.stderr:
            problems.append("%s: expected error containing %r, got exit %d: %s" % (name, needle, r.returncode, r.stderr.strip()[:200]))

    hdr = "\t.section .text.crt0,\"ax\"\n\t.globl _start\n_start:\t.long 0\n\t.ascii \"PLOS\"\n\t.space 0x30\n"
    expect("undef", [hdr + "\tjsr missing\n"], ["--oformat", "pex"], "undefined reference to `missing'")
    expect("dup", [hdr + "\t.globl f\nf:\trts\n", "\t.text\n\t.globl f\nf:\trts\n"], ["--oformat", "pex"], "duplicate definition of `f'")
    expect("pc16", [hdr + "\tbsr.w far\n", "\t.text\n\t.space 40000\n\t.globl far\nfar:\trts\n"], ["--oformat", "pex"], "R_68K_PC16")
    expect("pc8", [hdr + "\tbsr.s far\n", "\t.text\n\t.space 400\n\t.globl far\nfar:\trts\n"], ["--oformat", "pex"], "R_68K_PC8")
    expect("abs16", [hdr + "\tmove.w far.w,%d0\n", "\t.text\n\t.globl far\nfar:\trts\n"], ["--oformat", "pex"], "cannot be relocated")
    expect("odd", [hdr + "\t.byte 0\n\t.long _start\n"], ["--oformat", "pex"], "odd relocation address")
    expect("noplos", ["\t.text\n\t.globl _start\n_start:\trts\n"], ["--oformat", "pex"], "PLOS")
    for p in problems:
        print("  FAIL", p)
    print("errors: undefined, duplicate, PC16/PC8 overflow, 16-bit absolute, odd reloc, missing header  %s" % ("PASS" if not problems else "FAIL"))
    return not problems


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--only", default="")
    ap.add_argument("--apps", default="")
    ap.add_argument("--libgcc", default="")
    ap.add_argument("--verbose", "-v", action="store_true")
    a = ap.parse_args()
    os.makedirs(TMP, exist_ok=True)
    if not os.path.exists(LD):
        sys.exit("p68-ld not found at %s (run toolchain/src/ld/build.sh)" % LD)
    libgcc = find_libgcc(a.libgcc)
    only = a.only.split(",") if a.only else ["kernel", "apps", "ar", "elf", "errors"]
    ok = True
    if not libgcc and ("kernel" in only or "apps" in only):
        print("libgcc.a not found (use --libgcc or P68_LIBGCC): kernel/apps tests SKIPPED")
        only = [x for x in only if x not in ("kernel", "apps")]
    if "kernel" in only:
        ok &= test_kernel(libgcc, a.verbose)
    if "apps" in only:
        ok &= test_apps(libgcc, a.verbose, a.apps.split(",") if a.apps else None)
    if "ar" in only:
        ok &= test_ar(a.verbose)
    if "elf" in only:
        ok &= test_elf(libgcc, a.verbose)
    if "errors" in only:
        ok &= test_errors(a.verbose)
    print("OVERALL: %s" % ("PASS" if ok else "FAIL"))
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
