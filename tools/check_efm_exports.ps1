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
        throw 'dumpbin.exe was not found. Run from a Visual Studio Developer shell or pass -DumpbinPath.'
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
        throw "dumpbin failed with exit code $LASTEXITCODE."
    }

    $names = foreach ($line in $output) {
        if ($line -match '^\s+\d+\s+[0-9A-F]+\s+[0-9A-F]+\s+(\S+)') {
            $Matches[1]
        }
    }
    $result = @($names | Sort-Object -Unique)
    if ($result.Count -eq 0) {
        throw "No named exports were found in $TargetDll."
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
    throw "EFM DLL not found: $DllPath"
}
if (-not (Test-Path -LiteralPath $BaselinePath)) {
    throw "Export baseline not found: $BaselinePath"
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
        $meaning = if ($_.SideIndicator -eq '<=') { 'missing' } else { 'unexpected' }
        "$meaning export: $($_.InputObject)"
    }
    throw "EFM export baseline mismatch:`n$($details -join "`n")"
}

Write-Output "EFM export baseline matched: $($actual.Count) names."
Write-Output "DLL: $DllPath"
Write-Output "Baseline: $BaselinePath"
