<#
.SYNOPSIS
    PlusOS Workspace Clean Script
    Deletes compiler intermediate files, object files, debug symbols,
    dependency lists, and temporary build debris unnecessary for recompiling.

.DESCRIPTION
    Removes:
      - MSVC object & resource files (*.obj, *.res, *.pdb, *.idb, *.ilk, *.exp)
      - GCC / Retro68 untracked object files (*.o) in tools/ and build/
      - Compiler dependency files (*.d)
      - Compiler build log files (*.log)
      - Python cache directories (__pycache__) and bytecode (*.pyc, *.pyo)
      - Temporary / editor backup files (*.tmp, *.bak)

.PARAMETER All
    Full wipe: also deletes all build binaries in build/ and cleans SDK outputs.

.PARAMETER RestoreTracked
    Reverts git-tracked build outputs in build/ that were modified during compilation,
    leaving only your source code modifications ready for git commit.

.PARAMETER UntrackObjects
    Runs `git rm --cached` on any compiler object files (*.obj, *.res) that were
    accidentally committed, staging their deletion so git stops tracking them.

.PARAMETER DryRun
    Previews what files would be deleted without deleting anything.
#>

[CmdletBinding()]
param(
    [switch]$All,
    [switch]$RestoreTracked,
    [switch]$UntrackObjects,
    [switch]$DryRun,
    [switch]$Help
)

if ($Help) {
    Get-Help $MyInvocation.MyCommand.Path
    exit 0
}

$repoRoot = $PSScriptRoot
Set-Location $repoRoot

Write-Host "=========================================================================" -ForegroundColor Cyan
Write-Host " PlusOS Build & Compiler Cleanup" -ForegroundColor Cyan
Write-Host "=========================================================================" -ForegroundColor Cyan
Write-Host "Repository root: $repoRoot`n" -ForegroundColor DarkGray

$deletedCount = 0
$deletedBytes = 0

function Remove-CleanFile([string]$path, [string]$reason) {
    if (Test-Path -LiteralPath $path -PathType Leaf) {
        $item = Get-Item -LiteralPath $path -ErrorAction SilentlyContinue
        if ($null -ne $item) {
            $bytes = $item.Length
            $rel = $item.FullName.Substring($repoRoot.Length).TrimStart('\', '/')
            if ($DryRun) {
                Write-Host "  [DRY-RUN] Would delete: $rel ($reason)" -ForegroundColor Yellow
            } else {
                try {
                    Remove-Item -LiteralPath $path -Force -ErrorAction Stop
                    Write-Host "  [DELETED] $rel" -ForegroundColor Green
                    $script:deletedCount++
                    $script:deletedBytes += $bytes
                } catch {
                    Write-Warning "  Could not delete $($rel): $_"
                }
            }
        }
    }
}

function Remove-CleanDirectory([string]$path, [string]$reason) {
    if (Test-Path -LiteralPath $path -PathType Container) {
        $rel = $path.Substring($repoRoot.Length).TrimStart('\', '/')
        if ($DryRun) {
            Write-Host "  [DRY-RUN] Would delete folder: $rel ($reason)" -ForegroundColor Yellow
        } else {
            try {
                Remove-Item -LiteralPath $path -Recurse -Force -ErrorAction Stop
                Write-Host "  [DELETED FOLDER] $rel" -ForegroundColor Green
                $script:deletedCount++
            } catch {
                Write-Warning "  Could not delete folder $($rel): $_"
            }
        }
    }
}

# 1. Clean MSVC object, resource, and debug/linker files (*.obj, *.res, *.pdb, *.idb, *.ilk, *.exp)
Write-Host "[1/6] Cleaning MSVC & Windows compiler artifacts..." -ForegroundColor Yellow
$msvcExtensions = @('*.obj', '*.res', '*.pdb', '*.idb', '*.ilk', '*.exp')
foreach ($ext in $msvcExtensions) {
    Get-ChildItem -Path $repoRoot -Recurse -Filter $ext -File -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -notmatch '[\\/]\.git[\\/]' } |
        ForEach-Object {
            Remove-CleanFile $_.FullName "MSVC compiler output"
        }
}

# 2. Clean tools compiled objects (*.o in tools/disk_studio, tools/return_to_plusos, etc.)
Write-Host "`n[2/6] Cleaning tool compiler objects..." -ForegroundColor Yellow
if (Test-Path "$repoRoot\tools") {
    Get-ChildItem -Path "$repoRoot\tools" -Recurse -Filter "*.o" -File -ErrorAction SilentlyContinue |
        ForEach-Object {
            Remove-CleanFile $_.FullName "Tools object file"
        }
}

# 3. Clean compiler dependency files (*.d) and logs
Write-Host "`n[3/6] Cleaning compiler dependency lists (*.d) and build logs..." -ForegroundColor Yellow
Get-ChildItem -Path $repoRoot -Recurse -Filter "*.d" -File -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -notmatch '[\\/]\.git[\\/]' } |
    ForEach-Object {
        Remove-CleanFile $_.FullName "Compiler dependency list"
    }

