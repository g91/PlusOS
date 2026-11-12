#!/bin/sh
# Compile each tests/cc/t*.c with p68-cc at several optimisation levels, link it with the
# test crt0 and the runtime, run it on the emulator's CPU core and report pass/fail.
#   usage: run.sh [test-name-pattern]      (run inside WSL/Linux or Git Bash)
# Needs NODE (path to a node binary) unless `node` is on PATH. Uses p68-ld when present,
# otherwise GNU ld as a stand-in linker.
cd "$(dirname "$0")"
TC=../..
NODE=${NODE:-node}
OUT=${OUT:-/tmp/p68tests}
mkdir -p "$OUT"
CC="$TC/bin/p68-cc"
pass=0; fail=0
"$CC" -m68000 -c crt0.S -o "$OUT/crt0.o" || exit 1
"$CC" -m68000 -c $TC/rt/lmath.S -o "$OUT/lmath.o" || exit 1
RTOBJS="$OUT/lmath.o"
for f in $TC/rt/*.c; do
  [ -f "$f" ] || continue
  o="$OUT/rt_$(basename "$f" .c).o"
  "$CC" -m68000 -O2 -c "$f" -o "$o" || exit 1
  RTOBJS="$RTOBJS $o"
done
for t in ${1:-t}*.c; do
  name=$(basename "$t" .c)
  for O in -O0 -O1 -O2 -Os; do
    cpu=68000
    if "$CC" -m$cpu $O -I. -c "$t" -o "$OUT/$name.o" 2>"$OUT/$name.err"; then
      if [ -x "$TC/bin/p68-ld" ]; then
        "$TC/bin/p68-ld" --oformat binary -Ttext 0x10000 -e _start -o "$OUT/$name.bin" "$OUT/crt0.o" "$OUT/$name.o" $RTOBJS 2>>"$OUT/$name.err"
      else
        m68k-linux-gnu-ld -Ttext 0x10000 -e _start --oformat binary -o "$OUT/$name.bin" "$OUT/crt0.o" "$OUT/$name.o" $RTOBJS 2>>"$OUT/$name.err"
      fi
      if [ $? -eq 0 ] && "$NODE" "$TC/../emulator/tools/run68k.mjs" --cpu $cpu --max-insns 200000000 "$OUT/$name.bin" >"$OUT/$name.out" 2>&1; then
        pass=$((pass+1))
      else
        fail=$((fail+1)); echo "FAIL $name $O"; head -12 "$OUT/$name.out" 2>/dev/null; head -5 "$OUT/$name.err"
      fi
    else
      fail=$((fail+1)); echo "FAIL(compile) $name $O"; head -8 "$OUT/$name.err"
    fi
  done
done
echo "passed $pass, failed $fail"
[ $fail -eq 0 ]
