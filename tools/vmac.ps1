<#
  vmac.ps1 - drive Mini vMac for automated PlusOS testing.

    vmac.ps1 start  [-Image F:\BOOTTEST.hda]   launch emulator on an image
    vmac.ps1 shot   [-Out file.png]           screenshot the emulator window
    vmac.ps1 keys   -Text "HELP{ENTER}"       type into the emulator (SendKeys syntax)
    vmac.ps1 click                            click the middle of the Mac screen
    vmac.ps1 drag   -X 60 -Y 50 -X2 300 -Y2 200 [-Steps 20]   press, move, release
    vmac.ps1 stop                             close the emulator (unlocks the image)
#>
param(
    [Parameter(Position = 0)][ValidateSet('start', 'shot', 'keys', 'click', 'dbl', 'drag', 'stop')][string]$Cmd = 'shot',
    [int]$X2 = -1,
    [int]$Y2 = -1,
    [int]$Steps = 20,
    [string]$Image = 'F:\BOOTTEST.hda',
    [string]$Exe = 'F:\Mini vMac (BlueSCSI patched).exe',
    [string]$Out = "$env:TEMP\plusos_shot.png",
    [string]$Text = '',
    [int]$X = -1,
    [int]$Y = -1,
    [int]$Wait = 0
)
$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Windows.Forms, System.Drawing
if (-not ('PlusOsWin' -as [type])) {
    Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class PlusOsWin {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int n);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern void mouse_event(uint f, int x, int y, uint d, UIntPtr e);
    [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
    [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint code, uint mapType);
    [DllImport("user32.dll")] public static extern short VkKeyScan(char ch);
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
}
"@
}

function Get-VMac {
    Get-Process | Where-Object { $_.ProcessName -like '*vMac*' -and $_.MainWindowHandle -ne 0 } | Select-Object -First 1
}

function Focus-VMac($p) {
    [PlusOsWin]::ShowWindow($p.MainWindowHandle, 9) | Out-Null   # SW_RESTORE
    [PlusOsWin]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
    Start-Sleep -Milliseconds 250
}

