# Macintosh LC III ($067C) vs Macintosh SE ($0276) Architectural Diff Report

## 1. System Architecture Upgrades
| Feature | Macintosh SE | Macintosh LC III |
|---|---|---|
| **CPU** | Motorola 68000 @ 8 MHz (24-bit bus) | Motorola 68030 @ 25 MHz (32-bit clean) |
| **ROM Size** | 256 KB ($400000-$43FFFF) | 1024 KB ($40800000-$408FFFFF) |
| **ROM Version** | `$0276` (SE) | `$067C` (SuperMario / Vail) |
| **Dispatch Table** | `$000D32` (Fixed 512 entries) | `$000D32B0` (1,280 compressed entries) |
| **Video Hardware** | 512×342 1-bit RAM ($003FA700) | Sonora Built-in Video 640×480 ($50F00000) |
| **SCSI Hardware** | NCR 5380 (polled handshake) | NCR 53C96 SCSI-2 Core ($50F10000) |
| **MMU / Cache** | None (68000 flat memory) | 68030 on-chip MMU + CACR/CAAR caches |

## 2. Key Hardware & Boot Vector Differences
```
Header Checksum:  SE=$B2E362A8    LC-III=$ECBBC41C
ROM Version:      SE=$0276        LC-III=$067C
Reset Vector:     SE=$40002A      LC-III=$4080000A -> $4080008C
Dispatch Table:   SE=$000D32      LC-III=$000D32B0 (Decoded: 512 Toolbox + 768 OS)
Resource Map:     SE=$03EC10      LC-III=$07EC10
```

## 3. Discovered Function Matches & Opcode Alterations
| Symbol | SE Addr | LC III Offset | Signature Status | Byte Difference Details |
|---|---|---|---|---|
| `DRAWBEEPSCREEN` | `0x0001DA` | `0x000530` | IDENTICAL_8B | SE: `48 6d ff fc a8 6e 48 6e fe 00 a8 6f 24 55 48 6a`<br>LC3: `48 6d ff fc a8 6e 48 6e fe 00 aa 00 24 55 48 6a` |
| `INITRSRCMGR` | `0x0007E8` | `0x0011D0` | IDENTICAL_8B | SE: `50 f8 0a 58 42 b8 0a 50 42 67 a9 95 54 4f 4e 75`<br>LC3: `50 f8 0a 58 42 b8 0a 50 1f 38 1e fc 08 f8 00 00` |

## 4. Boot Block & Execution Analysis for PlusOS
- **Boot Drive Register ABI**: In SE/Plus ROM, `$0210` (`BootDrive`) was read by the boot blocks. In the LC III Universal ROM, `BootDrive` is unpopulated on entry; register `A1` holds `DrvQEl` and `D0` holds the drive number.
- **Cache Coherency**: On the LC III, 68030 instruction and data caches must be flushed via `movec d0, cacr` immediately after loading executable code into RAM before jumping.
- **Sonora Framebuffer**: The framebuffer base is at `$50100000` (mapped in lowmem `ScrnBase` = `$0824`), with `ScreenRow` = `80` (640x480).
