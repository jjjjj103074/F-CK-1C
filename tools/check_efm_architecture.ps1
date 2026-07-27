param(
    [string]$ModuleRoot = ''
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$maximumSourceLines = 700
$sourceExtensions = @('.cpp', '.h')
$findings = [System.Collections.Generic.List[string]]::new()

function Get-RelativeSourcePath {
    param(
        [string]$Root,
        [string]$Path
    )

    return $Path.Substring($Root.Length + 1)
}

function Resolve-LocalInclude {
    param(
        [IO.FileInfo]$Source,
        [string]$Include,
        [string]$Root
    )

    $candidates = @(
        (Join-Path $Source.DirectoryName $Include),
        (Join-Path $Root $Include)
    )
    foreach ($candidate in $candidates) {
        $fullPath = [IO.Path]::GetFullPath($candidate)
        if (Test-Path -LiteralPath $fullPath -PathType Leaf) {
            return $fullPath
        }
    }
    return $null
}

function Add-DependencyFinding {
    param(
        [string]$Source,
        [string]$Target,
        [string]$Rule
    )

    $findings.Add("$Rule`: $Source -> $Target")
}

function Test-Dependency {
    param(
        [string]$Source,
        [string]$Target
    )

    if ($Source -like 'DcsBridge\*' -and
        ($Target -match '^Core\\Systems\\[^\\]+\\' -or
         $Target -like 'Core\Simulation\Models\*')) {
        Add-DependencyFinding $Source $Target 'DcsBridge boundary violation'
    }

    if ($Source -match '^Core\\Systems\\([^\\]+)\\') {
        $sourceOwner = $Matches[1]
        if ($Target -match '^Core\\Systems\\([^\\]+)\\' -and
            $Matches[1] -ne $sourceOwner) {
            Add-DependencyFinding $Source $Target 'Cross-System dependency'
        }
        if ($Target -like 'DcsBridge\*' -or
            $Target -like 'Core\Simulation\Models\*') {
            Add-DependencyFinding $Source $Target 'System boundary violation'
        }
    }

    if ($Source -like 'Core\Simulation\Models\*' -and
        $Target -match '^Core\\Systems\\[^\\]+\\') {
        Add-DependencyFinding $Source $Target 'Simulation Model boundary violation'
    }

    if ($Source -like 'Common\*' -and
        ($Target -like 'Core\Systems\*' -or
         $Target -like 'Core\Simulation\*' -or
         $Target -like 'DcsBridge\*')) {
        Add-DependencyFinding $Source $Target 'Common boundary violation'
    }

    if ($Source -match '^Core\\Fck1cEfm\.(cpp|h)$' -and
        ($Target -match '^Core\\Systems\\[^\\]+\\' -or
         $Target -like 'Core\Simulation\Models\*')) {
        Add-DependencyFinding $Source $Target 'Core facade boundary violation'
    }
}

function Test-Includes {
    param(
        [IO.FileInfo[]]$Sources,
        [string]$Root
    )

    foreach ($source in $Sources) {
        $sourcePath = Get-RelativeSourcePath $Root $source.FullName
        foreach ($line in Get-Content -LiteralPath $source.FullName) {
            if ($line -notmatch '^\s*#\s*include\s*["<]([^">]+)[">]') {
                continue
            }
            $target = Resolve-LocalInclude $source $Matches[1] $Root
            if ($null -eq $target -or
                -not $target.StartsWith(
                    $Root + [IO.Path]::DirectorySeparatorChar,
                    [StringComparison]::OrdinalIgnoreCase)) {
                continue
            }
            $targetPath = Get-RelativeSourcePath $Root $target
            Test-Dependency $sourcePath $targetPath
        }
    }
}

function Test-CoreRoot {
    param([string]$Root)

    $coreRoot = Join-Path $Root 'Core'
    $allowed = @('Fck1cEfm.cpp', 'Fck1cEfm.h')
    foreach ($file in Get-ChildItem -LiteralPath $coreRoot -File) {
        if ($sourceExtensions -contains $file.Extension -and
            $allowed -notcontains $file.Name) {
            $findings.Add("Core root source is not an entry facade: $($file.Name)")
        }
    }
}

function Test-LegacyPaths {
    param([string]$Root)

    foreach ($relativePath in @('Data', 'Systems')) {
        $legacyRoot = Join-Path $Root $relativePath
        if (-not (Test-Path -LiteralPath $legacyRoot -PathType Container)) {
            continue
        }
        foreach ($file in Get-ChildItem -LiteralPath $legacyRoot -File -Recurse) {
            $path = Get-RelativeSourcePath $Root $file.FullName
            $findings.Add("Legacy production path still contains a file: $path")
        }
    }
}

function Test-FileSizes {
    param(
        [IO.FileInfo[]]$Sources,
        [string]$Root
    )

    foreach ($source in $Sources) {
        $relativePath = Get-RelativeSourcePath $Root $source.FullName
        if ($relativePath -like 'include\*') {
            continue
        }
        $lineCount = @(Get-Content -LiteralPath $source.FullName).Count
        if ($lineCount -gt $maximumSourceLines) {
            $findings.Add(
                "Source exceeds $maximumSourceLines lines: " +
                "$relativePath ($lineCount)")
        }
    }
}

if (-not $ModuleRoot) {
    $repositoryRoot = Split-Path -Parent $PSScriptRoot
    $ModuleRoot = Join-Path $repositoryRoot 'src\efm\F-CK-1C_EFM'
}
$resolvedRoot = [IO.Path]::GetFullPath($ModuleRoot.TrimEnd('\', '/'))
if (-not (Test-Path -LiteralPath $resolvedRoot -PathType Container)) {
    throw "EFM module root does not exist: $resolvedRoot"
}

$sources = @(
    Get-ChildItem -LiteralPath $resolvedRoot -Recurse -File |
        Where-Object { $sourceExtensions -contains $_.Extension }
)
Test-CoreRoot $resolvedRoot
Test-LegacyPaths $resolvedRoot
Test-FileSizes $sources $resolvedRoot
Test-Includes $sources $resolvedRoot

if ($findings.Count -ne 0) {
    throw "EFM architecture check failed:`n$($findings -join "`n")"
}
Write-Output "EFM architecture check passed ($($sources.Count) source files)."
