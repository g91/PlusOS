#!/usr/bin/env python3
"""build.py - compile CPython 3.15 for PlusOS (68000) with the P68 toolchain.

  python PlusOS/build.py [--out DIR] [--check]   compile every file, report failures
  python PlusOS/build.py --link                  link PYTHON.PEX too

Reads the object lists straight out of Makefile.pre.in so the file set tracks upstream, then
adds the PlusOS port layer (PlusOS/*.c) and the statically linked module set (PlusOS/config.c).
"""
import argparse, os, re, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
PY = os.path.dirname(HERE)
REPO = os.path.abspath(os.path.join(PY, "..", "..", ".."))

# files from Makefile.pre.in we never build (JIT, perf, free-threading, other OSes)
SKIP = {"Python/jit.c", "Python/jit_publish.c", "Python/perf_trampoline.c", "Python/perf_jit_trampoline.c",
        "Python/jit_unwind.c", "Python/remote_debugging.c", "Python/gc_free_threading.c",
        "Python/dynamic_annotations.c", "Modules/config.c"}

# extra built-in modules (name -> sources); everything here is statically linked
MODULES = open(os.path.join(HERE, "modules.txt")).read().split("\n") if os.path.exists(os.path.join(HERE, "modules.txt")) else []


def makefile_lists():
    BS = chr(92); txt = open(os.path.join(PY, "Makefile.pre.in")).read().replace(BS + chr(13) + chr(10), " ").replace(BS + chr(10), " ")
    out = []
    for var in ("MODULE_OBJS", "IO_OBJS", "POBJS", "PEGEN_OBJS", "TOKENIZER_OBJS", "PYTHON_OBJS", "OBJECT_OBJS"):
        m = re.search(r"^" + var + r"=\s*(.*)$", txt, re.M)
        for tok in m.group(1).split():
            if tok.endswith(".o") and not tok.startswith(("$", "@")):
                out.append(tok[:-2] + ".c")
    out.append("Parser/myreadline.c")
    return [f for f in out if f not in SKIP]


def sources():
    srcs = makefile_lists()
    srcs += [l.strip() for l in MODULES if l.strip() and not l.startswith("#")]
    srcs += sorted("PlusOS/" + f for f in os.listdir(HERE) if f.endswith((".c", ".S")))
    seen, res = set(), []
    for s in srcs:
        if s not in seen and os.path.exists(os.path.join(PY, s)):
            seen.add(s); res.append(s)
        elif s not in seen:
            print("  missing source:", s)
    return res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(REPO, "build_p68", "python"))
    ap.add_argument("--link", action="store_true")
    ap.add_argument("--install", metavar="PEX", help="copy the linked python.pex here (implies --link)")
    ap.add_argument("--opt", default="-Os")
    ap.add_argument("--only", nargs="*")
    ap.add_argument("-j", type=int, default=os.cpu_count() or 4)
    ap.add_argument("-v", action="store_true")
    a = ap.parse_args()
    os.chdir(PY)
    cc = os.path.join(REPO, "toolchain", "bin", "p68-cc.exe")
    sdk = os.path.join(REPO, "sdks", "macplus")
    flags = ["-m68000", a.opt, "-ffunction-sections", "-fdata-sections", "-w", "-DPy_BUILD_CORE",
             "-DPy_BUILD_CORE_BUILTIN", "-I", "PlusOS/include", "-I", ".", "-I", "Include", "-I", "Include/internal",
             "-I", "Modules/_io", "-I", "Modules/_sre", "-I", "Modules", "-I", "Python", "-I", "Parser",
             "-I", sdk + "/include/libc", "-I", sdk + "/include"]
    srcs = a.only or sources()
    os.makedirs(a.out, exist_ok=True)

    def one(s):
        o = os.path.join(a.out, s.replace("/", "__") + ".o")
        if os.path.exists(o) and os.path.getmtime(o) > os.path.getmtime(s):
            return s, o, 0, ""
        extra = ["-mlong-jtab"] if s in ("Python/ceval.c",) else []
        r = subprocess.run([cc] + flags + extra + ["-c", s, "-o", o], capture_output=True, text=True)
        return s, o, r.returncode, (r.stdout + r.stderr)

    ok, bad = [], []
    with ThreadPoolExecutor(a.j) as ex:
        for s, o, rc, msg in ex.map(one, srcs):
            if rc == 0:
                ok.append(o)
            else:
                bad.append(s)
                lines = [l for l in msg.splitlines() if l.strip()]
                print("FAIL", s, "|", (lines[0] if lines else "?")[:200])
                if a.v:
                    print("\n".join(lines[:10]))
    print("compiled %d ok, %d failed (of %d)" % (len(ok), len(bad), len(srcs)))
    if bad or not (a.link or a.install):
        return 1 if bad else 0
    rc = link(a.out, ok, sdk)
    if rc == 0 and a.install:
        import shutil
        os.makedirs(os.path.dirname(os.path.abspath(a.install)), exist_ok=True)
        shutil.copyfile(os.path.join(a.out, "python.pex"), a.install)
    return rc


def link(out, objs, sdk):
    ld = os.path.join(REPO, "toolchain", "bin", "p68-ld.exe")
    lib = os.path.join(REPO, "build_p68", "sdk_macplus", "lib")
    pex = os.path.join(out, "python.pex")
    cmd = [ld, "-T", sdk + "/ld/plusos.ld", "--gc-sections", "--name", "PYTHON", "-o", pex,
           lib + "/crt0_app.o"] + objs + ["--start-group", lib + "/librt_app.a", lib + "/libplusgfx.a",
           lib + "/libplusnet.a", lib + "/libplusos.a", lib + "/libplusrom.a", lib + "/libc.a",
           os.path.join(REPO, "build_p68", "rt", "libp68rt.a"), "--end-group"]
    r = subprocess.run(cmd, capture_output=True, text=True)
    print((r.stdout + r.stderr)[:4000])
    if r.returncode == 0:
        print("linked", pex, os.path.getsize(pex), "bytes")
    return r.returncode


if __name__ == "__main__":
    sys.exit(main())
