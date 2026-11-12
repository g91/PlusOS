#!/bin/sh
# build_plusos.sh - build the PlusOS SDK libraries, kernel and apps with the P68 toolchain
# into build_p68/ (the GCC-built build/ directory is left alone).
#
#   sh toolchain/build_plusos.sh [MACHINE=se] [kernel|apps|all]
#
# Uses p68-cc for every compile. The link step uses p68-ld/p68-ar when they exist in
# toolchain/bin, otherwise GNU ld/ar + mkexe.py as stand-ins.
cd "$(dirname "$0")/.."
ROOT=$(pwd)
TC=$ROOT/toolchain
CC=$TC/bin/p68-cc
OUT=${OUT:-build_p68}
MACHINE=${MACHINE:-se}
WHAT=${1:-all}
JOBS=${JOBS:-8}
case $MACHINE in
  plus) MDEF="-DTARGET_MACPLUS -DPLUSOS_MACHINE_PLUS" ;;
  classic) MDEF="-DTARGET_MACCLASSIC -DPLUSOS_MACHINE_CLASSIC" ;;
  universal) MDEF="-DTARGET_UNIVERSAL -DPLUSOS_MACHINE_UNIVERSAL" ;;
  *) MDEF="-DTARGET_MACSE -DPLUSOS_MACHINE_SE" ;;
esac
MDEF="$MDEF -DFB_DEFAULT_WIDTH=512 -DFB_DEFAULT_HEIGHT=342 -DFB_DEFAULT_ROWBYTES=64"
CFLAGS="-m68000 -Os -ffunction-sections -fdata-sections -w"
if [ -x $TC/bin/p68-ar ]; then AR="$TC/bin/p68-ar"; else AR=m68k-linux-gnu-ar; fi
mkdir -p $OUT

# compile LIST of "src obj flags..." lines in parallel; stop on the first failure
compile_all() {
  xargs -P $JOBS -L 1 sh -c 'src=$0; obj=$1; shift; mkdir -p "$(dirname "$obj")"; if [ ! "$obj" -nt "$src" ] || [ "$obj" -ot "'$CC'" ]; then "'$CC'" "$@" -c "$src" -o "$obj" || { echo "FAILED: $src" >&2; exit 255; }; fi' || exit 1
}

# ---- runtime library
{
  echo "$TC/rt/lmath.S $OUT/rt/lmath.o -m68000"
  echo "$TC/rt/int64.c $OUT/rt/int64.o -m68000 -O2 -ffunction-sections"
  echo "$TC/rt/softfloat.c $OUT/rt/softfloat.o -m68000 -O2 -ffunction-sections"
} | compile_all || exit 1
rm -f $OUT/rt/libp68rt.a
$AR rcs $OUT/rt/libp68rt.a $OUT/rt/lmath.o $OUT/rt/int64.o $OUT/rt/softfloat.o || exit 1

