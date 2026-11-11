#Requires -Version 5.1
<#
.SYNOPSIS
    PlusOS Portable Windows Build Environment Setup
    Downloads MSYS2, installs build tools, and compiles the Retro68 m68k
    cross-compiler from source — no WSL, no Linux required.

.DESCRIPTION
    One-shot setup. Run once; afterwards use build.bat (or the Disk Studio
    "Build Image" button) to compile PlusOS normally on Windows.

    What this script does:
      1. Downloads MSYS2 portable (≈90 MB) if not found
      2. Extracts it to tools\winbuild\msys2\
      3. Runs the MSYS2 first-time init (pacman -Syu)
      4. Installs CMake, Ninja, GCC, Python, Git inside MSYS2
      5. Clones Retro68 and builds it inside MSYS2 (~20–40 min)
      6. Installs the toolchain to tools\winbuild\toolchain\
      7. Writes tools\winbuild\env.txt  (paths for build.bat)

.NOTES
    Progress is printed to stdout so the Disk Studio can capture it live.
#>
[CmdletBinding()]
param(
    [switch]$SkipMsys2Download,   # Reuse existing MSYS2 at Msys2Dir
    [switch]$SkipRetro68Build,    # Skip Retro68 build (use existing toolchain)
    [switch]$Quiet                # Suppress banner
)

$ErrorActionPreference = 'Stop'
$ProgressPreference    = 'SilentlyContinue'

# ── Paths ───────────────────────────────────────────────────────────────────
$WinbuildDir  = $PSScriptRoot
$RepoRoot     = Split-Path -Parent $WinbuildDir   # <repo>/tools/winbuild → <repo>
$Msys2Dir     = Join-Path $WinbuildDir "msys2"
$ToolchainDir = Join-Path $WinbuildDir "toolchain"
$EnvFile      = Join-Path $WinbuildDir "env.txt"

# ── Helpers ─────────────────────────────────────────────────────────────────
function Step([string]$n, [string]$msg) {
    Write-Host "[$n] $msg" -ForegroundColor Cyan
}
function Ok([string]$msg)   { Write-Host "  OK  $msg" -ForegroundColor Green }
function Warn([string]$msg) { Write-Host "  !!  $msg" -ForegroundColor Yellow }
function Fail([string]$msg) { Write-Host "FAIL  $msg" -ForegroundColor Red; throw $msg }

