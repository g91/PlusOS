#!/usr/bin/env python3
"""diff_calls.py OLD_SRC_DIR NEW_SRC_DIR - which calls changed in/out of ROM
between two generated src/<prefix> catalog directories."""
import glob
import os
import re
import sys


def load(d):
    r = {}
    for p in glob.glob(os.path.join(d, "*.c")):
        for m in re.finditer(r'\{ "(\w+)", 0x([0-9A-F]{4}), (-?\d+), (\d), ', open(p).read()):
            r[m.group(1)] = (os.path.basename(p)[:-2], m.group(2), m.group(4) == "1")
    return r


old, new = load(sys.argv[1]), load(sys.argv[2])
for name in sorted(old):
    if name in new and old[name][2] != new[name][2]:
        print("%-28s %-12s %s  %s" % (name, new[name][0], new[name][1], "now IN ROM" if new[name][2] else "now NOT in ROM"))
