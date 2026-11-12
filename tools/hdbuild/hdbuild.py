#!/usr/bin/env python3
"""hdbuild.py - PlusOS hard-disk / floppy image builder.

    hdbuild.py stage   [--root build/disk_root] ...     lay the volume out as a folder
    hdbuild.py image   --root DIR --out IMAGE ...        folder -> disk image (from scratch)
    hdbuild.py build   --out IMAGE ...                   stage + image
    hdbuild.py ls | info | extract | verify | extract-driver

Pure Python 3.8+ standard library; runs unchanged on Windows and Linux.
See tools/hdbuild/README.md.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from hdlib.cli import main  # noqa: E402

if __name__ == "__main__":
    sys.exit(main())
