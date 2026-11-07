# PlusOS Macintosh SE ROM C++ Tool Suite — User & Management Guide

This directory contains the management, inspection, verification, and reverse-engineering CLI tools for the **Macintosh SE ROM C++ Scaffold (`libserom_cpp.a`)** and **System 7.1 Source Tree**.

## Files
- `serom_mgr.py`: Core cross-platform Python CLI engine.
- `serom.ps1`: Windows PowerShell wrapper script.
- `serom.bat`: Windows Command Prompt launcher.
- `TOOL_USER_GUIDE.md`: This reference document.

---

## Command Quick-Reference

### 1. Codebase Audit & Status
Audit all 102 C++ modules, 9,545 entry points, and track implementation progress:
```powershell
.\serom.ps1 status
```

### 2. Module Entry Point Inspection
Inspect all functions declared and defined inside a specific module:
```powershell
.\serom.ps1 inspect boot/boot_search
.\serom.ps1 inspect os/memory_mgr
```

### 3. White-Box Symbol Diffing (Assembly vs C++)
Compare any 68000 assembly file (`F:\mac_rom\se/...`) with its C++ mirror (`src/...`) to verify 1:1 symbol coverage:
```powershell
.\serom.ps1 diff boot/boot_search
.\serom.ps1 diff toolbox/quickdraw/BitBlt
```

### 4. ROM Checksum Verification & Automated Patching
Verify the 32-bit master checksum ($00) and the Macintosh SE dual-chip split checksums ($30, $34). Use `--patch` to recompute and write correct checksums:
```powershell
.\serom.ps1 checksum --rom "..\MacSE.ROM"
.\serom.ps1 checksum --rom "..\MacSE.ROM" --patch
```

### 5. Floppy Disk GCR (Group Coded Recording) 6&2 Engine
Simulate the IWM/SWIM 6&2 GCR nibblizing and flux transition rules:
```powershell
.\serom.ps1 gcr --text "Macintosh SE SuperMario Universal ROM"
```

### 6. Parameter RAM (PRAM / XPRAM) Unpacker
Unpack packed 4-bit nibbles from the RTC parameter RAM bytes:
```powershell
.\serom.ps1 pram --kbd 0x42 --clik 0x58 --vol 0x13
```

### 7. Export JSON Audit Report
Export complete metadata for all 102 modules to JSON:
```powershell
.\serom.ps1 export-json --output report.json
```
