<#
. SYNOPSIS
    Build BasicEFM_template.dll and copy it into the mod `bin` folder.

. DESCRIPTION
    This script runs MSBuild (or the msbuild path you provide), performs a
    rebuild for Release/x64, and copies the produced DLL to the mod `bin`
    folder. It prints SHA256 hashes for confirmation.

. EXAMPLE
    .\build_dll.ps1
    .\build_dll.ps1 -MsBuildPath 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe'
#>

param(
    [string]$MsBuildPath = 'MSBuild.exe',
    [string]$Configuration = 'Release',
    [string]$Platform = 'x64'
)

Set-StrictMode -Version Latest

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$solutionDir = Join-Path $repoRoot 'DCS-Basic-EFM-Template-main'
$slnPath = Join-Path $solutionDir 'BasicEFM.sln'

if (-not (Test-Path $slnPath)) {
    Write-Error "Solution not found: $slnPath"
    exit 2
}

Write-Output "Using MSBuild: $MsBuildPath"
Write-Output "Solution: $slnPath"
Write-Output "Configuration: $Configuration | Platform: $Platform"

# Resolve MSBuild path before attempting to run it.
$resolvedMsBuild = $null

if ($MsBuildPath -ne 'MSBuild.exe') {
    if (Test-Path $MsBuildPath) { $resolvedMsBuild = $MsBuildPath }
    else {
        $cmd = Get-Command -Name $MsBuildPath -ErrorAction SilentlyContinue
        if ($cmd) { $resolvedMsBuild = $cmd.Source }
    }
} else {
    $cmd = Get-Command -Name $MsBuildPath -ErrorAction SilentlyContinue
    if ($cmd) { $resolvedMsBuild = $cmd.Source }
}

if (-not $resolvedMsBuild) {
    Write-Output "MSBuild not found on PATH. Searching common Visual Studio locations..."
    $candidates = @(
        'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe',
        'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe',
        'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe',
        'C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe',
        'C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe',
        'C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe'
    )
    foreach ($p in $candidates) { if (Test-Path $p) { $resolvedMsBuild = $p; break } }

    if (-not $resolvedMsBuild) {
        try {
            $found = Get-ChildItem -Path 'C:\Program Files','C:\Program Files (x86)' -Filter 'MSBuild.exe' -Recurse -ErrorAction SilentlyContinue | Sort-Object -Property LastWriteTime -Descending | Select-Object -First 1
            if ($found) { $resolvedMsBuild = $found.FullName }
        } catch { }
    }
}

if (-not $resolvedMsBuild) {
    Write-Error "MSBuild.exe not found. Run this script from a Visual Studio Developer Command Prompt or pass -MsBuildPath with the full path to MSBuild.exe."
    exit 2
} else {
    Write-Output "Resolved MSBuild: $resolvedMsBuild"
    $MsBuildPath = $resolvedMsBuild
}

try {
    & $MsBuildPath $slnPath /t:Rebuild /p:Configuration=$Configuration /p:Platform=$Platform /m /v:m
}
catch {
    Write-Error "MSBuild failed. Command: $MsBuildPath`n$($_.Exception.Message)"
    throw
}

$dllSrc = Join-Path $solutionDir "x64\$Configuration\BasicEFM_template.dll"
$dllDstFolder = Join-Path $repoRoot 'bin'
$dllDst = Join-Path $dllDstFolder 'BasicEFM_template.dll'

if (-not (Test-Path $dllSrc)) {
    Write-Error "Built DLL not found: $dllSrc"
    exit 3
}

if (-not (Test-Path $dllDstFolder)) {
    New-Item -Path $dllDstFolder -ItemType Directory | Out-Null
}

Copy-Item $dllSrc $dllDst -Force

Write-Output "Copied: $dllSrc -> $dllDst"

Write-Output "SRC HASH:"
Get-FileHash $dllSrc -Algorithm SHA256 | Format-List

Write-Output "BIN HASH:"
Get-FileHash $dllDst -Algorithm SHA256 | Format-List

Write-Output "Done."
