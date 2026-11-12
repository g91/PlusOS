#!/bin/sh
# Developer build of p68-ld and p68-ar (the real build scripts live at the toolchain top level).
# usage: build.sh [output-dir]   (default: toolchain/tests/ld/bin)
set -e
cd "$(dirname "$0")"
OUT="${1:-../../tests/ld/bin}"
mkdir -p "$OUT"
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -o "$OUT/p68-ld" linker.cpp
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -o "$OUT/p68-ar" ar_main.cpp
