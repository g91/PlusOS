#!/usr/bin/env bash
# bootstrap_toolchain.sh  —  Build Retro68 (m68k-apple-macos-gcc) inside MSYS2.
#
# Usage (called by setup.ps1):
#   bash bootstrap_toolchain.sh  <install_prefix>  <repo_root>
#
# Both arguments are MSYS2-style paths (e.g. /c/Users/Anyone/PlusOS/...)
#
# The script:
#   1. Clones Retro68 from GitHub (depth=1 for speed)
#   2. Configures with cmake -G Ninja
#   3. Builds with ninja (uses all available CPU cores)
#   4. Installs to <install_prefix>/
#
# On success, <install_prefix>/bin/m68k-apple-macos-gcc.exe exists.

set -euo pipefail

INSTALL_PREFIX="${1:-/c/PlusOS/tools/winbuild/toolchain}"
REPO_ROOT="${2:-}"
BUILD_DIR="/tmp/retro68-build-$$"
SRC_DIR="/tmp/Retro68-src"

echo ""
echo "=== Retro68 Cross-Compiler Bootstrap ==="
echo "  Install prefix : $INSTALL_PREFIX"
echo "  Build dir      : $BUILD_DIR"
echo ""

# Ensure UCRT64 tools are in PATH
export PATH="/ucrt64/bin:/usr/bin:$PATH"

# Verify required tools
for tool in cmake ninja gcc git python3; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "ERROR: $tool not found in PATH. Run setup.ps1 to install dependencies."
        exit 1
    fi
done
echo "[prereqs] cmake=$(cmake --version | head -1)  ninja=$(ninja --version)  gcc=$(gcc --version | head -1)"

# ── Clone or update Retro68 source ──────────────────────────────────────────
echo ""
echo "[1/3] Fetching Retro68 source code..."

if [[ -d "$SRC_DIR/.git" ]]; then
    echo "  Source already cloned; pulling latest..."
    git -C "$SRC_DIR" pull --ff-only 2>&1 | tail -3
else
    echo "  Cloning from https://github.com/autc04/Retro68.git ..."
    git clone --depth=1 https://github.com/autc04/Retro68.git "$SRC_DIR" 2>&1 | tail -10
fi
echo "  Done."

# ── Configure ───────────────────────────────────────────────────────────────
echo ""
echo "[2/3] Configuring Retro68 (CMake + Ninja)..."

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake -G "Ninja" \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
    -DCMAKE_BUILD_TYPE=Release \
    -DRETRO68_SKIP_TESTS=ON \
    "$SRC_DIR" 2>&1 | tail -20

echo "  CMake configure complete."

# ── Build ────────────────────────────────────────────────────────────────────
echo ""
NCPU=$(nproc 2>/dev/null || echo 4)
echo "[3/3] Building Retro68 with ninja -j$NCPU  (this takes 20-40 minutes)..."
echo "  Progress updates every 60 s..."

# Run ninja with periodic progress pings so the Studio log stays alive
ninja -j"$NCPU" install 2>&1 &
NINJA_PID=$!
while kill -0 "$NINJA_PID" 2>/dev/null; do
    sleep 60
    if kill -0 "$NINJA_PID" 2>/dev/null; then
        echo "  ... still building ..."
    fi
done
wait "$NINJA_PID"
NINJA_EXIT=$?

if [[ $NINJA_EXIT -ne 0 ]]; then
    echo "ERROR: ninja build failed (exit $NINJA_EXIT)"
    exit $NINJA_EXIT
fi

# ── Verify ───────────────────────────────────────────────────────────────────
GCC_EXE="$INSTALL_PREFIX/bin/m68k-apple-macos-gcc"
# On Windows (MSYS2), the binary will have .exe suffix
if [[ -f "${GCC_EXE}.exe" ]]; then GCC_EXE="${GCC_EXE}.exe"; fi

if [[ ! -f "$GCC_EXE" ]]; then
    echo "ERROR: Compiler not found after build: $GCC_EXE"
    ls "$INSTALL_PREFIX/bin/" 2>/dev/null || true
    exit 1
fi

echo ""
echo "=== Retro68 installed successfully! ==="
echo "  Compiler : $GCC_EXE"
"$GCC_EXE" --version 2>&1 | head -1
echo ""

# Clean up build directory to save disk space (~2 GB)
echo "  Cleaning build cache (~2 GB)..."
rm -rf "$BUILD_DIR"
echo "  Done."
