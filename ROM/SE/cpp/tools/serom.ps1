<#
.SYNOPSIS
    PlusOS Macintosh SE ROM Management & White-Box Analysis CLI Tool
.DESCRIPTION
    Wraps serom_mgr.py with color output and convenient presets.
.EXAMPLE
    .\serom.ps1 status
    .\serom.ps1 diff boot/boot_search
    .\serom.ps1 checksum
    .\serom.ps1 gcr
    .\serom.ps1 pram
#>

[CmdletBinding()]
param (
    [Parameter(Position=0)]
    [string]$Command = "status",

    [Parameter(Position=1, ValueFromRemainingArguments=$true)]
    [string[]]$Arguments
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$pythonScript = Join-Path $scriptDir "serom_mgr.py"

if (-not (Test-Path $pythonScript)) {
    Write-Error "Could not find serom_mgr.py at $pythonScript"
    exit 1
}

$pyCmd = "python"
if (-not (Get-Command $pyCmd -ErrorAction SilentlyContinue)) {
    Write-Error "Python executable not found in PATH."
    exit 1
}

& $pyCmd $pythonScript $Command @Arguments
