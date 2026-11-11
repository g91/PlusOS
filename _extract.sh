#!/bin/bash
export PATH="/home/evil/Retro68-build/toolchain/bin:$PATH"
cd /mnt/c/Users/Evil/PlusOS
OUT=/mnt/c/Users/Evil/AppData/Local/Temp/claude/6c5fad65428cb315/188de904-1a2e-4d6a-b0b3-3533fc62a266/scratchpad
for p in 2 3 4 1; do
  if hmount images/PlusOS.hda "$p" >/dev/null 2>&1; then echo "mounted partition $p"; break; fi
done
hdir : 2>/dev/null | grep -i dbgrun || true
hcopy -t ":dbgrun.trace" "$OUT/d_trace.txt" 2>&1 || echo "hcopy failed"
humount >/dev/null 2>&1 || true
wc -c "$OUT/d_trace.txt" 2>/dev/null || echo "no trace file"