# 4. Clean GCC / kernel / app intermediate objects in build/
Write-Host "`n[4/6] Cleaning kernel and app object files in build/..." -ForegroundColor Yellow
# Untracked *.o in build/kernel and build/apps
$buildFolders = @("$repoRoot\build\kernel", "$repoRoot\build\apps")
foreach ($bf in $buildFolders) {
    if (Test-Path $bf) {
        Get-ChildItem -Path $bf -Recurse -Filter "*.o" -File -ErrorAction SilentlyContinue |
            ForEach-Object {
                Remove-CleanFile $_.FullName "Intermediate object file"
            }
    }
}

# Remove empty app/kernel build subdirectories created by compiler
$subDirsToClean = @(
    "$repoRoot\build\apps\development",
    "$repoRoot\build\apps\games",
    "$repoRoot\build\apps\network",
    "$repoRoot\build\apps\system",
    "$repoRoot\build\apps\tools",
    "$repoRoot\build\kernel\core",
    "$repoRoot\build\kernel\cpu",
    "$repoRoot\build\kernel\drivers",
    "$repoRoot\build\kernel\proc",
    "$repoRoot\build\kernel\services"
)
foreach ($sd in $subDirsToClean) {
    if (Test-Path $sd) {
        $remaining = Get-ChildItem -Path $sd -Recurse -File -ErrorAction SilentlyContinue
        if ($null -eq $remaining -or $remaining.Count -eq 0) {
            Remove-CleanDirectory $sd "Empty compiler output directory"
        }
    }
}

# 5. Clean Python bytecode and cache
Write-Host "`n[5/6] Cleaning Python cache and bytecode..." -ForegroundColor Yellow
Get-ChildItem -Path $repoRoot -Recurse -Filter "__pycache__" -Directory -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -notmatch '[\\/]\.git[\\/]' } |
    ForEach-Object {
        Remove-CleanDirectory $_.FullName "Python bytecode cache"
    }
Get-ChildItem -Path $repoRoot -Recurse -Include "*.pyc", "*.pyo" -File -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -notmatch '[\\/]\.git[\\/]' } |
    ForEach-Object {
        Remove-CleanFile $_.FullName "Python bytecode"
    }

# 6. Clean temporary editor / build files
Write-Host "`n[6/6] Cleaning temporary files..." -ForegroundColor Yellow
Get-ChildItem -Path $repoRoot -Recurse -Include "*.tmp", "*.bak", "*~" -File -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -notmatch '[\\/]\.git[\\/]' } |
    ForEach-Object {
        Remove-CleanFile $_.FullName "Temporary file"
    }

# Handle --All flag
if ($All) {
    Write-Host "`n[ALL] Cleaning full build/ tree and SDK tests..." -ForegroundColor Magenta
    if (Test-Path "$repoRoot\build") {
        # Check if WSL is available to run make clean
        $wslAvailable = Get-Command "wsl" -ErrorAction SilentlyContinue
        if ($wslAvailable) {
            Write-Host "  Invoking WSL make clean..." -ForegroundColor DarkGray
            wsl -d Ubuntu --cd /mnt/c/Users/Evil/PlusOS -- make clean
        } else {
            Remove-CleanDirectory "$repoRoot\build" "Full build tree"
        }
    }
}

# Handle --UntrackObjects flag (untracks any .obj or .res files in git index)
if ($UntrackObjects) {
    Write-Host "`n[GIT] Removing tracked compiler objects from git index..." -ForegroundColor Cyan
    $trackedObjects = git ls-files '*.obj' '*.res'
    if ($trackedObjects) {
        foreach ($f in $trackedObjects) {
            git rm --cached $f
            Write-Host "  [UNTRACKED FROM GIT] $f" -ForegroundColor Green
        }
    } else {
        Write-Host "  No tracked .obj or .res files found in git index." -ForegroundColor DarkGray
    }
}

# Handle --RestoreTracked flag (reverts tracked build outputs in build/ to clean git state)
if ($RestoreTracked) {
    Write-Host "`n[GIT] Restoring git-tracked build outputs to clean state..." -ForegroundColor Cyan
    git restore "$repoRoot\build" 2>$null
    git restore "$repoRoot\PlusOSDiskStudio.exe" 2>$null
    git restore "$repoRoot\tools\disk_studio\PlusOSDiskStudio.exe" 2>$null
    Write-Host "  Restored tracked binaries in build/ and tools/ to repo commit state." -ForegroundColor Green
}

# Summary
Write-Host "`n=========================================================================" -ForegroundColor Cyan
if ($DryRun) {
    Write-Host " Dry-run complete. No files were deleted." -ForegroundColor Yellow
} else {
    $kb = [math]::Round($deletedBytes / 1024, 1)
    Write-Host " Cleanup finished: $deletedCount item(s) deleted ($kb KB freed)." -ForegroundColor Green
}
Write-Host "=========================================================================" -ForegroundColor Cyan

# Show git status if git is available
$gitCmd = Get-Command "git" -ErrorAction SilentlyContinue
if ($gitCmd) {
    Write-Host "`nCurrent Git Status:" -ForegroundColor DarkCyan
    git status -s
}
