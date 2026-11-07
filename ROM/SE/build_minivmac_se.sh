#!/usr/bin/env bash
# Finish building Mini vMac (Macintosh SE) for Windows and deploy it.
#
# The source is already downloaded and extracted at /root/mnvm/src/minivmac
# (Mini vMac 36.04, from gryphel.com). This script regenerates the SE/wx86
# build config, patches the generated Windows makefile for a Linux->Windows
# cross build (mingw-w64), compiles, and deploys the .exe + your SE ROM to
# F:\MiniVMacSE\.
#
# It builds with -DCheckRomCheckSum=0 so the emulator accepts BOTH the stock
# and the tweaked SE ROM (the tweaked one has a non-standard checksum).
#
# Run from PowerShell as:
#   wsl -d Ubuntu -u root bash /mnt/c/Users/Evil/PlusOS/ROM/SE/build_minivmac_se.sh
set -e

SRC=/root/mnvm/src/minivmac
ROM_TWEAK=/mnt/c/Users/Evil/PlusOS/ROM/SE/MacSE.ROM
ROM_STOCK=/mnt/c/Users/Evil/PlusOS/ROM/SE/MacSE-stock.ROM
DEST=/mnt/f/MiniVMacSE

# make sure F: is live (WSL drvfs mount goes stale after SD reinsert)
ls /mnt/f >/dev/null 2>&1 || { umount -l /mnt/f 2>/dev/null; mount -t drvfs F: /mnt/f; }

cd "$SRC"

echo "== generate wx86 / SE build config =="
gcc setup/tool.c -o setup_t
./setup_t -t wx86 -m SE > setup.sh
. ./setup.sh

echo "== patch generated makefile for mingw cross build =="
cp -f Makefile Makefile.orig
# accept any ROM (stock or tweaked): disable Mini vMac's known-checksum check
sed -i 's#^mk_COptionsCommon = #mk_COptionsCommon = -DCheckRomCheckSum=0 #' Makefile
# native gcc -> mingw cross gcc
sed -i 's/\bgcc\b/i686-w64-mingw32-gcc/g' Makefile
# fix the mangled resource rule (bare "bld/" -> a real .res.o, correct -I dirs)
# (the generator truncates the name: the real file is cfg/main.rc)
sed -i 's#^bld/: cfg/main.r$#bld/main.res.o: cfg/main.rc#' Makefile
sed -i 's#windres.exe -i "cfg/main.r" --input-format=rc -o "bld/" -O coff  --include-dir SRC#i686-w64-mingw32-windres -i "cfg/main.rc" --input-format=rc -o "bld/main.res.o" -O coff --include-dir cfg --include-dir src#' Makefile
sed -i 's#$(ObjFiles) "bld/" -mwindows#$(ObjFiles) "bld/main.res.o" -mwindows#' Makefile
sed -i 's#minivmac.exe : $(ObjFiles) bld/#minivmac.exe : $(ObjFiles) bld/main.res.o#' Makefile
sed -i 's#rm -f "bld/"#rm -f "bld/main.res.o"#' Makefile

echo "== compile =="
make clean >/dev/null 2>&1 || true
make

echo "== deploy to $DEST (own folder; does not touch the Plus vMac.ROM on F:\\) =="
mkdir -p "$DEST"
cp -f minivmac.exe "$DEST/minivmac.exe"
cp -f "$ROM_TWEAK" "$DEST/vMac.ROM"          # default: the tweaked ROM
cp -f "$ROM_STOCK" "$DEST/vMac-stock.ROM"    # fallback copy
ls -l "$DEST"

echo
echo "DONE. Launch from PowerShell:"
echo '  tools\vmac.ps1 start -Exe "F:\MiniVMacSE\minivmac.exe" -Image "F:\HD20_512 System 7.1.hda"'
echo
echo "To A/B the tweak: boot with NO disk mounted and watch the wait before"
echo "the blinking '?' disk. To test the stock ROM instead, in $DEST copy"
echo "vMac-stock.ROM over vMac.ROM."
