<#
. SYNOPSIS
    Build F-CK-1C_EFM.dll and copy it into the mod `bin` folder.

. DESCRIPTION
    This script runs MSBuild (or the msbuild path you provide), performs a
    rebuild for the only supported target, Release/x64, and copies the produced DLL to the mod `bin`
    folder. It prints SHA256 hashes for confirmation.

. EXAMPLE
    .\tools\build_dll.ps1
    .\tools\build_dll.ps1 -MsBuildPath '<path-to-MSBuild.exe>'
#>

param(
    [string]$MsBuildPath = 'MSBuild.exe'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$Configuration = 'Release'
$Platform = 'x64'

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir
$solutionDir = Join-Path $repoRoot 'src\efm'
$projectPath = Join-Path $solutionDir 'F-CK-1C_EFM\F-CK-1C_EFM.vcxproj'

if (-not (Test-Path $projectPath)) {
    Write-Error "EFM project not found: $projectPath"
    exit 2
}

Write-Output "Using MSBuild: $MsBuildPath"
Write-Output "Project: $projectPath"
Write-Output "Configuration: $Configuration | Platform: $Platform"
Write-Output "Only Release | x64 is supported for the F-CK-1C EFM DLL."

# Resolve MSBuild path before attempting to run it.
$resolvedMsBuild = $null

if ($MsBuildPath -ne 'MSBuild.exe') {
    if (Test-Path $MsBuildPath) { $resolvedMsBuild = $MsBuildPath }
    else {
        $cmd = Get-Command -Name $MsBuildPath -ErrorAction SilentlyContinue
        if ($cmd) { $resolvedMsBuild = $cmd.Source }
    }
}
else {
    $cmd = Get-Command -Name $MsBuildPath -ErrorAction SilentlyContinue
    if ($cmd) { $resolvedMsBuild = $cmd.Source }
}

if (-not $resolvedMsBuild) {
    Write-Output "MSBuild not found on PATH. Searching common Visual Studio locations..."
    $programFilesRoots = @(@($env:ProgramFiles, ${env:ProgramFiles(x86)}) |
        Where-Object { $_ -and (Test-Path $_) } |
        Select-Object -Unique)
    $vsYears = @('2022', '2019')
    $vsEditions = @('Community', 'Professional', 'Enterprise', 'BuildTools')
    $candidates = @()
    foreach ($root in $programFilesRoots) {
        foreach ($year in $vsYears) {
            foreach ($edition in $vsEditions) {
                $candidates += Join-Path $root "Microsoft Visual Studio\$year\$edition\MSBuild\Current\Bin\MSBuild.exe"
                $candidates += Join-Path $root "Microsoft Visual Studio\$year\$edition\MSBuild\Current\Bin\amd64\MSBuild.exe"
            }
        }
    }
    foreach ($p in $candidates) { if (Test-Path $p) { $resolvedMsBuild = $p; break } }

    if (-not $resolvedMsBuild -and $programFilesRoots.Count -gt 0) {
        try {
            $found = Get-ChildItem -Path $programFilesRoots -Filter 'MSBuild.exe' -Recurse -ErrorAction SilentlyContinue | Sort-Object -Property LastWriteTime -Descending | Select-Object -First 1
            if ($found) { $resolvedMsBuild = $found.FullName }
        }
        catch { }
    }
}

if (-not $resolvedMsBuild) {
    Write-Error "MSBuild.exe not found. Run this script from a Visual Studio Developer Command Prompt or pass -MsBuildPath with the full path to MSBuild.exe."
    exit 2
}
else {
    Write-Output "Resolved MSBuild: $resolvedMsBuild"
    $MsBuildPath = $resolvedMsBuild
}

try {
    & $MsBuildPath $projectPath /t:Rebuild /p:Configuration=$Configuration /p:Platform=$Platform /m /v:m
    $msBuildExitCode = $LASTEXITCODE
}
catch {
    Write-Error "MSBuild failed. Command: $MsBuildPath`n$($_.Exception.Message)"
    exit 2
}

if ($msBuildExitCode -ne 0) {
    Write-Error "MSBuild failed with exit code $msBuildExitCode. Command: $MsBuildPath $projectPath /t:Rebuild /p:Configuration=$Configuration /p:Platform=$Platform" -ErrorAction Continue
    exit $msBuildExitCode
}

$dllSrc = Join-Path $solutionDir "x64\$Configuration\F-CK-1C_EFM.dll"
$dllDstFolder = Join-Path $repoRoot 'bin'
$dllDst = Join-Path $dllDstFolder 'F-CK-1C_EFM.dll'

if (-not (Test-Path $dllSrc)) {
    Write-Error "Built DLL not found: $dllSrc"
    exit 3
}

if (-not (Test-Path $dllDstFolder)) {
    New-Item -Path $dllDstFolder -ItemType Directory | Out-Null
}

try {
    Copy-Item $dllSrc $dllDst -Force -ErrorAction Stop
}
catch {
    Write-Error "Failed to copy built DLL to runtime bin folder: $($_.Exception.Message)"
    exit 4
}

Write-Output "Copied: $dllSrc -> $dllDst"

$sourceHash = (Get-FileHash $dllSrc -Algorithm SHA256).Hash
$runtimeHash = (Get-FileHash $dllDst -Algorithm SHA256).Hash
Write-Output "SRC HASH: $sourceHash"
Write-Output "BIN HASH: $runtimeHash"

if ($sourceHash -ne $runtimeHash) {
    Write-Error "Runtime DLL hash does not match the build output."
    exit 5
}

Write-Output "Done."
