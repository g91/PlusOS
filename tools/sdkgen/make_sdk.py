#!/usr/bin/env python3
"""
make_sdk.py - build one complete, standalone PlusOS SDK folder.

    make_sdk.py --machine plus|se --out sdks/macplus --rom-bin ROM.bin --rom-map MAP.lst.txt
                --split-inc DIR --split-src DIR --defs multiversal/defs --macrom mac_rom

Steps: decode the machine's ROM trap table; copy the shared template (libc,
PlusOS runtime, linker script, build files) filling in the machine-specific
names; generate the ROM layer (include/<prefix>, src/<prefix>, docs); generate
the console font; write the README. The output folder's build/ is kept.
"""
import argparse
import json
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import gen_sdk  # noqa: E402

EXAMPLE_ERROR = {
    "plus": "error: 'ADBReInit' is unavailable: ADBReInit: trap 0xA07B is not in the Mac Plus ROM",
    "se": "error: 'WaitNextEvent' is unavailable: WaitNextEvent: trap 0xA860 is not in the Mac SE ROM",
}


def render(text, tokens):
    for k, v in tokens.items():
        text = text.replace("@%s@" % k, str(v))
    return text


def copy_template(template, out, tokens, prefix):
    for root, _dirs, files in os.walk(template):
        rel = os.path.relpath(root, template)
        # include/rom -> include/<prefix>, src/rom -> src/<prefix>
        parts = [prefix if (i == 1 and p == "rom" and rel.split(os.sep)[0] in ("include", "src")) else p
                 for i, p in enumerate(rel.split(os.sep))] if rel != "." else []
        dst_dir = os.path.join(out, *parts)
        os.makedirs(dst_dir, exist_ok=True)
        for f in files:
            if f == "README.md.in":
                continue                          # written last, needs the stats
            src = os.path.join(root, f)
            if f.endswith(".in"):
                name = f[:-3]
                with open(src, encoding="utf-8") as fh:
                    data = render(fh.read(), tokens)
                with open(os.path.join(dst_dir, name), "w", newline="\n", encoding="utf-8") as fh:
                    fh.write(data)
            else:
                shutil.copyfile(src, os.path.join(dst_dir, f))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--machine", required=True, choices=sorted(gen_sdk.MACHINES))
    for x in ("--out", "--rom-bin", "--rom-map", "--split-inc", "--split-src", "--defs", "--macrom"):
        ap.add_argument(x, required=True)
    a = ap.parse_args()
    m = gen_sdk.MACHINES[a.machine]
    prefix = m["prefix"]
    out = os.path.abspath(a.out)
    template = os.path.join(HERE, "template")

    # start clean, but keep the SDK's own build output
    if os.path.isdir(out):
        for entry in os.listdir(out):
            if entry != "build":
                p = os.path.join(out, entry)
                shutil.rmtree(p) if os.path.isdir(p) else os.remove(p)
    os.makedirs(os.path.join(out, "gen"), exist_ok=True)

    traps = os.path.join(out, "gen", "traps.json")
    subprocess.run([sys.executable, os.path.join(HERE, "decode_traps.py"), a.rom_bin, a.rom_map, traps], check=True)

    tokens = dict(prefix=prefix, PREFIX=prefix.upper(), MACHINE=m["name"], machine=a.machine,
                  sdkdir="sdks/mac" + a.machine)
    copy_template(template, out, tokens, prefix)

    stats_path = os.path.join(out, "gen", "stats.json")
    args = argparse.Namespace(machine=a.machine, traps=traps, split_inc=a.split_inc, split_src=a.split_src,
                              defs=a.defs, macrom=a.macrom, inc=os.path.join(out, "include", prefix),
                              src=os.path.join(out, "src", prefix), docs=os.path.join(out, "docs"),
                              stats=stats_path)
    gen_sdk.Gen(args).run()
    subprocess.run([sys.executable, os.path.join(HERE, "genfont.py"),
                    os.path.join(out, "src", "runtime", "font8x8.c")], check=True, stdout=subprocess.DEVNULL)

    st = json.load(open(stats_path))
    tokens.update(NHEADERS=st["headers"], NTRAPS=st["traps"], NTOOL=st["toolbox"], NOS=st["os"],
                  NLOWMEM=st["lowmem"], NHW=st["hw"], NEQU=st["equ"], EXAMPLE_ERROR=EXAMPLE_ERROR[a.machine])
    with open(os.path.join(template, "README.md.in"), encoding="utf-8") as fh:
        readme = render(fh.read(), tokens)
    with open(os.path.join(out, "README.md"), "w", newline="\n", encoding="utf-8") as fh:
        fh.write(readme)
    print("make_sdk: %s SDK written to %s" % (m["name"], out))


if __name__ == "__main__":
    main()
