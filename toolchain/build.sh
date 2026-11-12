#!/bin/sh
# Build the P68 toolchain with any C++17 compiler:  ./build.sh  [CXX=g++]
set -e
cd "$(dirname "$0")"
CXX=${CXX:-g++}
CXXFLAGS=${CXXFLAGS:-"-std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter"}
mkdir -p bin obj
EXE=${EXE:-}
objs=""
for f in src/as/*.cpp; do
  case "$f" in */main.cpp) continue;; esac
  o=obj/as_$(basename "$f" .cpp).o
  [ "$o" -nt "$f" ] && [ "$o" -nt src/as/assembler.h ] || $CXX $CXXFLAGS -c "$f" -o "$o"
  objs="$objs $o"
done
ccobjs=""
for f in src/cc/*.cpp; do
  o=obj/cc_$(basename "$f" .cpp).o
  [ "$o" -nt "$f" ] && [ "$o" -nt src/cc/cc.h ] && [ "$o" -nt src/cc/parse_int.h ] || $CXX $CXXFLAGS -DP68_HAVE_AS -c "$f" -o "$o"
  ccobjs="$ccobjs $o"
done
$CXX $CXXFLAGS -o bin/p68-cc$EXE $ccobjs $objs
$CXX $CXXFLAGS -o bin/p68-as$EXE src/as/main.cpp $objs
if [ -f src/ld/main.cpp ]; then
  ldsrc=$(ls src/ld/*.cpp | grep -v '/ar_main.cpp' | grep -v '/ar.cpp' || true)
  [ -f src/ld/build.sh ] && sh src/ld/build.sh >/dev/null 2>&1 || true
fi
echo "built: bin/p68-cc bin/p68-as"
