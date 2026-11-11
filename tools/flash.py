import getpass
import glob
import hashlib
import os
import shutil
import subprocess
import sys

def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while True:
            chunk = f.read(1024 * 1024)
            if not chunk:
                break
            h.update(chunk)
    return h.hexdigest()

def copy_and_flush(src, dst):
    print(f"Writing {dst} directly from {src}...")
    with open(src, "rb") as fsrc, open(dst, "wb") as fdst:
        shutil.copyfileobj(fsrc, fdst, length=1024 * 1024)
        fdst.flush()
        try:
            os.fsync(fdst.fileno())
        except Exception:
            pass
    # Force system write-back
    try:
        subprocess.run(["sync"], check=False)
    except Exception:
        pass

def find_project_root():
    """Return the PlusOS repo root by walking up from this script's location."""
    here = os.path.dirname(os.path.abspath(__file__))  # tools/
    root = os.path.dirname(here)                        # PlusOS/
    if os.path.isfile(os.path.join(root, "Makefile")):
        return root
    raise RuntimeError(
        f"Cannot find PlusOS repo root (expected Makefile in {root})"
    )

def find_hfsutils():
    """Locate the Retro68 toolchain bin/ dir for hfsutils; returns None if absent."""
    username = getpass.getuser()
    candidates = [
        f"/home/{username}/Retro68-build/toolchain/bin",
        "/usr/local/retro68/bin",
    ]
    # Also scan any Windows user homes accessible via WSL
    candidates += sorted(glob.glob("/mnt/c/Users/*/Retro68-build/toolchain/bin"))
    for c in candidates:
        if os.path.isdir(c):
            return c
    return None

def main():
    root = find_project_root()
    os.chdir(root)

    # Ensure loader.bin is built
    subprocess.check_call(["make", "MACHINE=lc3", "loader"])

    apps = sorted(glob.glob("build/apps/*.pex"))
    print(f"Found {len(apps)} apps to install.")

    # Install onto images/PlusOS.hda
    cmd = [
        "python3", "tools/install.py",
        "--image", "images/PlusOS.hda",
        "--loader", "build/boot/loader.bin",
        "--kernel", "build/kernel/kernel.pex",
        "--kernel-block", "60000",
        "--kernel-blocks", "512",
        "--apps-block", "61000",
        "--hfs"
    ]
    for a in apps:
        name = os.path.basename(a)[:-4].upper()
        cmd.extend(["--app", f"{name}={a}"])

    print("Running install.py on images/PlusOS.hda...")
    env = os.environ.copy()
    hfsutils = find_hfsutils()
    if hfsutils:
        env["HFSUTILS"] = hfsutils
    subprocess.check_call(cmd, env=env)

    src = "images/PlusOS.hda"
    src_hash = sha256_file(src)
    print(f"Source image SHA256: {src_hash}")

    # Flash to SD card if present (/mnt/f); skip gracefully if not mounted
    sdcard = "/mnt/f"
    if os.path.ismount(sdcard):
        targets = [
            f"{sdcard}/HD60_512 PlusOS.hda",
            f"{sdcard}/BOOTTEST.hda",
        ]
        for dst in targets:
            copy_and_flush(src, dst)
            dst_hash = sha256_file(dst)
            print(f"Target {dst} SHA256: {dst_hash}")
            if dst_hash != src_hash:
                raise RuntimeError(
                    f"Hash mismatch on {dst}! Write did not commit cleanly."
                )
            print(f"  Verified clean copy on {dst} (SHA256 matches source)")
        print("\n=======================================================")
        print("ALL TARGETS SUCCESSFULLY FLASHED AND VERIFIED ON DISK!")
        print("=======================================================\n")
    else:
        print(
            "\nSD card not mounted at /mnt/f — image written to images/PlusOS.hda only.\n"
            "Insert the BlueSCSI SD card and re-run to flash it.\n"
        )

if __name__ == "__main__":
    main()
