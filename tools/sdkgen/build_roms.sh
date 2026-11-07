#!/bin/bash
# build_roms.sh MACROM OUTDIR CROSS
# Assemble the Mac Plus v3, Mac SE and Mac SE FDHD ROM images from the
# mac_rom source tree (in a scratch copy - the tree is not modified) and check
# each against the original ROM's fingerprint (checksum, version, MD5).
set -e
SRC=$1; OUT=$2; CROSS=$3
[ -d "$SRC/include" ] || { echo "build_roms: mac_rom tree not found at $SRC" >&2; exit 1; }
mkdir -p "$OUT"
OUT=$(realpath "$OUT")
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
cp -r "$SRC"/. "$WORK"/
cd "$WORK"
declare -A FP=(
  [mac_plus_v3]="4D1F8172 0075 8a41e0754ffd1bb00d8183875c55164c"
  [mac_se]="B2E362A8 0276 9fb38bdcc0d53d9d380897ee53dc1322"
  [mac_se_fdhd]="B306E171 0276 886444d7abc1185112391b8656c7e448"
)
for t in mac_plus_v3 mac_se mac_se_fdhd; do
    make --no-print-directory CROSS="$CROSS" "build/rom-$t.bin" >/dev/null
    f="build/rom-$t.bin"
    read -r ck ver md5 <<<"${FP[$t]}"
    got="$(xxd -p -l4 "$f" | tr a-z A-Z) $(xxd -p -s8 -l2 "$f") $(md5sum "$f" | cut -d' ' -f1)"
    if [ "$got" != "$ck $ver $md5" ]; then
        echo "build_roms: $t does not match the original ROM ($got)" >&2
        exit 1
    fi
    cp "$f" "$OUT/"
    echo "build_roms: $t OK (checksum $ck, bit-identical to the original)"
done
