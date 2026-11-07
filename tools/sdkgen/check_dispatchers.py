#!/usr/bin/env python3
"""check_dispatchers.py - list every multiversal selector dispatcher and what the
Plus / SE ROMs actually have at that trap number (to catch package traps whose
code is not in ROM). Usage: check_dispatchers.py DEFS PLUS.json SE.json"""
import glob
import json
import os
import sys

import yaml

defs, pj, sj = sys.argv[1:4]
roms = {k: {t["trap"]: t for t in json.load(open(p))["traps"]} for k, p in (("plus", pj), ("se", sj))}
norm = lambda t: (0xA800 | (t & 0x3FF)) if t & 0x0800 else (0xA000 | (t & 0xFF))
for path in sorted(glob.glob(os.path.join(defs, "*.yaml"))):
    for item in yaml.safe_load(open(path)) or []:
        if isinstance(item, dict) and "dispatcher" in item:
            d = item["dispatcher"]
            t = norm(d["trap"])
            cols = []
            for k in ("plus", "se"):
                e = roms[k].get(t)
                cols.append("%-5s %-22s" % (k, (e["rom_labels"][0] if e and e["rom_labels"] else "-") if e else "-"))
            print("%-22s %-14s %04X  %s" % (d["name"], os.path.basename(path)[:-5], t, "  ".join(cols)))
