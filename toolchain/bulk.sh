#!/bin/sh
# developer helper: compile-check PlusOS sources with p68-cc and summarise the errors
cd /mnt/c/Users/Evil/Desktop/mac/PlusOS
CC=toolchain/bin/p68-cc
SDK=sdks/macplus
INC="${INC:--Ikernel -Ikernel/core -Ikernel/cpu -Ikernel/proc -Ikernel/drivers -Ikernel/drivers/video -Ikernel/drivers/input -Ikernel/drivers/sound -Ikernel/services -I$SDK/include/libc -I$SDK/include -idirafter sdks/macse/include}"
DEF="-DTARGET_MACSE -DPLUSOS_MACHINE_SE -DFB_DEFAULT_WIDTH=512 -DFB_DEFAULT_HEIGHT=342 -DFB_DEFAULT_ROWBYTES=64"
mkdir -p toolchain/obj/bulk
ok=0; bad=0
: > toolchain/obj/bulk/errors.txt
for f in "$@"; do
  o=toolchain/obj/bulk/$(echo "$f" | tr '/' '_').o
  if $CC -m68000 -Os -ffunction-sections -fdata-sections -w $INC $DEF -c "$f" -o "$o" 2>toolchain/obj/bulk/err.txt; then ok=$((ok+1)); else bad=$((bad+1)); echo "== $f" >> toolchain/obj/bulk/errors.txt; head -3 toolchain/obj/bulk/err.txt >> toolchain/obj/bulk/errors.txt; fi
done
echo "ok $ok  failed $bad"
grep -v '^==' toolchain/obj/bulk/errors.txt | sed -E 's/^[^ ]+:[0-9]+: //; s/\(near .*//' | sort | uniq -c | sort -rn | head -40
