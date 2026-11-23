@echo off
setlocal EnableExtensions
powershell -NoProfile -ExecutionPolicy Bypass -Command "& { $root = '%~dp0'; try { $t = Get-Content -Raw -LiteralPath '%~f0'; $m = '#===PS===#'; $i = $t.LastIndexOf($m); if ($i -lt 0) { throw 'marker not found' }; Invoke-Expression $t.Substring($i + $m.Length); exit 0 } catch { Remove-Item -Recurse -Force (Join-Path $env:TEMP 'rdr2_mingw_build') -ErrorAction SilentlyContinue; Write-Host ''; Write-Host ('[ERROR] ' + $_.Exception.Message); exit 1 } }"
set "RC=%ERRORLEVEL%"
if not "%RC%"=="0" (
  echo.
  echo [ERROR] Build failed - see the messages above.
  pause
  exit /b %RC%
)
echo.
echo The Final Countdown 3...
ping -n 2 127.0.0.1 >nul 2>&1
echo The Final Countdown 2...
ping -n 2 127.0.0.1 >nul 2>&1
echo The Final Countdown 1...
ping -n 2 127.0.0.1 >nul 2>&1
exit /b 0
#===PS===#
Set-Location -LiteralPath $root
$ErrorActionPreference = 'Continue'

# Maximize the console window (SW_MAXIMIZE = 3)
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
[StructLayout(LayoutKind.Sequential)]
public struct CursorInfo {
    public uint dwSize;
    [MarshalAs(UnmanagedType.Bool)] public bool bVisible;
}
public static class WinConsole {
    [DllImport("kernel32.dll")] public static extern IntPtr GetConsoleWindow();
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    [DllImport("kernel32.dll")] public static extern IntPtr GetStdHandle(int nStdHandle);
    [DllImport("kernel32.dll")] public static extern bool GetConsoleCursorInfo(IntPtr h, out CursorInfo ci);
    [DllImport("kernel32.dll")] public static extern bool SetConsoleCursorInfo(IntPtr h, ref CursorInfo ci);
}
'@
[WinConsole]::ShowWindow([WinConsole]::GetConsoleWindow(), 3) | Out-Null

# Hide the blinking block cursor while the spinner is running.
$hOut = [WinConsole]::GetStdHandle(-11)
$cursor = New-Object CursorInfo
$cursorOk = [WinConsole]::GetConsoleCursorInfo($hOut, [ref]$cursor)
if ($cursorOk) {
    $cursor.bVisible = $false
    [WinConsole]::SetConsoleCursorInfo($hOut, [ref]$cursor) | Out-Null
}

$kitUrl = 'https://github.com/skeeto/w64devkit/releases'
$kitDir = Join-Path $root 'w64devkit'

Write-Host '================================='
Write-Host ' Dreamaker-Mod - Build MinGW-w64 '
Write-Host '================================='
Write-Host ''

if (-not (Test-Path -LiteralPath $kitDir)) {
    Write-Host 'w64devkit folder was not found next to this .bat file.'
    Write-Host "Opening: $kitUrl"
    try { Start-Process $kitUrl } catch { Write-Host 'Could not open the browser. Download it manually from the URL above.' }
    throw 'Download w64devkit x64, extract it next to this .bat file, then run again.'
}

$cxx = Join-Path $kitDir 'bin\g++.exe'
if (-not (Test-Path -LiteralPath $cxx)) {
    Write-Host 'w64devkit folder exists, but bin\g++.exe was not found.'
    Write-Host "Opening: $kitUrl"
    try { Start-Process $kitUrl } catch { Write-Host 'Could not open the browser. Download it manually from the URL above.' }
    throw 'Re-extract w64devkit x64 next to this .bat file, then run again.'
}

$bin = Split-Path -Parent $cxx
$machine = (& $cxx -dumpmachine | Out-String).Trim()
Write-Host "Compiler: $cxx"
Write-Host "Target:   $machine"
if ($machine -notmatch 'mingw|w64') { throw "This g++ is not MinGW-w64 (target: $machine)" }
foreach ($tool in @('nm.exe', 'objcopy.exe', 'dlltool.exe', 'c++filt.exe')) {
    if (-not (Test-Path -LiteralPath (Join-Path $bin $tool))) { throw "Missing tool: $bin\$tool" }
}

$work = Join-Path $env:TEMP 'rdr2_mingw_build'
Remove-Item -Recurse -Force $work -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $work | Out-Null
$src = Join-Path $work 'src'
Remove-Item -Recurse -Force $src -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $src | Out-Null

Write-Host ''
Write-Host -NoNewline 'Compiling... '
Copy-Item (Join-Path $root 'script.cpp') $src -Force
Copy-Item (Join-Path $root 'script.h') $src -Force
Copy-Item (Join-Path $root 'inc.h') $src -Force
Copy-Item (Join-Path $root 'inc') (Join-Path $src 'inc') -Recurse -Force

$enc = [Text.Encoding]::GetEncoding(1252)

# GCC treats __try as a keyword and does not support SEH in C++.
$cpp = Join-Path $src 'script.cpp'
$t = $enc.GetString([IO.File]::ReadAllBytes($cpp))
$t = [Text.RegularExpressions.Regex]::Replace($t, '\b__except\b', 'MOD_EXCEPT')
$t = [Text.RegularExpressions.Regex]::Replace($t, '\b__try\b', 'MOD_TRY')
[IO.File]::WriteAllBytes($cpp, $enc.GetBytes($t))

