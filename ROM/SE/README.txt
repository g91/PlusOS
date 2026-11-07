Macintosh SE ROM - rebuilt from source + tweak
===============================================

Built 2026-11-06 from the 68000 assembly source tree at F:\mac_rom
(target: mac_se, via m68k-linux-gnu-as in WSL Ubuntu).

Files
-----
MacSE.ROM        Tweaked SE ROM  (256 KB, use this one) -- the real, working
                 ROM, assembled from the 68000 source at F:\mac_rom
MacSE-stock.ROM  Unmodified SE ROM, byte-for-byte identical to Apple's
                 original (kept for reference / rollback)
cpp/             C++ reimplementation SCAFFOLD of the whole SE ROM, compiled
                 for the 68000 into cpp/libserom_cpp.a with the Retro68
                 toolchain. 102 modules, 9,545 stubbed entry points mirroring
                 the asm module tree. Complete in structure, NOT in behavior
                 (every body is an unimplemented stub) -- it is a starting
                 point for a C++ port, not a bootable ROM. See cpp/README.md.

Fingerprints
------------
MacSE-stock.ROM : size 262144, checksum B2E362A8, version 0276
                  MD5 9fb38bdcc0d53d9d380897ee53dc1322  (== Apple original)
MacSE.ROM       : size 262144, checksum B2E36294, version 0276
                  MD5 665b3c6315cbbe004fe662a956f4ecee

The stock build was verified bit-identical to Apple's 1987 SE ROM before
any change was made.

Tweak applied: faster startup disk search
------------------------------------------
Source: se/boot/boot_search.s, routine WAITFORINTERNAL.
The default "wait for the internal floppy" timeout was reduced from
15 seconds to 5 seconds. This timeout only applies when no boot timeout
is configured in PRAM; it governs how long the Mac waits for a disk that
is not yet present before the search gives up and moves on (e.g. to the
blinking "?" floppy). A disk that IS ready - SCSI hard disk or an inserted
floppy - still boots immediately; this only trims the dead wait.

The change is a single, size-preserving instruction operand edit
(MOVE.W #15,D0 -> MOVE.W #5,D0), guarded to the SE target so the SE FDHD
build stays bit-identical and no ROM offsets shift. Exactly three bytes
differ from stock: the instruction operand (offset 0xD3D) and the two
recomputed checksum fields (offset 0x00 and offset 0x37), fixed up with
tools/fixup_checksum.py.

Rebuild from scratch
--------------------
  cd /mnt/f/mac_rom   (in WSL Ubuntu, after: mount -t drvfs F: /mnt/f)
  make build/rom-mac_se.bin
  python3 tools/fixup_checksum.py build/rom-mac_se.bin
To get stock behavior back, revert se/boot/boot_search.s (git checkout)
and rebuild, or just use MacSE-stock.ROM.
