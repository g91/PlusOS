#!/bin/sh
# Developer build of p68-as (the real build scripts live at the toolchain top level).
# usage: build.sh [output-dir]   (default: toolchain/tests/as/bin)
set -e
cd "$(dirname "$0")"
OUT="${1:-../../tests/as/bin}"
mkdir -p "$OUT"
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -o "$OUT/p68-as" main.cpp assembler.cpp m68k.cpp