# ---- one SDK:  build_sdk <dir> <romprefix>
build_sdk() {
  sdk=$1; rom=$2; o=$OUT/sdk_$(basename $sdk)
  inc="-I$sdk/include/libc -I$sdk/include"
  list=$o/list.txt
  mkdir -p $o/lib
  : > $list
  for f in $sdk/src/libc/*.c $sdk/src/libc/*.S $sdk/src/$rom/*.c $sdk/src/$rom/glue/*.c $sdk/src/runtime/rt_common.c $sdk/src/runtime/font8x8.c \
           $sdk/src/runtime/rt_app.c $sdk/src/runtime/thread.c $sdk/src/net/*.c; do
    [ -f "$f" ] && echo "$f $o/obj/${f#$sdk/}.o $CFLAGS $inc" >> $list
  done
  for f in $sdk/src/gfx/*.c $sdk/src/gfx/*.S; do
    [ -f "$f" ] && echo "$f $o/obj/${f#$sdk/}.o $CFLAGS -O2 $inc" >> $list
  done
  echo "$sdk/src/runtime/crt0.S $o/lib/crt0_app.o -m68000 $inc" >> $list
  echo "$sdk/src/runtime/crt0.S $o/lib/crt0_kernel.o -m68000 $inc -DPLUSOS_KERNEL" >> $list
  compile_all < $list || return 1
  mk() { lib=$1; shift; rm -f $o/lib/$lib; $AR rcs $o/lib/$lib "$@"; }
  mk libc.a $o/obj/src/libc/*.o
  mk lib$rom.a $(find $o/obj/src/$rom -name '*.o')
  mk libplusos.a $o/obj/src/runtime/rt_common.c.o $o/obj/src/runtime/font8x8.c.o
  mk librt_app.a $o/obj/src/runtime/rt_app.c.o $o/obj/src/runtime/thread.c.o
  mk libplusgfx.a $o/obj/src/gfx/*.o
  mk libplusnet.a $o/obj/src/net/*.o
}
build_sdk sdks/macplus plusrom || exit 1
build_sdk sdks/macse serom || exit 1
SP=$OUT/sdk_macplus
SS=$OUT/sdk_macse

link_pex() {  # link_pex out.pex NAME crt0 objs... -- libs...
  out=$1; name=$2; shift 2
  if [ -x $TC/bin/p68-ld ]; then
    $TC/bin/p68-ld -T sdks/macplus/ld/plusos.ld --gc-sections -Map=${out%.pex}.map --name $name -o $out "$@"
  else
    m68k-linux-gnu-ld -T sdks/macplus/ld/plusos.ld -q --gc-sections --no-warn-rwx-segments -Map=${out%.pex}.map -o ${out%.pex}.elf "$@" &&
    python3 sdks/macplus/tools/mkexe.py ${out%.pex}.elf $out --name $name
  fi
}

# ---- kernel
if [ "$WHAT" = all ] || [ "$WHAT" = kernel ]; then
  KINC="-I$ROOT/sdks/macplus/include/libc -I$ROOT/sdks/macplus/include -Ikernel -Ikernel/core -Ikernel/cpu -Ikernel/proc -Ikernel/drivers -Ikernel/drivers/video -Ikernel/drivers/input -Ikernel/drivers/sound -Ikernel/services -idirafter sdks/macse/include"
  list=$OUT/kernel.list
  : > $list
  for f in $(ls kernel/*.c kernel/*.S kernel/*/*.c kernel/*/*.S kernel/*/*/*.c kernel/*/*/*.S 2>/dev/null); do
    echo "$f $OUT/$f.o $CFLAGS $MDEF $KINC" >> $list
  done
  compile_all < $list || exit 1
  KOBJ=$(sed 's/^[^ ]* \([^ ]*\) .*/\1/' $list)
  mkdir -p $OUT/kernel
  link_pex $OUT/kernel/kernel.pex PLUSOS $SP/lib/crt0_kernel.o $KOBJ --start-group $SP/lib/libplusos.a $SP/lib/libplusrom.a $SP/lib/libc.a $OUT/rt/libp68rt.a $SS/lib/libserom.a --end-group || exit 1
  # boot block
  mkdir -p $OUT/boot
  $CC -m68000 $MDEF "-DKERNEL_OFFSET=(60000*512)" "-DKERNEL_BYTES=(512*512)" -c boot/loader.S -o $OUT/boot/loader.o || exit 1
  if [ -x $TC/bin/p68-ld ]; then $TC/bin/p68-ld --oformat binary -Ttext 0 -o $OUT/boot/loader.bin $OUT/boot/loader.o
  else m68k-linux-gnu-objcopy -O binary $OUT/boot/loader.o $OUT/boot/loader.bin; fi
  ls -l $OUT/kernel/kernel.pex $OUT/boot/loader.bin
fi

# ---- apps
if [ "$WHAT" = all ] || [ "$WHAT" = apps ]; then
  mkdir -p $OUT/apps
  okc=0; badc=0; bad=""
  for dir in apps/*/*/; do
    a=$(basename $dir)
    if [ -f $dir/SE ]; then S=$SS; sdk=sdks/macse; rom=serom; else S=$SP; sdk=sdks/macplus; rom=plusrom; fi
    inc="-I$sdk/include/libc -I$sdk/include"
    objs=""; fail=0
    for f in $dir*.c $dir*.S; do
      [ -f "$f" ] || continue
      o=$OUT/$f.o; mkdir -p $(dirname $o)
      if [ ! "$o" -nt "$f" ] || [ "$o" -ot "$CC" ]; then $CC $CFLAGS $inc -c $f -o $o 2>$OUT/apps/$a.log || fail=1; fi
      objs="$objs $o"
    done
    ls $dir*.cpp >/dev/null 2>&1 && fail=1 && echo "C++ app (not yet supported)" > $OUT/apps/$a.log
    if [ $fail = 0 ] && link_pex $OUT/apps/$a.pex $(echo $a | tr a-z A-Z) $S/lib/crt0_app.o $objs --start-group $S/lib/librt_app.a $S/lib/libplusgfx.a $S/lib/libplusnet.a $S/lib/libplusos.a $S/lib/lib$rom.a $S/lib/libc.a $OUT/rt/libp68rt.a --end-group >>$OUT/apps/$a.log 2>&1; then
      okc=$((okc+1))
    else badc=$((badc+1)); bad="$bad $a"; fi
  done
  echo "apps built: $okc   failed: $badc $bad"
fi
