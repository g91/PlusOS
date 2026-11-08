# showcase.ps1 - automated screenshot tour of PlusOS for the README / gallery.
#   powershell -ExecutionPolicy Bypass -File tools\showcase.ps1
# Boots PlusOS in Mini vMac, drives the base programs, and saves PNGs to
# docs\screenshots. Networking is shown via its command surface (the live
# online session only runs on real hardware; Mini vMac has no DaynaPORT).
param([string]$Out = "$PSScriptRoot\..\docs\screenshots")

$ErrorActionPreference = 'Stop'
$vmac = Join-Path $PSScriptRoot 'vmac.ps1'
New-Item -ItemType Directory -Force -Path $Out | Out-Null
$script:n = 0

function VM { & $vmac @args | Out-Null }
function Boot {
    VM start; Start-Sleep -Seconds 16
    # prime: settle keyboard focus (the first post-boot keystroke is sometimes lost)
    for ($i = 0; $i -lt 3; $i++) { VM keys -Text '{ENTER}'; Start-Sleep -Milliseconds 300 }
}
function Shot([string]$label) {
    $script:n++
    $file = Join-Path $Out ("{0:D2}_{1}.png" -f $script:n, $label)
    VM shot -Out $file
    Write-Output ("shot {0}" -f (Split-Path $file -Leaf))
}
# a command that prints and returns to the shell on its own
function Cmd([string]$cmd, [string]$label, [int]$wait = 3) {
    VM keys -Text 'clear{ENTER}'; Start-Sleep -Milliseconds 700
    VM keys -Text $cmd; Start-Sleep -Milliseconds 300
    VM keys -Text '{ENTER}'; Start-Sleep -Seconds $wait
    Shot $label
}
# launch an interactive program, screenshot it, then send quit keys
function App([string]$cmd, [string]$label, [string[]]$quit, [int]$wait = 4) {
    VM keys -Text 'clear{ENTER}'; Start-Sleep -Milliseconds 700
    VM keys -Text $cmd; Start-Sleep -Milliseconds 300
    VM keys -Text '{ENTER}'; Start-Sleep -Seconds $wait
    Shot $label
    foreach ($k in $quit) { VM keys -Text $k; Start-Sleep -Milliseconds 800 }
    Start-Sleep -Seconds 1
}

Write-Output '=== PlusOS showcase: booting (shell phase) ==='
Boot
Cmd 'sysinfo' 'sysinfo'
Cmd 'ver'     'version'
Cmd 'mem'     'memory'
Cmd 'ps'      'processes'
Cmd 'date'    'date'
Cmd 'ls'      'files-ls'
Cmd 'reg'     'registry'
Cmd 'help'    'help-all-commands' 3
App 'taskman' 'task-manager-text' @('{ESC}') 3   # interactive text mode from the shell

Write-Output '=== networking command surface ==='
Cmd 'net'              'net-interface' 4
Cmd 'ping google.com' 'ping' 4
Cmd 'telnet'          'telnet-usage' 2
Cmd 'irc'             'irc-usage' 2

Write-Output '=== interactive base programs ==='
App 'calc'    'calculator'  @('{ESC}') 3
App 'notes'   'notepad'     @('{ESC}') 3
App 'puzzle'  '15-puzzle'   @('{ESC}') 3
App 'hexedit' 'hex-editor'  @('{ESC}','q') 3
App 'sound'   'sound-synth' @('{ESC}') 3

Write-Output '=== 3D engine (fixed-point) ==='
VM keys -Text 'clear{ENTER}'; Start-Sleep -Milliseconds 700
VM keys -Text 'cube3d'; Start-Sleep -Milliseconds 300
VM keys -Text '{ENTER}'; Start-Sleep -Seconds 10
Shot 'cube3d-solid'
VM keys -Text 'f'; Start-Sleep -Seconds 10        # wireframe (fast mode)
Shot 'cube3d-wireframe'
VM keys -Text 'q'; Start-Sleep -Seconds 2

Write-Output '=== desktop phase: cold reboot for a clean desktop ==='
VM stop; Start-Sleep -Seconds 2
Boot
VM keys -Text 'desktop'; Start-Sleep -Milliseconds 300
VM keys -Text '{ENTER}'; Start-Sleep -Seconds 8
Shot 'desktop'
# open the Programs window (double-click the Programs icon, top-right)
VM click -X 471 -Y 40; Start-Sleep -Milliseconds 120
VM click -X 471 -Y 40; Start-Sleep -Seconds 4
Shot 'desktop-programs'
# Files explorer (double-click the Files/disk icon)
VM click -X 471 -Y 136; Start-Sleep -Milliseconds 120
VM click -X 471 -Y 136; Start-Sleep -Seconds 5
Shot 'desktop-files'
# Task Manager GUI (double-click the Tasks icon)
VM click -X 471 -Y 232; Start-Sleep -Milliseconds 120
VM click -X 471 -Y 232; Start-Sleep -Seconds 5
Shot 'desktop-taskman-gui'

VM stop
Write-Output ("=== done: {0} screenshots in {1} ===" -f $script:n, (Resolve-Path $Out))
