#!/usr/bin/env python3
"""package.py - build the PlusUI download archive for the website.

    python tools/uidesigner/package.py [--out Servers/web/downloads]

Writes PlusUI-<version>.zip: the framework sources, the Designer, the code generator,
the tests, the uidemo example and the documentation, laid out exactly as in the PlusOS
source tree so the archive can be unpacked straight over a checkout.
"""
import argparse
import os
import sys
import zipfile

VERSION = "1.0"
HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))

# files and folders, relative to the repository root
CONTENT = [
    "sdks/macplus/include/plusos/ui.h",
    "sdks/macplus/src/gfx/ui.c",
    "sdks/macse/include/plusos/ui.h",
    "sdks/macse/src/gfx/ui.c",
    "tools/uidesigner",
    "apps/development/uidemo",
    "docs/08_PLUSUI_GUI_FRAMEWORK.md",
]
SKIP_DIRS = {"__pycache__"}
SKIP_EXT = {".pyc", ".o", ".pex", ".log"}

README = """PlusUI %(v)s - the PlusOS GUI framework, Designer and code generator
====================================================================

What is in this archive
-----------------------
  tools/uidesigner/designer.py     PlusUI Designer: draw forms with the mouse
  tools/uidesigner/puigen.py       turns a saved .pui design into C or C++ source
  tools/uidesigner/puimodel.py     the .pui project model (shared)
  tools/uidesigner/puirender.py    pixel-exact form renderer used by the Designer
  tools/uidesigner/test/           headless tests (run on the emulator's 68k core)
  sdks/macplus/.../ui.h, ui.c      the PlusUI framework (same files under sdks/macse)
  apps/development/uidemo/         a complete example app, designed and generated
  docs/08_PLUSUI_GUI_FRAMEWORK.md  the manual

Just want to design a form?
---------------------------
You only need Python 3 (3.8 or newer) with tkinter, which the standard Windows and
macOS installers include. Nothing else to install.

    Windows:        tools\\uidesigner\\designer.cmd
    macOS / Linux:  sh tools/uidesigner/designer.sh
    any:            python tools/uidesigner/designer.py [project.pui]

Draw the form, File > Save, and send the .pui file to whoever builds the app.
Try it on the example:  python tools/uidesigner/designer.py apps/development/uidemo/uidemo.pui

Want to build apps with it?
---------------------------
Copy the sdks/, tools/, apps/ and docs/ folders from this archive over your PlusOS
source folder (they only add files). Then:

    python tools/uidesigner/puigen.py apps/tools/myapp/myapp.pui
    make.cmd apps

PlusOS sources newer than this archive already contain all of it.

Full instructions: docs/08_PLUSUI_GUI_FRAMEWORK.md, or the PlusUI chapter of the wiki.
"""


def collect():
    out = []
    for rel in CONTENT:
        p = os.path.join(REPO, rel)
        if os.path.isfile(p):
            out.append(rel)
        elif os.path.isdir(p):
            for root, dirs, files in os.walk(p):
                dirs[:] = sorted(d for d in dirs if d not in SKIP_DIRS)
                for f in sorted(files):
                    if os.path.splitext(f)[1] in SKIP_EXT:
                        continue
                    out.append(os.path.relpath(os.path.join(root, f), REPO).replace(os.sep, "/"))
        else:
            raise SystemExit("package.py: missing %s" % rel)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(REPO, "Servers", "web", "downloads"))
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    name = "PlusUI-%s" % VERSION
    path = os.path.join(a.out, name + ".zip")
    files = collect()
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr(name + "/README.txt", README % {"v": VERSION})
        for rel in files:
            z.write(os.path.join(REPO, rel), name + "/" + rel)
    print("%s: %d files, %d bytes" % (path, len(files) + 1, os.path.getsize(path)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
