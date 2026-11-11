# flash.ps1 - build, flash, and hardware-flush the PlusOS image to SD card.
# Portable: derives all paths from $PSScriptRoot so it works for any user.

$ErrorActionPreference = 'Stop'

# Resolve project root (this script lives in tools/)
$ProjectRoot = Split-Path -Parent $PSScriptRoot
Set-Location $ProjectRoot

# Convert Windows project path to WSL path dynamically (works for any username)
$WSLRoot = $null
try {
    $WSLRoot = (wsl -d Ubuntu -- wslpath -a ($ProjectRoot.Replace('\', '/'))) 2>$null
} catch {}
if (-not $WSLRoot) {
    # Fallback manual conversion: C:\Users\Foo\PlusOS -> /mnt/c/Users/Foo/PlusOS
    $WSLRoot = "/mnt/" + $ProjectRoot[0].ToString().ToLower() + $ProjectRoot.Substring(2).Replace('\', '/')
}

Write-Host "Project root : $ProjectRoot" -ForegroundColor DarkGray
Write-Host "WSL path     : $WSLRoot"     -ForegroundColor DarkGray
Write-Host ""

Write-Host "Running build + install via WSL..." -ForegroundColor Cyan
wsl -d Ubuntu -- bash -c "cd '$WSLRoot' && python3 tools/flash.py"

$src = Join-Path $ProjectRoot "images\PlusOS.hda"
if (-not (Test-Path $src)) {
    Write-Error "Source image not found: $src"
    exit 1
}

# Flash to SD card (F:) if attached; skip gracefully if not present
if (Test-Path "F:\") {
    Write-Host ""
    Write-Host "Copying verified image to SD card (F:\)..." -ForegroundColor Cyan
    $targets = @("F:\HD60_512 PlusOS.hda", "F:\BOOTTEST.hda")

    foreach ($t in $targets) {
        Copy-Item -Path $src -Destination $t -Force
        $stream = [System.IO.File]::Open($t,
            [System.IO.FileMode]::Open,
            [System.IO.FileAccess]::ReadWrite,
            [System.IO.FileShare]::ReadWrite)
        $stream.Flush($true)
        $stream.Close()
        Write-Host "  Copied & Flushed $t" -ForegroundColor Green
    }

    Write-Host "Verifying SHA256 hashes..." -ForegroundColor Yellow
    $hashes = Get-FileHash -Path ($targets + $src) -Algorithm SHA256
    $hashes | Format-Table -AutoSize
    $allMatch = ($hashes | Select-Object -ExpandProperty Hash -Unique).Count -eq 1
    if ($allMatch) {
        Write-Host "`nSUCCESS: All images verified bit-for-bit identical on physical media!" -ForegroundColor Green
    } else {
        Write-Error "ERROR: Hash mismatch detected between source and SD card!"
    }
} else {
    Write-Host ""
    Write-Host "SD card (F:\) not found — image written to images\PlusOS.hda only." -ForegroundColor Yellow
    Write-Host "Insert the BlueSCSI SD card and re-run to flash it."                -ForegroundColor Yellow
}
