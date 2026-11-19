param()

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$generator = Join-Path $PSScriptRoot "generate_system_catalog.ps1"
$temporaryRoot = [IO.Path]::GetFullPath(
    (Join-Path ([IO.Path]::GetTempPath()) ("fck1c_catalog_" + [Guid]::NewGuid())))
$systemsRoot = Join-Path $temporaryRoot "Systems"
$outputPath = Join-Path $temporaryRoot "Generated\SystemCatalog.g.cpp"
$emptyCatalogCount = 0
$singleEntryCount = 1
$twoEntryCount = 2
$noPreviousEntryIndex = -1
$fixedTimestampText = "2000-01-01T00:00:00"

function Invoke-Generator {
    & $generator -SystemsRoot $systemsRoot -OutputPath $outputPath
}

function Add-FixtureEntry {
    param([string]$Name)

    $directory = Join-Path $systemsRoot $Name
    [IO.Directory]::CreateDirectory($directory) | Out-Null
    [IO.File]::WriteAllText(
        (Join-Path $directory "Entry.cpp"),
        "// fixture`r`n",
        [Text.UTF8Encoding]::new($false))
}

function Assert-Catalog {
    param(
        [int]$Count,
        [string[]]$OrderedNames
    )

    $content = [IO.File]::ReadAllText($outputPath)
    if ($content -notmatch "// Generated system entries: $Count") {
        throw "Expected $Count generated entries."
    }

    $lastIndex = $noPreviousEntryIndex
    foreach ($name in $OrderedNames) {
        $index = $content.IndexOf("Catalog::$name::create_entry()", [StringComparison]::Ordinal)
        if ($index -le $lastIndex) {
            throw "Generated entries are missing or not ordinally sorted."
        }
        $lastIndex = $index
    }
}

try {
    [IO.Directory]::CreateDirectory($systemsRoot) | Out-Null
    Invoke-Generator
    Assert-Catalog -Count $emptyCatalogCount -OrderedNames @()

    Add-FixtureEntry -Name "Bravo"
    Add-FixtureEntry -Name "Alpha"
    Invoke-Generator
    Assert-Catalog -Count $twoEntryCount -OrderedNames @("Alpha", "Bravo")

    $fixedTimestamp = [DateTime]::SpecifyKind(
        [DateTime]::Parse($fixedTimestampText),
        [DateTimeKind]::Utc)
    [IO.File]::SetLastWriteTimeUtc($outputPath, $fixedTimestamp)
    Invoke-Generator
    if ([IO.File]::GetLastWriteTimeUtc($outputPath) -ne $fixedTimestamp) {
        throw "Unchanged catalog was rewritten."
    }

    Remove-Item -LiteralPath (Join-Path $systemsRoot "Bravo") -Recurse
    Invoke-Generator
    Assert-Catalog -Count $singleEntryCount -OrderedNames @("Alpha")
    Write-Output "System catalog generator fixtures passed."
}
finally {
    $systemTemp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    if (-not $temporaryRoot.StartsWith($systemTemp, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove a temporary path outside the system temp directory."
    }
    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}
