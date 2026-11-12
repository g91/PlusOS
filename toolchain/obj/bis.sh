cd /mnt/c/Users/Evil/Desktop/mac/PlusOS
lo=20; hi=1172
try() { head -$1 sdks/macplus/src/libc/posix.c > toolchain/obj/px.c; printf '\nstatic sighandler_t gzz[4];\n' >> toolchain/obj/px.c; toolchain/bin/p68-cc -w -S -Isdks/macplus/include/libc -Isdks/macplus/include toolchain/obj/px.c -o /dev/null 2>&1 | grep -q "gzz"; }
for n in $(grep -n '^}' sdks/macplus/src/libc/posix.c | cut -d: -f1); do
  if try $n; then echo "first failing after line $n"; break; fi
done
