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
if ($env:OS -ne 'Windows_NT' -or -not [Environment]::Is64BitProcess) { throw '請使用 x64 Windows PowerShell 檢查 DLL。' }
if ($ExpectedSha256 -and $ExpectedSha256 -notmatch '^[A-Fa-f0-9]{64}$') { throw 'ExpectedSha256 需為完整 SHA256。' }
$actualHash = Get-EfmSha256 $DllPath
if ($ExpectedSha256 -and $actualHash -ne $ExpectedSha256) { throw "載入前 DLL 雜湊不符。`n預期：$ExpectedSha256`n目前：$actualHash" }
$expected = @(Get-Content -LiteralPath $BaselinePath | ForEach-Object { $_.Trim() } | Where-Object { $_ -and -not $_.StartsWith('#') })
if ($expected.Count -eq 0 -or @($expected | Sort-Object -Unique).Count -ne $expected.Count) { throw '接口基準為空或有重複名稱。' }
foreach ($name in $expected) { if ($name -notmatch '^[A-Za-z_][A-Za-z0-9_]*$') { throw "接口名稱格式錯誤：$name" } }

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
    if ($module -eq [IntPtr]::Zero) { throw "DLL 載入失敗（Win32 錯誤 $([Runtime.InteropServices.Marshal]::GetLastWin32Error())）。" }
    try {
        foreach ($name in $expected) {
            if ([EfmDllLoader]::GetProcAddress($module, $name) -eq [IntPtr]::Zero) { throw "缺少 DLL 接口：$name" }
        }
    }
    finally {
        if (-not [EfmDllLoader]::FreeLibrary($module)) { throw "DLL 卸載失敗（Win32 錯誤 $([Runtime.InteropServices.Marshal]::GetLastWin32Error())）。" }
    }
    [pscustomobject]@{ load = 'passed'; exports = $expected.Count; unload = 'passed'; sha256 = $actualHash; dcs = 'not_run' } | ConvertTo-Json -Compress
    exit 0
}

# A crash or stalled DllMain must fail without hanging the PR runner.
$powershell = Join-Path $env:WINDIR 'System32\WindowsPowerShell\v1.0\powershell.exe'
foreach ($argument in @($PSCommandPath, $DllPath, $BaselinePath)) {
    if ($argument -match '["\r\n\x00]') { throw 'DLL 路徑含無效字元。' }
}
$process = [Diagnostics.Process]::new()
$process.StartInfo = [Diagnostics.ProcessStartInfo]::new()
$process.StartInfo.FileName = $powershell
$process.StartInfo.Arguments = '-NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' + $PSCommandPath + '" -ChildProcess -DllPath "' + $DllPath + '" -BaselinePath "' + $BaselinePath + '" -ExpectedSha256 ' + $actualHash
$process.StartInfo.UseShellExecute = $false
$process.StartInfo.CreateNoWindow = $true
$process.StartInfo.RedirectStandardOutput = $true
$process.StartInfo.RedirectStandardError = $true
$process.StartInfo.StandardOutputEncoding = [Text.UTF8Encoding]::new($false)
$process.StartInfo.StandardErrorEncoding = [Text.UTF8Encoding]::new($false)
$lockedDll = [IO.File]::Open($DllPath, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
try {
    [void]$process.Start()
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        $process.Kill()
        $process.WaitForExit()
        throw "DLL 載入逾時（$TimeoutSeconds 秒）。"
    }
    $outText = $stdout.GetAwaiter().GetResult()
    $errorText = $stderr.GetAwaiter().GetResult()
    if ($process.ExitCode -ne 0) { throw "DLL 檢查失敗（結束碼 $($process.ExitCode)）。`n$errorText`n$outText" }
    $result = $outText | ConvertFrom-Json
    if ($result.sha256 -ne $actualHash -or $result.exports -ne $expected.Count -or $result.load -ne 'passed' -or $result.unload -ne 'passed') { throw 'DLL 檢查結果不符，請查看載入紀錄。' }
    Write-Output $outText.Trim()
}
finally {
    $lockedDll.Dispose()
    $process.Dispose()
}
