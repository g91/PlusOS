#!/usr/bin/env bash
# PlusOS 2.0 Web Server Launcher (Linux / macOS)
# by the1Domo

cd "$(dirname "$0")" || exit 1
python3 server.py "$@"
