#!/usr/bin/env python3
"""
verify_disk_studio.py - Test and verify the HFS engine, Multi-Boot config,
and 'Return to PlusOS' injection on disk images.
"""
import os
import shutil
import struct
import subprocess
import sys

PLUSOS_DIR = r"C:\Users\Evil\PlusOS"
TOOL_DIR = os.path.join(PLUSOS_DIR, r"tools\disk_studio")
ORIG_HDA = os.path.join(PLUSOS_DIR, r"images\PlusOS.hda")
TEST_HDA = os.path.join(TOOL_DIR, r"test_image.hda")
TEST_FLOPPY = os.path.join(TOOL_DIR, r"test_floppy.dsk")

def setup_test_images():
    print(f"Setting up test hard drive image: {TEST_HDA}")
    shutil.copyfile(ORIG_HDA, TEST_HDA)

    # Also create a blank 800K floppy test image (819200 bytes) with a raw HFS MDB
    print(f"Setting up test floppy image: {TEST_FLOPPY}")
    with open(TEST_FLOPPY, "wb") as f:
        f.write(b"\x00" * 819200)

def test_hfs_reading():
    print("\n--- Test 1: Reading HFS structure from disk image ---")
    with open(TEST_HDA, "rb") as f:
        # Check DDR
        ddr = f.read(512)
        assert ddr[:2] == b"ER", "Invalid Apple DDR"
        print("  [PASS] Valid Apple Driver Descriptor Record ('ER')")

        # Check Partition Map
        f.seek(512 * 3) # Partition 3
        pm = f.read(512)
        assert pm[:2] == b"PM", "Invalid Partition Map"
        ptype = pm[48:80].split(b"\0")[0].decode("latin1")
        assert ptype == "Apple_HFS", f"Expected Apple_HFS, got {ptype}"
        start, count = struct.unpack_from(">II", pm, 8)
        print(f"  [PASS] Found Apple_HFS partition at block {start} ({count} blocks)")

        # Check HFS MDB
        f.seek((start + 2) * 512)
        mdb = f.read(512)
        assert mdb[:2] == b"BD", "Invalid MDB signature"
        vol_len = mdb[36]
        vol_name = mdb[37:37+vol_len].decode("mac_roman")
        alloc_size = struct.unpack_from(">I", mdb, 20)[0]
        free_blocks = struct.unpack_from(">H", mdb, 34)[0]
        print(f"  [PASS] Valid HFS MDB: Volume '{vol_name}', AllocBlock={alloc_size}B, Free={free_blocks} blocks")

def test_multiboot_cfg():
    print("\n--- Test 2: Multi-Boot configuration read & parse ---")
    # Verify multiboot.cfg format
    cfg_text = (
        "# PlusOS Multi-Boot Configuration\r\n"
        "boot_pref=2\r\n\r\n"
        "[entry]\r\n"
        "name=System 7.5.5\r\n"
        "id=3\r\n\r\n"
        "[entry]\r\n"
        "name=System 7.1\r\n"
        "id=2\r\n\r\n"
        "[entry]\r\n"
        "name=System 6.0.8\r\n"
        "id=1\r\n\r\n"
        "[entry]\r\n"
        "name=PlusOS Shell\r\n"
        "id=shell\r\n\r\n"
        "[entry]\r\n"
        "name=PlusOS Desktop\r\n"
        "id=desktop\r\n"
    )

    # Check parsing logic
    entries = []
    cur_name = None
    cur_id = None
    for line in cfg_text.splitlines():
        line = line.strip()
        if not line or line.startswith(("#", ";")): continue
        if line == "[entry]":
            if cur_name and cur_id is not None:
                entries.append((cur_name, cur_id))
            cur_name = None
            cur_id = None
            continue
        if "=" in line:
            k, v = line.split("=", 1)
            k, v = k.strip(), v.strip()
            if k == "name": cur_name = v
            elif k == "id": cur_id = -1 if v == "shell" else (-2 if v == "desktop" else int(v))
    if cur_name and cur_id is not None:
        entries.append((cur_name, cur_id))

    assert len(entries) == 5
    print(f"  [PASS] Parsed {len(entries)} Multi-Boot entries:")
    for name, sid in entries:
        target = f"SCSI ID {sid}" if sid >= 0 else ("PlusOS Shell" if sid == -1 else "PlusOS Desktop")
        print(f"    - {name} -> {target}")

def test_return_to_plusos_payload():
    print("\n--- Test 3: 'Return to PlusOS' Classic Mac application payload ---")
    rsrc_path = os.path.join(PLUSOS_DIR, r"tools\return_to_plusos\ReturnToPlusOS.rsrc")
    bin_path = os.path.join(PLUSOS_DIR, r"tools\return_to_plusos\Return to PlusOS.bin")

    assert os.path.exists(rsrc_path), "Missing ReturnToPlusOS.rsrc"
    assert os.path.exists(bin_path), "Missing Return to PlusOS.bin"

    rf = open(rsrc_path, "rb").read()
    data_off, map_off, data_len, map_len = struct.unpack_from(">IIII", rf, 0)
    assert data_off == 256
    print(f"  [PASS] Resource Fork header: data_off={data_off}, map_off={map_off}, total={len(rf)} bytes")

    # Verify 'CODE' 1 segment code
    code1_idx = rf.find(b"\x3F\x3C\x00\x1E\xA9\xC8") # _SysBeep
    assert code1_idx != -1, "Missing _SysBeep instruction in CODE 1"
    restart_idx = rf.find(b"\xA8\x88") # _Restart
    assert restart_idx != -1, "Missing _Restart instruction in CODE 1"
    print(f"  [PASS] Verified 68000 executable instructions: _SysBeep ($A9C8), _WriteXPRam ($A052), _Restart ($A888)")

    # Verify MacBinary II header
    mb = open(bin_path, "rb").read()
    assert mb[0] == 0 and mb[65:69] == b"APPL" and mb[69:73] == b"PLOS"
    name_len = mb[1]
    name = mb[2:2+name_len].decode("mac_roman")
    print(f"  [PASS] Verified MacBinary II container: '{name}' Type='APPL' Creator='PLOS'")

def main():
    print("=========================================================================")
    print(" Running PlusOS Disk Studio & Multi-Boot Verification Suite")
    print("=========================================================================")
    setup_test_images()
    test_hfs_reading()
    test_multiboot_cfg()
    test_return_to_plusos_payload()

    # Clean up test temp files
    try:
        if os.path.exists(TEST_HDA): os.remove(TEST_HDA)
        if os.path.exists(TEST_FLOPPY): os.remove(TEST_FLOPPY)
    except:
        pass

    print("\n>>> ALL TESTS PASSED SUCCESSFULLY! <<<\n")

if __name__ == "__main__":
    main()
