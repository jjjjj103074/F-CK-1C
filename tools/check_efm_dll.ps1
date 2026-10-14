<#
.SYNOPSIS
    Load, resolve the recorded exports, and unload a DLL in an isolated x64 process.
    No EFM callback is called, and DCS is not launched.
#>
param([string]$DllPath = '', [string]$BaselinePath = '', [string]$ExpectedSha256 = '',
    [ValidateRange(1, 120)][int]$TimeoutSeconds = 30, [switch]$ChildProcess)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'efm-build-common.ps1')
if ($ChildProcess) { [Console]::OutputEncoding = [Text.UTF8Encoding]::new($false) }
$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $DllPath) { $DllPath = Join-Path $repoRoot 'bin\F-CK-1C_EFM.dll' }
if (-not $BaselinePath) { $BaselinePath = Join-Path $repoRoot 'src\efm\F-CK-1C_EFM\DcsBridge\EfmExports.baseline.txt' }
$DllPath = (Resolve-Path -LiteralPath $DllPath).Path
$BaselinePath = (Resolve-Path -LiteralPath $BaselinePath).Path
if ($env:OS -ne 'Windows_NT' -or -not [Environment]::Is64BitProcess) { throw 'DLL checks require a 64-bit Windows PowerShell process.' }
if ($ExpectedSha256 -and $ExpectedSha256 -notmatch '^[A-Fa-f0-9]{64}$') { throw 'ExpectedSha256 must be a complete SHA256.' }
$actualHash = Get-EfmSha256 $DllPath
if ($ExpectedSha256 -and $actualHash -ne $ExpectedSha256) { throw "DLL changed before loading: expected $ExpectedSha256, found $actualHash." }
$expected = @(Get-Content -LiteralPath $BaselinePath | ForEach-Object { $_.Trim() } | Where-Object { $_ -and -not $_.StartsWith('#') })
if ($expected.Count -eq 0 -or @($expected | Sort-Object -Unique).Count -ne $expected.Count) { throw 'Export baseline must contain unique, nonempty export names.' }
foreach ($name in $expected) { if ($name -notmatch '^[A-Za-z_][A-Za-z0-9_]*$') { throw "Invalid export name: $name" } }

if ($ChildProcess) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class EfmDllLoader
{
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern IntPtr LoadLibraryExW(string path, IntPtr file, uint flags);
    [DllImport("kernel32.dll", CharSet = CharSet.Ansi, ExactSpelling = true, SetLastError = true)]
    public static extern IntPtr GetProcAddress(IntPtr module, string name);
    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool FreeLibrary(IntPtr module);
    [DllImport("kernel32.dll")]
    public static extern uint SetErrorMode(uint mode);
}
'@
    [void][EfmDllLoader]::SetErrorMode(0x8003)
    # Resolve imports and run initialization normally, never resource-only loading.
    $module = [EfmDllLoader]::LoadLibraryExW($DllPath, [IntPtr]::Zero, 0x00000900)
    if ($module -eq [IntPtr]::Zero) { throw "Windows DLL load failed (Win32 error $([Runtime.InteropServices.Marshal]::GetLastWin32Error()))." }
    try {
        foreach ($name in $expected) {
            if ([EfmDllLoader]::GetProcAddress($module, $name) -eq [IntPtr]::Zero) { throw "Missing DLL export: $name" }
        }
    }
    finally {
        if (-not [EfmDllLoader]::FreeLibrary($module)) { throw "Windows DLL unload failed (Win32 error $([Runtime.InteropServices.Marshal]::GetLastWin32Error()))." }
    }
    [pscustomobject]@{ load = 'passed'; exports = $expected.Count; unload = 'passed'; sha256 = $actualHash; dcs = 'not_run' } | ConvertTo-Json -Compress
    exit 0
}

# A crash or stalled DllMain must fail without hanging the PR runner.
$powershell = Join-Path $env:WINDIR 'System32\WindowsPowerShell\v1.0\powershell.exe'
foreach ($argument in @($PSCommandPath, $DllPath, $BaselinePath)) {
    if ($argument -match '["\r\n\x00]') { throw 'Invalid DLL check path.' }
}
$process = [Diagnostics.Process]::new()
$process.StartInfo = [Diagnostics.ProcessStartInfo]::new()
$process.StartInfo.FileName = $powershell
$process.StartInfo.Arguments = '-NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' + $PSCommandPath + '" -ChildProcess -DllPath "' + $DllPath + '" -BaselinePath "' + $BaselinePath + '" -ExpectedSha256 ' + $actualHash
$process.StartInfo.UseShellExecute = $false
$process.StartInfo.CreateNoWindow = $true
$process.StartInfo.RedirectStandardOutput = $true
$process.StartInfo.RedirectStandardError = $true
$lockedDll = [IO.File]::Open($DllPath, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
try {
    [void]$process.Start()
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        $process.Kill()
        $process.WaitForExit()
        throw "DLL load check timed out after $TimeoutSeconds seconds."
    }
    $outText = $stdout.GetAwaiter().GetResult()
    $errorText = $stderr.GetAwaiter().GetResult()
    if ($process.ExitCode -ne 0) { throw "DLL check failed (process exit $($process.ExitCode)):`n$errorText`n$outText" }
    $result = $outText | ConvertFrom-Json
    if ($result.sha256 -ne $actualHash -or $result.exports -ne $expected.Count -or $result.load -ne 'passed' -or $result.unload -ne 'passed') { throw 'Invalid DLL check result.' }
    Write-Output $outText.Trim()
}
finally {
    $lockedDll.Dispose()
    $process.Dispose()
}
