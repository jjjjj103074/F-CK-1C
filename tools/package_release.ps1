<#
.SYNOPSIS
    Create a clean F-CK-1C release package.

.DESCRIPTION
    Copies only runtime files and small user-facing documents into dist/F-CK-1C,
    then creates dist/F-CK-1C-<Version>.zip and a SHA256 file.

.EXAMPLE
    .\tools\package_release.ps1 -Version v0.1.0

.EXAMPLE
    .\tools\package_release.ps1 -Version v0.1.0-test -AllowDirty
#>

param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[A-Za-z0-9._-]+$')]
    [string]$Version,

    [switch]$AllowDirty
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$ModuleName = 'F-CK-1C'
$DistDirName = 'dist'
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir
$repoRootFull = [System.IO.Path]::GetFullPath($repoRoot)
$distRoot = Join-Path $repoRootFull $DistDirName
$packageRoot = Join-Path $distRoot $ModuleName
$zipPath = Join-Path $distRoot "$ModuleName-$Version.zip"
$hashPath = "$zipPath.sha256"

$requiredFiles = @(
    'entry.lua',
    'F-CK-1C.lua',
    'comm.lua',
    'Views.lua',
    'install.bat',
    'uninstall.bat',
    'LICENSE',
    'README.md',
    'README.zh-TW.md'
)

$optionalFiles = @(
    'F-CK-1C.png'
)

$requiredRuntimeFiles = @(
    'bin\F-CK-1C_EFM.dll'
)

$requiredDirs = @(
    'bin',
    'Cockpit',
    'FM',
    'Input',
    'Liveries',
    'Shapes',
    'Sounds',
    'Textures'
)

$optionalDirs = @(
    'Missions',
    'Options'
)

$forbiddenRootEntries = @(
    '.git',
    '.github',
    '.vscode',
    'docs',
    'node_modules',
    'src',
    'tools'
)

$forbiddenFilePattern = '\.(cpp|h|hpp|sln|vcxproj|filters|user|obj|pdb|ipdb|iobj|pch|exp|lib|ilk|tlog|lastbuildstate|ps1)$|^package(-lock)?\.json$'

function Assert-UnderRoot {
    param(
        [string]$Path,
        [string]$Root
    )

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    $fullRoot = [System.IO.Path]::GetFullPath($Root)
    $rootPrefix = $fullRoot.TrimEnd('\') + '\'

    if (-not $fullPath.StartsWith($rootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to modify path outside repository: $fullPath"
    }
}

function Remove-ExistingPath {
    param(
        [string]$Path
    )

    if (-not (Test-Path -LiteralPath $Path)) { return }
    Assert-UnderRoot -Path $Path -Root $repoRootFull
    Remove-Item -LiteralPath $Path -Recurse -Force
}

function Assert-GitWorkingTreeClean {
    if ($AllowDirty) { return }

    try {
        $gitStatus = & git -C $repoRootFull status --porcelain --untracked-files=all
        $gitExitCode = $LASTEXITCODE
    }
    catch {
        throw "Unable to check git working tree status: $($_.Exception.Message)"
    }

    if ($gitExitCode -ne 0) {
        throw "Unable to check git working tree status. Git exited with code $gitExitCode."
    }

    if ($gitStatus) {
        $details = ($gitStatus | ForEach-Object { $_ }) -join "`n"
        throw "Working tree has uncommitted changes. Commit or stash them before release packaging, or rerun with -AllowDirty for a local test package.`n$details"
    }
}

function Assert-RequiredRuntimeFiles {
    foreach ($file in $requiredRuntimeFiles) {
        $path = Join-Path $repoRootFull $file
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Missing required runtime file: $file"
        }
    }
}

function Copy-RequiredFile {
    param(
        [string]$RelativePath
    )

    $source = Join-Path $repoRootFull $RelativePath
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
        throw "Missing required file: $RelativePath"
    }

    $destination = Join-Path $packageRoot $RelativePath
    New-Item -Path (Split-Path -Parent $destination) -ItemType Directory -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination -Force
}

function Copy-OptionalFile {
    param(
        [string]$RelativePath
    )

    $source = Join-Path $repoRootFull $RelativePath
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) { return }
    $destination = Join-Path $packageRoot $RelativePath
    New-Item -Path (Split-Path -Parent $destination) -ItemType Directory -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination -Force
}

function Copy-RequiredDirectory {
    param(
        [string]$RelativePath
    )

    $source = Join-Path $repoRootFull $RelativePath
    if (-not (Test-Path -LiteralPath $source -PathType Container)) {
        throw "Missing required directory: $RelativePath"
    }

    $destination = Join-Path $packageRoot $RelativePath
    New-Item -Path (Split-Path -Parent $destination) -ItemType Directory -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination -Recurse -Force
}

function Copy-OptionalDirectory {
    param(
        [string]$RelativePath
    )

    $source = Join-Path $repoRootFull $RelativePath
    if (-not (Test-Path -LiteralPath $source -PathType Container)) { return }
    $destination = Join-Path $packageRoot $RelativePath
    New-Item -Path (Split-Path -Parent $destination) -ItemType Directory -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination -Recurse -Force
}

function Assert-CleanPackage {
    foreach ($entry in $forbiddenRootEntries) {
        $path = Join-Path $packageRoot $entry
        if (Test-Path -LiteralPath $path) {
            throw "Forbidden release package entry found: $entry"
        }
    }

    $forbiddenFiles = Get-ChildItem -LiteralPath $packageRoot -Recurse -File |
        Where-Object { $_.Name -match $forbiddenFilePattern }
    if ($forbiddenFiles) {
        $names = ($forbiddenFiles | ForEach-Object { $_.FullName.Replace($packageRoot, $ModuleName) }) -join "`n"
        throw "Forbidden development file(s) found in package:`n$names"
    }
}

function Assert-TexturesAreArchived {
    $textureRoot = Join-Path $packageRoot 'Textures'
    $looseTextures = Get-ChildItem -LiteralPath $textureRoot -Recurse -File |
        Where-Object { $_.Extension -ne '.zip' }
    if ($looseTextures) {
        $names = ($looseTextures | ForEach-Object { $_.FullName.Replace($packageRoot, $ModuleName) }) -join "`n"
        throw "Textures must stay inside zip archives. Loose file(s):`n$names"
    }
}

function Write-Sha256File {
    $hash = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash
    $zipName = Split-Path -Leaf $zipPath
    Set-Content -Path $hashPath -Value "$hash *$zipName" -Encoding ASCII
}

Assert-GitWorkingTreeClean
Assert-RequiredRuntimeFiles

New-Item -Path $distRoot -ItemType Directory -Force | Out-Null
Remove-ExistingPath -Path $packageRoot
Remove-ExistingPath -Path $zipPath
Remove-ExistingPath -Path $hashPath
New-Item -Path $packageRoot -ItemType Directory -Force | Out-Null

foreach ($file in $requiredFiles) { Copy-RequiredFile -RelativePath $file }
foreach ($file in $optionalFiles) { Copy-OptionalFile -RelativePath $file }
foreach ($dir in $requiredDirs) { Copy-RequiredDirectory -RelativePath $dir }
foreach ($dir in $optionalDirs) { Copy-OptionalDirectory -RelativePath $dir }

Assert-CleanPackage
Assert-TexturesAreArchived

Compress-Archive -Path $packageRoot -DestinationPath $zipPath -CompressionLevel Optimal
Write-Sha256File

Write-Output "Release package created:"
Write-Output "  $zipPath"
Write-Output "  $hashPath"
