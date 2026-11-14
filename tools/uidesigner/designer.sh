#!/bin/sh
# PlusUI Designer - draw PlusOS forms with the mouse.  ./designer.sh [project.pui]
exec python3 "$(dirname "$0")/designer.py" "$@"