switch ($Cmd) {
    'start' {
        Get-Process | Where-Object { $_.ProcessName -like '*vMac*' } | Stop-Process -Force -ErrorAction SilentlyContinue
        Start-Sleep -Milliseconds 300
        $p = Start-Process -FilePath $Exe -ArgumentList "`"$Image`"" -WorkingDirectory (Split-Path $Exe) -PassThru
        for ($i = 0; $i -lt 50 -and $p.MainWindowHandle -eq 0; $i++) { Start-Sleep -Milliseconds 200; $p.Refresh() }
        "started pid=$($p.Id) window=$($p.MainWindowHandle)"
        if ($Wait) { Start-Sleep -Seconds $Wait }
    }
    'shot' {
        if ($Wait) { Start-Sleep -Seconds $Wait }
        $p = Get-VMac; if (-not $p) { throw 'Mini vMac is not running' }
        $r = New-Object PlusOsWin+RECT
        [PlusOsWin]::GetClientRect($p.MainWindowHandle, [ref]$r) | Out-Null
        $w = $r.R - $r.L; $h = $r.B - $r.T
        $bmp = New-Object System.Drawing.Bitmap $w, $h
        $g = [System.Drawing.Graphics]::FromImage($bmp)
        $hdc = $g.GetHdc()
        [PlusOsWin]::PrintWindow($p.MainWindowHandle, $hdc, 3) | Out-Null   # client area, full content
        $g.ReleaseHdc($hdc); $g.Dispose()
        $bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
        "saved $Out (${w}x${h})"
    }
    'keys' {
        # Real held keypresses (down ~90 ms, then up). PlusOS polls the hardware
        # KeyMap, so an instantaneous SendKeys tap would be invisible to it.
        # Text: plain characters, plus {ENTER} {BS} {SPACE} {TAB} {ESC}.
        $p = Get-VMac; if (-not $p) { throw 'Mini vMac is not running' }
        Focus-VMac $p
        $named = @{ '{ENTER}' = 0x0D; '{BS}' = 0x08; '{SPACE}' = 0x20; '{TAB}' = 0x09; '{ESC}' = 0x1B
                    '{LEFT}' = 0x25; '{UP}' = 0x26; '{RIGHT}' = 0x27; '{DOWN}' = 0x28 }
        foreach ($m in [regex]::Matches($Text, '\{[^}]+\}|.')) {
            $tok = $m.Value
            $ext = 0
            if ($tok -in '{LEFT}', '{UP}', '{RIGHT}', '{DOWN}') { $ext = 1 }   # extended keys
            if ($named.ContainsKey($tok)) { $vk = [byte]$named[$tok]; $shift = $false }
            else {
                $s = [PlusOsWin]::VkKeyScan([char]$tok)
                $vk = [byte]($s -band 0xFF); $shift = (($s -shr 8) -band 1) -eq 1
            }
            $sc = [byte][PlusOsWin]::MapVirtualKey($vk, 0)
            if ($shift) { [PlusOsWin]::keybd_event(0x10, 0x2A, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 30 }
            [PlusOsWin]::keybd_event($vk, $sc, $ext, [UIntPtr]::Zero)
            Start-Sleep -Milliseconds 90
            [PlusOsWin]::keybd_event($vk, $sc, 2 -bor $ext, [UIntPtr]::Zero)
            if ($shift) { Start-Sleep -Milliseconds 30; [PlusOsWin]::keybd_event(0x10, 0x2A, 2, [UIntPtr]::Zero) }
            Start-Sleep -Milliseconds 90
        }
        "typed: $Text"
    }
    'click' {
        $p = Get-VMac; if (-not $p) { throw 'Mini vMac is not running' }
        Focus-VMac $p
        $r = New-Object PlusOsWin+RECT
        [PlusOsWin]::GetClientRect($p.MainWindowHandle, [ref]$r) | Out-Null
        $pt = New-Object PlusOsWin+POINT; $pt.X = [int](($r.R - $r.L) / 2); $pt.Y = [int](($r.B - $r.T) / 2)
        if ($X -ge 0) { $pt.X = $X }          # Mac screen coordinates (window is 1:1)
        if ($Y -ge 0) { $pt.Y = $Y }
        [PlusOsWin]::ClientToScreen($p.MainWindowHandle, [ref]$pt) | Out-Null
        [PlusOsWin]::SetCursorPos($pt.X, $pt.Y) | Out-Null
        [PlusOsWin]::mouse_event(2, 0, 0, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 150
        [PlusOsWin]::mouse_event(4, 0, 0, 0, [UIntPtr]::Zero)
        "clicked"
    }
    'dbl' {
        # double-click at (X,Y): two press/release cycles ~120 ms apart, inside
        # one call so they fall within the app's double-click window.
        $p = Get-VMac; if (-not $p) { throw 'Mini vMac is not running' }
        Focus-VMac $p
        $pt = New-Object PlusOsWin+POINT; $pt.X = $X; $pt.Y = $Y
        [PlusOsWin]::ClientToScreen($p.MainWindowHandle, [ref]$pt) | Out-Null
        [PlusOsWin]::SetCursorPos($pt.X, $pt.Y) | Out-Null; Start-Sleep -Milliseconds 80
        [PlusOsWin]::mouse_event(2, 0, 0, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 60
        [PlusOsWin]::mouse_event(4, 0, 0, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 120
        [PlusOsWin]::mouse_event(2, 0, 0, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 60
        [PlusOsWin]::mouse_event(4, 0, 0, 0, [UIntPtr]::Zero)
        "double-clicked"
    }
    'drag' {
        # press at (X,Y), glide to (X2,Y2) in -Steps moves, release - Mac screen coordinates
        $p = Get-VMac; if (-not $p) { throw 'Mini vMac is not running' }
        Focus-VMac $p
        $toScreen = {
            param($mx, $my)
            $pt = New-Object PlusOsWin+POINT; $pt.X = $mx; $pt.Y = $my
            [PlusOsWin]::ClientToScreen($p.MainWindowHandle, [ref]$pt) | Out-Null
            $pt
        }
        $a = & $toScreen $X $Y
        [PlusOsWin]::SetCursorPos($a.X, $a.Y) | Out-Null; Start-Sleep -Milliseconds 250
        [PlusOsWin]::mouse_event(2, 0, 0, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 200
        for ($i = 1; $i -le $Steps; $i++) {
            $mx = [int]($X + ($X2 - $X) * $i / $Steps); $my = [int]($Y + ($Y2 - $Y) * $i / $Steps)
            $b = & $toScreen $mx $my
            [PlusOsWin]::SetCursorPos($b.X, $b.Y) | Out-Null
            Start-Sleep -Milliseconds 40
        }
        Start-Sleep -Milliseconds 200
        [PlusOsWin]::mouse_event(4, 0, 0, 0, [UIntPtr]::Zero)
        "dragged ($X,$Y) -> ($X2,$Y2)"
    }
    'stop' {
        Get-Process | Where-Object { $_.ProcessName -like '*vMac*' } | Stop-Process -Force -ErrorAction SilentlyContinue
        Start-Sleep -Milliseconds 300
        "stopped"
    }
}
