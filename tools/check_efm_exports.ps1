<#
.SYNOPSIS
    Verify that the EFM DLL exports the recorded DCS ABI names.

.EXAMPLE
    .\tools\check_efm_exports.ps1
    .\tools\check_efm_exports.ps1 -DumpbinPath '<path-to-dumpbin.exe>'
#>

param(
    [string]$DllPath = '',
    [string]$BaselinePath = '',
    [string]$DumpbinPath = 'dumpbin.exe'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Find-Dumpbin {
    param([string]$RequestedPath)

    if ($RequestedPath -ne 'dumpbin.exe' -and (Test-Path -LiteralPath $RequestedPath)) {
        return (Resolve-Path -LiteralPath $RequestedPath).Path
    }

    $command = Get-Command -Name $RequestedPath -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    $roots = @($env:ProgramFiles, ${env:ProgramFiles(x86)}) |
        Where-Object { $_ -and (Test-Path -LiteralPath $_) } |
        Select-Object -Unique
    $candidates = foreach ($root in $roots) {
        $visualStudioRoot = Join-Path $root 'Microsoft Visual Studio'
        if (Test-Path -LiteralPath $visualStudioRoot) {
            Get-ChildItem -Path $visualStudioRoot -Filter 'dumpbin.exe' -Recurse `
                -ErrorAction SilentlyContinue
        }
    }
    $match = $candidates |
        Where-Object { $_.FullName -match '\\Host[xX]64\\x64\\dumpbin\.exe$' } |
        Sort-Object -Property LastWriteTime -Descending |
        Select-Object -First 1
    if (-not $match) {
        throw '找不到 dumpbin.exe，請指定 -DumpbinPath。'
    }
    return $match.FullName
}

function Read-ExportNames {
    param(
        [string]$ToolPath,
        [string]$TargetDll
    )

    $output = & $ToolPath /nologo /exports $TargetDll
    if ($LASTEXITCODE -ne 0) {
        throw "接口讀取失敗（結束碼 $LASTEXITCODE）。"
    }

    $names = foreach ($line in $output) {
        if ($line -match '^\s+\d+\s+[0-9A-F]+\s+[0-9A-F]+\s+(\S+)') {
            $Matches[1]
        }
    }
    $result = @($names | Sort-Object -Unique)
    if ($result.Count -eq 0) {
        throw "DLL 沒有具名接口：$TargetDll"
    }
    return $result
}

$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $DllPath) {
    $DllPath = Join-Path $repoRoot 'bin\F-CK-1C_EFM.dll'
}
if (-not $BaselinePath) {
    $BaselinePath = Join-Path $repoRoot `
        'src\efm\F-CK-1C_EFM\DcsBridge\EfmExports.baseline.txt'
}
if (-not (Test-Path -LiteralPath $DllPath)) {
    throw "找不到 DLL：$DllPath"
}
if (-not (Test-Path -LiteralPath $BaselinePath)) {
    throw "找不到接口基準：$BaselinePath"
}

$resolvedDumpbin = Find-Dumpbin $DumpbinPath
$actual = Read-ExportNames $resolvedDumpbin $DllPath
$expected = @(
    Get-Content -LiteralPath $BaselinePath |
        ForEach-Object { $_.Trim() } |
        Where-Object { $_ -and -not $_.StartsWith('#') } |
        Sort-Object -Unique
)
$differences = @(Compare-Object -ReferenceObject $expected -DifferenceObject $actual)
if ($differences.Count -ne 0) {
    $details = $differences | ForEach-Object {
        $meaning = if ($_.SideIndicator -eq '<=') { '缺少' } else { '多出' }
        "${meaning}接口：$($_.InputObject)"
    }
    throw "DLL 接口不符，請核對接口基準。`n$($details -join "`n")"
}

Write-Output "DLL 接口一致：$($actual.Count) 個。"
