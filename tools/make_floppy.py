#!/usr/bin/env python3
"""
make_floppy.py - Create a minimal, bootable PlusOS installation floppy disk (.dsk).

Contains ONLY essential files required to boot the machine and run the hard drive installer.
No games or non-essential files are included to keep image size small and fast.
Supports 1.44M (default) and 800K Macintosh floppy disks.
"""

import os
import sys
import shutil
import struct
import subprocess
import argparse

BLK = 512
FLOPPY_KERNEL_BLOCK = 100
FLOPPY_KERNEL_BLOCKS = 350

def run_cmd(args):
    r = subprocess.run(args, capture_output=True, text=True)
    return r

def make_floppy(output_path, size_type="1.44m", label="PlusOS Install"):
    total_bytes = 1474560 if size_type == "1.44m" else 819200
    print(f"Creating bootable {size_type} floppy image: {output_path} ({total_bytes} bytes)")

    os.makedirs(os.path.dirname(os.path.abspath(output_path)), exist_ok=True)
    with open(output_path, "wb") as f:
        f.write(b"\0" * total_bytes)

    # 1. Format HFS filesystem
    hformat = shutil.which("hformat")
    if not hformat:
        print("Error: hformat not found in PATH")
        return False

    r = run_cmd(["hformat", "-l", label, output_path])
    if r.returncode != 0:
        print(f"hformat failed: {r.stderr}")
        return False
    print("  [OK] Formatted HFS volume:", label)

    # 2. Write floppy boot blocks (loader_floppy.bin)
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    loader_path = os.path.join(repo_root, "build", "boot", "loader_floppy.bin")
    if not os.path.exists(loader_path):
        # Fallback to standard loader
        loader_path = os.path.join(repo_root, "build", "boot", "loader.bin")

    if os.path.exists(loader_path):
        with open(loader_path, "rb") as lf:
            loader_bytes = lf.read()
        with open(output_path, "r+b") as f:
            f.seek(0)
            f.write(loader_bytes.ljust(1024, b"\0"))
        print(f"  [OK] Written boot blocks ({len(loader_bytes)} bytes) to block 0-1")
    else:
        print("  [!] Warning: loader binary not found")

    # 3. Write kernel to block 100
    kernel_path = os.path.join(repo_root, "build", "kernel", "kernel.pex")
    if os.path.exists(kernel_path):
        with open(kernel_path, "rb") as kf:
            kdata = kf.read()
        with open(output_path, "r+b") as f:
            f.seek(FLOPPY_KERNEL_BLOCK * BLK)
            f.write(kdata)
        print(f"  [OK] Installed PlusOS kernel ({len(kdata)} bytes) at block {FLOPPY_KERNEL_BLOCK}")
    else:
        print("  [!] Warning: kernel.pex not found")

    # 4. Mount with hfsutils and copy essential system apps
    r = run_cmd(["hmount", output_path])
    if r.returncode != 0:
        print(f"hmount failed: {r.stderr}")
        return False

    try:
        run_cmd(["hmkdir", ":bin"])
        run_cmd(["hmkdir", ":Desktop"])

        # Essential apps ONLY (strictly no games!)
        essential_apps = [
            "INSTALLER", "SHELL", "DESKTOP", "FILES", "PKG", "PKGMGR", "CAT", "LS", "SYSINFO",
            "TASKMAN", "CALC", "NOTES", "MEM", "VER", "BOOT", "REBOOT", "OFF", "REG"
        ]

        apps_dir = os.path.join(repo_root, "build", "apps")
        copied_count = 0
        for app in essential_apps:
            pex_path = os.path.join(apps_dir, f"{app.lower()}.pex")
            if not os.path.exists(pex_path):
                pex_path = os.path.join(apps_dir, f"{app}.pex")
            if os.path.exists(pex_path):
                dst = f":bin:{app}"
                run_cmd(["hcopy", "-r", pex_path, dst])
                run_cmd(["hattrib", "-t", "PLOS", "-c", "PLOS", dst])
                copied_count += 1

        print(f"  [OK] Copied {copied_count} essential system binaries into :bin: (games excluded)")

        # Desktop installer shortcut
        installer_pex = os.path.join(apps_dir, "installer.pex")
        if os.path.exists(installer_pex):
            run_cmd(["hcopy", "-r", installer_pex, ":Desktop:Installer"])
            run_cmd(["hattrib", "-t", "PLOS", "-c", "PLOS", ":Desktop:Installer"])
            print("  [OK] Seeded :Desktop:Installer")

        # Desktop Return to PlusOS
        ret_bin = os.path.join(repo_root, "tools", "return_to_plusos", "Return to PlusOS.bin")
        if os.path.exists(ret_bin):
            run_cmd(["hcopy", "-m", ret_bin, ":Desktop:Return to PlusOS"])
            print("  [OK] Seeded :Desktop:Return to PlusOS")

        # Multiboot config default
        cfg_content = (
            "# PlusOS Floppy Installer Boot Configuration\r\n"
            "boot_pref=1\r\n\r\n"
            "[entry]\r\nname=PlusOS Installer\r\nid=installer\r\n\r\n"
            "[entry]\r\nname=PlusOS Desktop\r\nid=desktop\r\n\r\n"
            "[entry]\r\nname=PlusOS Shell\r\nid=shell\r\n"
        )
        import tempfile
        with tempfile.NamedTemporaryFile("w", suffix=".cfg", delete=False, newline="") as t:
            t.write(cfg_content)
        run_cmd(["hcopy", "-r", t.name, ":multiboot.cfg"])
        run_cmd(["hattrib", "-t", "TEXT", "-c", "ttxt", ":multiboot.cfg"])
        os.unlink(t.name)
        print("  [OK] Seeded :multiboot.cfg (installer boot profile)")

    finally:
        run_cmd(["humount"])

    print("\n[SUCCESS] Bootable Minimal Installer Floppy successfully created at:")
    print("   ", output_path)
    return True

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Build PlusOS minimal installer floppy image")
    parser.add_argument("--output", "-o", default="images/PlusOS_Floppy.dsk", help="Output .dsk path")
    parser.add_argument("--size", "-s", choices=["1.44m", "800k"], default="1.44m", help="Floppy size")
    parser.add_argument("--label", "-l", default="PlusOS Install", help="Volume label")
    args = parser.parse_args()

    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    out = os.path.normpath(os.path.join(repo_root, args.output)) if not os.path.isabs(args.output) else args.output
    ok = make_floppy(out, args.size, args.label)
    sys.exit(0 if ok else 1)