# Convert a Windows path to MSYS2's /drive/... form
function ToMsys2Path([string]$WinPath) {
    if ($WinPath.Length -ge 2 -and $WinPath[1] -eq ':') {
        $drive = $WinPath[0].ToString().ToLower()
        $rest  = $WinPath.Substring(2).Replace('\', '/')
        return "/$drive$rest"
    }
    return $WinPath.Replace('\', '/')
}

# Run a command inside MSYS2 bash (login shell = full PATH with ucrt64 tools)
function Invoke-Msys2([string]$BashExe, [string]$Cmd, [string]$WorkDir = "") {
    $env:MSYSTEM = "UCRT64"
    $loginCmd = if ($WorkDir) { "cd '$(ToMsys2Path $WorkDir)' && $Cmd" } else { $Cmd }
    & $BashExe --login -c $loginCmd
    if ($LASTEXITCODE -ne 0) { throw "MSYS2 command failed (exit $LASTEXITCODE): $Cmd" }
}

if (-not $Quiet) {
    Write-Host ""
    Write-Host "=================================================" -ForegroundColor Magenta
    Write-Host "  PlusOS Windows-Native Build Environment Setup  " -ForegroundColor Magenta
    Write-Host "=================================================" -ForegroundColor Magenta
    Write-Host "  Repo    : $RepoRoot"
    Write-Host "  MSYS2   : $Msys2Dir"
    Write-Host "  Toolchain: $ToolchainDir"
    Write-Host ""
}

# ─────────────────────────────────────────────────────────────────────────────
# STEP 1  —  Locate or download MSYS2
# ─────────────────────────────────────────────────────────────────────────────
Step "1/5" "Locating MSYS2..."

$BashExe = $null

# Search order: local winbuild → user-installed common locations
$msys2Candidates = @(
    (Join-Path $Msys2Dir "usr\bin\bash.exe"),
    "C:\msys64\usr\bin\bash.exe",
    "C:\msys2\usr\bin\bash.exe",
    "$env:USERPROFILE\msys64\usr\bin\bash.exe"
)
foreach ($c in $msys2Candidates) {
    if (Test-Path $c) { $BashExe = (Resolve-Path $c).Path; break }
}

if ($BashExe) {
    Ok "MSYS2 found: $BashExe"
} elseif ($SkipMsys2Download) {
    Fail "MSYS2 not found and -SkipMsys2Download was set."
} else {
    Write-Host "  MSYS2 not found — downloading portable archive (~90 MB)..." -ForegroundColor Yellow

    # Fetch the latest nightly SFX from MSYS2's GitHub releases
    $sfxUrl = "https://github.com/msys2/msys2-installer/releases/download/nightly-x86_64/msys2-base-x86_64-latest.sfx.exe"
    $sfxPath = Join-Path $env:TEMP "msys2-base.sfx.exe"

    Write-Host "  Downloading: $sfxUrl"
    try {
        Invoke-WebRequest -Uri $sfxUrl -OutFile $sfxPath -UseBasicParsing
    } catch {
        # Try alternative URL format
        $sfxUrl2 = "https://repo.msys2.org/distrib/msys2-x86_64-latest.sfx.exe"
        Write-Host "  Primary URL failed, trying: $sfxUrl2"
        Invoke-WebRequest -Uri $sfxUrl2 -OutFile $sfxPath -UseBasicParsing
    }

    Write-Host "  Extracting to $Msys2Dir ..."
    New-Item -ItemType Directory -Force -Path $Msys2Dir | Out-Null

    # The SFX is a 7zip self-extractor; run with -y (yes) -o<dir>
    $sfxArgs = @("-y", "-o$Msys2Dir")
    & $sfxPath @sfxArgs | Out-Null

    # The archive extracts as "msys64\" inside the target directory
    $innerDir = Join-Path $Msys2Dir "msys64"
    if (Test-Path (Join-Path $innerDir "usr\bin\bash.exe")) {
        # Flatten: move msys64\* → msys2\*
        Write-Host "  Flattening msys64\ → msys2\ ..."
        $tmpMove = Join-Path $Msys2Dir "_tmp_msys64"
        Rename-Item $innerDir $tmpMove
        Get-ChildItem $tmpMove | Move-Item -Destination $Msys2Dir -Force
        Remove-Item $tmpMove -Recurse -Force -ErrorAction SilentlyContinue
    }

    Remove-Item $sfxPath -Force -ErrorAction SilentlyContinue

    $BashExe = Join-Path $Msys2Dir "usr\bin\bash.exe"
    if (-not (Test-Path $BashExe)) {
        Fail "MSYS2 extraction failed — bash.exe not found at $BashExe"
    }
    Ok "MSYS2 installed at $Msys2Dir"
}

# ─────────────────────────────────────────────────────────────────────────────
# STEP 2  —  First-time MSYS2 initialization
# ─────────────────────────────────────────────────────────────────────────────
Step "2/5" "Initializing MSYS2 (pacman -Syu)..."

$env:MSYSTEM = "UCRT64"
# First run: initialize pacman keyring and DB
& $BashExe --login -c "pacman-key --init; pacman-key --populate msys2 2>/dev/null || true; pacman -Syu --noconfirm --noprogressbar 2>&1 | tail -20"
# A second pass is sometimes needed after the first pacman -Syu restarts
& $BashExe --login -c "pacman -Syu --noconfirm --noprogressbar 2>&1 | tail -10" 2>$null
Ok "MSYS2 up to date"

# ─────────────────────────────────────────────────────────────────────────────
# STEP 3  —  Install build dependencies
# ─────────────────────────────────────────────────────────────────────────────
Step "3/5" "Installing build dependencies (CMake, Ninja, GCC, Python, Git, Make)..."

$packages = @(
    "make",
    "git",
    "python",
    "mingw-w64-ucrt-x86_64-cmake",
    "mingw-w64-ucrt-x86_64-ninja",
    "mingw-w64-ucrt-x86_64-gcc",
    "mingw-w64-ucrt-x86_64-python"
)
$pkgList = $packages -join " "
& $BashExe --login -c "pacman -S --noconfirm --noprogressbar --needed $pkgList 2>&1 | grep -v '^warning:'"
if ($LASTEXITCODE -ne 0 -and $LASTEXITCODE -ne 1) {
    Fail "Failed to install MSYS2 packages (exit $LASTEXITCODE)"
}
Ok "Build dependencies installed"

# ─────────────────────────────────────────────────────────────────────────────
# STEP 4  —  Build Retro68 cross-compiler (m68k-apple-macos-gcc)
# ─────────────────────────────────────────────────────────────────────────────
$toolchainGcc = Join-Path $ToolchainDir "bin\m68k-apple-macos-gcc.exe"

if ($SkipRetro68Build -and (Test-Path $toolchainGcc)) {
    Ok "Retro68 already built, skipping (use -SkipRetro68Build=\$false to force rebuild)"
} elseif (Test-Path $toolchainGcc) {
    Ok "Retro68 already built at $ToolchainDir"
} else {
    Step "4/5" "Building Retro68 m68k cross-compiler (20-40 min, one time only)..."
    Write-Host "  This compiles GCC from source — please be patient." -ForegroundColor Yellow
    Write-Host ""

    # Copy the bootstrap script into the MSYS2 bash to run it
    $bootstrapSrc  = Join-Path $WinbuildDir "bootstrap_toolchain.sh"
    $toolchainMsys = ToMsys2Path $ToolchainDir
    $repoMsys      = ToMsys2Path $RepoRoot

    # Run the bootstrap script inside MSYS2 login shell
    $env:MSYSTEM = "UCRT64"
    & $BashExe --login -c "bash '$(ToMsys2Path $bootstrapSrc)' '$toolchainMsys' '$repoMsys'"
    if ($LASTEXITCODE -ne 0) {
        Fail "Retro68 build failed (exit $LASTEXITCODE). Check output above."
    }

    if (-not (Test-Path $toolchainGcc)) {
        Fail "Retro68 build completed but gcc.exe not found at $toolchainGcc"
    }
    Ok "Retro68 installed: $toolchainGcc"
}

# ─────────────────────────────────────────────────────────────────────────────
# STEP 5  —  Write env.txt so build.bat can find MSYS2 and the toolchain
# ─────────────────────────────────────────────────────────────────────────────
Step "5/5" "Writing environment config ($EnvFile)..."

$msys2BashDir  = Split-Path -Parent $BashExe   # <msys2>\usr\bin
$msys2RootDir  = Split-Path -Parent (Split-Path -Parent $msys2BashDir)  # <msys2>

@"
# PlusOS portable build environment — generated by setup.ps1
# Edit by hand only if you know what you're doing.
MSYS2_DIR=$msys2RootDir
MSYS2_BASH=$BashExe
TOOLCHAIN_DIR=$ToolchainDir
"@ | Set-Content $EnvFile -Encoding UTF8

Ok "Config written"

# ─────────────────────────────────────────────────────────────────────────────
# Done
# ─────────────────────────────────────────────────────────────────────────────
Write-Host ""
Write-Host "=================================================" -ForegroundColor Green
Write-Host "  Build environment ready!                       " -ForegroundColor Green
Write-Host "=================================================" -ForegroundColor Green
Write-Host "  MSYS2    : $BashExe"
Write-Host "  Toolchain: $ToolchainDir"
Write-Host ""
Write-Host "  To compile PlusOS, run:" -ForegroundColor White
Write-Host "      build.bat          (builds kernel + all apps)" -ForegroundColor Cyan
Write-Host "      build.bat install  (builds + writes disk image)" -ForegroundColor Cyan
Write-Host ""
