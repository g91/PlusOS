#!/usr/bin/env python3
"""freeze.py - generate Python/frozen_modules/*.h with the host Python (needs the same
bytecode magic as this tree, i.e. a 3.15 host).  Replaces the Makefile's _freeze_module step."""
import os, re, subprocess, sys
PY = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(PY)
txt = open("Makefile.pre.in").read()
BS = chr(92)
txt = txt.replace(BS + chr(13) + chr(10), " ").replace(BS + chr(10), " ")
rules = re.findall(r"\$\(FREEZE_MODULE(?:_BOOTSTRAP)?\)\s+(\S+)\s+(\S+)\s+(\S+)", txt)
os.makedirs("Python/frozen_modules", exist_ok=True)
seen = set()
for name, src, out in rules:
    src = src.replace("$(srcdir)/", "")
    if "$" in src or "$" in out or out in seen or not out.startswith("Python/frozen_modules") or not os.path.exists(src):
        continue
    seen.add(out)
    if os.path.exists(out) and os.path.getmtime(out) > os.path.getmtime(src):
        continue
    r = subprocess.run([sys.executable, "Programs/_freeze_module.py", name, src, out])
    if r.returncode:
        sys.exit("freeze failed: " + name)
print("froze", len(seen), "modules")