# GCC ignores __declspec(align) on members; alignas(8) keeps sizeof(Vector3) == 24.
$types = Join-Path $src 'inc\types.h'
if (-not (Test-Path -LiteralPath $types)) { throw 'inc/types.h not found.' }
$t = $enc.GetString([IO.File]::ReadAllBytes($types))
$old = '#define ALIGN8 __declspec(align(8))'
if (-not $t.Contains($old)) { throw 'ALIGN8 pattern not found in inc/types.h' }
$t = $t.Replace($old, '#define ALIGN8 alignas(8)')
[IO.File]::WriteAllBytes($types, $enc.GetBytes($t))

$compat = Join-Path $work 'compat.h'
$compatText = @'
#pragma once
#include <cmath>
#include <cstdint>
#define MOD_TRY if (true)
#define MOD_EXCEPT(x) if (false)
'@
[IO.File]::WriteAllText($compat, $compatText, [Text.Encoding]::ASCII)

$obj = Join-Path $work 'script.o'
$compileArgs = '-std=gnu++14 -include "{0}" -c "{1}" -I "{2}" -O3 -Wno-write-strings -DWIN32 -DNDEBUG -Dmain=mod_main -o "{3}"' -f $compat, (Join-Path $src 'script.cpp'), (Join-Path $src 'inc'), $obj
$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $cxx
$psi.Arguments = $compileArgs
$psi.UseShellExecute = $false
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
$psi.CreateNoWindow = $true
$proc = New-Object System.Diagnostics.Process
$proc.StartInfo = $psi
$proc.Start() | Out-Null

# Spinner while the compiler runs
$spinner = @('|', '/', '-', '\')
$i = 0
while (-not $proc.HasExited) {
    Write-Host -NoNewline ("`rCompiling... " + $spinner[$i % 4])
    Start-Sleep -Milliseconds 120
    $i++
}
$proc.WaitForExit() | Out-Null
$code = $proc.ExitCode
$stdout = $proc.StandardOutput.ReadToEnd()
$stderr = $proc.StandardError.ReadToEnd()
$out = @(($stdout + $stderr) -split "`r?`n" | Where-Object { $_.Trim() })
Write-Host -NoNewline "`r                    `r"
if ($cursorOk) {
    $cursor.bVisible = $true
    [WinConsole]::SetConsoleCursorInfo($hOut, [ref]$cursor) | Out-Null
}
Write-Host ''
if ($code -ne 0) { $out | ForEach-Object { Write-Host $_ }; throw 'Compilation failed.' }
$warn = @($out | Where-Object { "$_" -match 'warning:' }).Count
if ($warn -gt 0) { Write-Host "  $warn warning(s)" }

$nm      = Join-Path $bin 'nm.exe'
$objcopy = Join-Path $bin 'objcopy.exe'
$dlltool = Join-Path $bin 'dlltool.exe'
$cxxfilt = Join-Path $bin 'c++filt.exe'

$undef = @(& $nm -u $obj | ForEach-Object { ($_ -split '\s+')[-1] } | Where-Object { $_ -match '^(__imp_)?_Z' } | Sort-Object -Unique)
if ($undef.Count -eq 0) { throw 'No SDK symbols found in the object file.' }

$baseName = @{}
foreach ($g in $undef) {
    $mangled = $g -replace '^__imp_', ''
    $pretty = & $cxxfilt $mangled
    $name = ($pretty -split '\(')[0].Trim()
    if ($name -match '::') { $name = ($name -split '::')[-1] }
    $baseName[$g] = $name
}

$lib = Join-Path $root 'lib\ScriptHookRDR2.lib'
$exports = @(& $nm -g --defined-only $lib | ForEach-Object { ($_ -split '\s+')[-1] } | Where-Object { $_ -like '?*' -and $_ -notlike '__imp_*' } | Sort-Object -Unique)
$msvcName = @{}
foreach ($e in $exports) {
    $name = $e.Substring(1)
    $at = $name.IndexOf('@@')
    if ($at -gt 0) { $name = $name.Substring(0, $at) }
    $msvcName[$name] = $e
}

$redef = @()
$def = @('EXPORTS')
foreach ($g in $undef) {
    $base = $baseName[$g]
    if ($msvcName.ContainsKey($base)) {
        $m = $msvcName[$base]
        if ($g -like '__imp_*') { $target = '__imp_' + $m } else { $target = $m }
        $redef += "--redefine-sym $g=$target"
        $def += "  `"$m`""
    }
}
if ($redef.Count -eq 0) { throw 'No SDK symbols were mapped.' }

$redefPath = Join-Path $work 'redef.rsp'
$defPath   = Join-Path $work 'msvc.def'
$impLib    = Join-Path $work 'libScriptHookRDR2_mingw.a'
$msvcObj   = Join-Path $work 'script_msvc.o'
[IO.File]::WriteAllLines($redefPath, $redef, [Text.Encoding]::ASCII)
[IO.File]::WriteAllLines($defPath, $def, [Text.Encoding]::ASCII)

& $dlltool -d $defPath -D ScriptHookRDR2.dll -l $impLib
if ($LASTEXITCODE -ne 0) { throw 'dlltool failed.' }
& $objcopy "@$redefPath" $obj $msvcObj
if ($LASTEXITCODE -ne 0) { throw 'objcopy failed.' }

$asi = Join-Path $root 'Dreamaker-Mod.asi'
$out = & $cxx '-shared' '-O3' '-static-libgcc' '-static-libstdc++' '-Wl,--enable-auto-import' $msvcObj "-L$work" '-lScriptHookRDR2_mingw' '-lpsapi' '-s' '-o' $asi 2>&1
if ($LASTEXITCODE -ne 0) { $out | ForEach-Object { Write-Host $_ }; throw 'Link failed.' }

Write-Host ''
Write-Host ''
Write-Host "[OK] $asi"
Remove-Item -Recurse -Force $work -ErrorAction SilentlyContinue
