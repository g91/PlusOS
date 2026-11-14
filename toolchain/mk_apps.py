#!/usr/bin/env python3
"""mk_apps.py - compile and link every PlusOS app with the P68 toolchain.

Runs natively on Windows (or anywhere Python runs). Each app is a folder under apps/*/*;
a file named SE in the folder selects the Mac SE SDK, otherwise the Mac Plus SDK. C++ apps
are skipped (the compiler's C++ front end does not yet cover classes). Only recompiles
objects whose source is newer, like make.
"""
import argparse
import os
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def newer(src, obj):
    return not os.path.exists(obj) or os.path.getmtime(src) > os.path.getmtime(obj)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default="build_p68")
    ap.add_argument("--cc", required=True)
    ap.add_argument("--ld", required=True)
    ap.add_argument("--jobs", type=int, default=0)
    a = ap.parse_args()
    os.chdir(REPO)
    # normalise tool paths for this OS and make them absolute so subprocess finds them
    a.cc = os.path.abspath(os.path.normpath(a.cc))
    a.ld = os.path.abspath(os.path.normpath(a.ld))
    out = a.out
    cflags = "-m68000 -Os -ffunction-sections -fdata-sections -w".split()

    sdk = {"plus": ("sdks/macplus", "plusrom", os.path.join(out, "sdk_macplus")),
           "se": ("sdks/macse", "serom", os.path.join(out, "sdk_macse"))}

    app_dirs = []
    for cat in sorted(os.listdir("apps")):
        catp = os.path.join("apps", cat)
        if not os.path.isdir(catp):
            continue
        for name in sorted(os.listdir(catp)):
            d = os.path.join(catp, name)
            if os.path.isdir(d):
                app_dirs.append((name, d))

    ok, bad, skipped = [], [], []
    os.makedirs(os.path.join(out, "apps"), exist_ok=True)
    for name, d in app_dirs:
        srcs = [os.path.join(d, f) for f in sorted(os.listdir(d))
                if f.endswith((".c", ".S"))]
        cpps = [f for f in os.listdir(d) if f.endswith(".cpp")]
        if not srcs and not cpps:
            continue            # not a plain app folder (e.g. apps/tools/Python: built by its own build.py)
        if cpps and not srcs:
            skipped.append(name + " (C++)")
            continue
        which = "se" if os.path.exists(os.path.join(d, "SE")) else "plus"
        sdir, rom, slib = sdk[which]
        inc = ["-I" + sdir + "/include/libc", "-I" + sdir + "/include"]
        objs = []
        failed = False
        logp = os.path.join(out, "apps", name + ".log")
        log = open(logp, "w")
        for s in srcs:
            o = os.path.join(out, s + ".o")
            os.makedirs(os.path.dirname(o), exist_ok=True)
            if newer(s, o):
                r = subprocess.run([a.cc] + cflags + inc + ["-c", s, "-o", o],
                                   stdout=log, stderr=subprocess.STDOUT)
                if r.returncode != 0:
                    failed = True
                    break
            objs.append(o)
        if failed:
            log.close()
            bad.append(name)
            continue
        pex = os.path.join(out, "apps", name + ".pex")
        lib = slib + "/lib"
        cmd = [a.ld, "-T", sdir + "/ld/plusos.ld", "--gc-sections",
               "--name", name.upper(), "-o", pex, lib + "/crt0_app.o"] + objs + [
               "--start-group", lib + "/librt_app.a", lib + "/libplusgfx.a",
               lib + "/libplusnet.a", lib + "/libplusos.a", lib + "/lib" + rom + ".a",
               lib + "/libc.a", os.path.join(out, "rt", "libp68rt.a"), "--end-group"]
        r = subprocess.run(cmd, stdout=log, stderr=subprocess.STDOUT)
        log.close()
        if r.returncode == 0 and os.path.exists(pex):
            ok.append(name)
        else:
            bad.append(name)

    print("  apps built: %d  failed: %d  skipped: %d" % (len(ok), len(bad), len(skipped)))
    if bad:
        print("  failed:", " ".join(bad))
    if skipped:
        print("  skipped:", " ".join(skipped))
    return 0


if __name__ == "__main__":
    sys.exit(main())
