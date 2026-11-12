#!/bin/sh
# developer helper: rebuild the toolchain and run the compiler tests (WSL)
cd /mnt/c/Users/Evil/Desktop/mac/PlusOS/toolchain
export NODE="/mnt/c/Users/Evil/AppData/Local/Temp/claude/3451b76c6ae4a8e5/fbab00df-308f-4f26-84ec-6a5d8685326c/scratchpad/node/node-v22.11.0-win-x64/node.exe" OUT=../../obj/tests
sh build.sh 2>&1 | grep -E "error|built" | head -30
sh tests/cc/run.sh "$@" 2>&1 | head -${LINES_MAX:-60}
