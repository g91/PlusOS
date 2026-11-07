#!/bin/bash
# Refresh WSL's view of the SD card (F:). Needed whenever the card has been
# pulled and reinserted, otherwise /mnt/f shows a stale, empty mount.
umount -l /mnt/f 2>/dev/null
mkdir -p /mnt/f
if mount -t drvfs 'F:' /mnt/f 2>/dev/null; then
    echo "SD card mounted at /mnt/f"
else
    echo "warning: F: not available (is the SD card in the reader?)"
fi
