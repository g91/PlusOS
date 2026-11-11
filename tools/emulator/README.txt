Place your Mini vMac emulator binary and ROM file here.

Required files
──────────────
  Mini vMac (BlueSCSI patched).exe   ← emulator (preferred - with SD card support)
  Mini vMac.exe                      ← emulator (stock build, also accepted)
  vMac.ROM                           ← Mac Plus or SE ROM dump (512 KB)

The ROM file MUST be in the same folder as the .exe — Mini vMac looks for it
in its working directory.  Accepted ROM filenames:
  vMac.ROM  Mac Plus.ROM  MacPlus.ROM  Mac SE.ROM  MacSE.ROM  vMacSE.ROM

The "Boot Hard Drive Image in Mini vMac" button in Disk Studio will:
  1. Detect the emulator and ROM here automatically
  2. If the ROM is found elsewhere (images/, repo root) it will offer to copy it
  3. Auto-detect the best available .hda image to boot
  4. Launch the emulator with that image

To obtain Mini vMac:
  https://www.gryphel.com/c/minivmac/

The BlueSCSI-patched build (for SD card disk images) can be found on the
PlusOS project wiki or built from the PlusOS tooling.

Disk images used for testing live in:
  images/BOOTTEST.hda   (test image written by "build install")
  images/PlusOS.hda     (main image)
