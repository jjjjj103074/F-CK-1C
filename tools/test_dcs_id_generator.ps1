param(
    [string]$RepoRoot = ''
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$resolvedRoot = if ($RepoRoot) {
    [IO.Path]::GetFullPath($RepoRoot)
} else {
    [IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
}
$generator = Join-Path $resolvedRoot 'tools\generate_dcs_ids.ps1'
$utf8NoBom = [Text.UTF8Encoding]::new($false)

function Get-GeneratedPaths {
    param([string]$Root)

    return @(
        (Join-Path $Root 'src\efm\F-CK-1C_EFM\DcsIds\CustomCommands.g.h'),
        (Join-Path $Root 'Cockpit\Scripts\command_defs.lua'),
        (Join-Path $Root 'src\efm\F-CK-1C_EFM\DcsIds\CockpitParams.g.h'),
        (Join-Path $Root 'Cockpit\Scripts\generated\CockpitParams.g.lua')
    )
}

function Test-IdempotentGeneration {
    param([string]$Root)

    $paths = Get-GeneratedPaths $Root
    $before = @{}
    foreach ($path in $paths) {
        $before[$path] = [IO.File]::ReadAllText($path)
    }
    & $generator -RepoRoot $Root | Out-Null
    foreach ($path in $paths) {
        if ([IO.File]::ReadAllText($path) -cne $before[$path]) {
            throw "Generator output was not current before the test: $path"
        }
    }
}

function New-GeneratorFixture {
    param([string]$Root)

    [IO.Directory]::CreateDirectory(
        (Join-Path $Root 'src\efm\F-CK-1C_EFM\DcsIds')) | Out-Null
    [IO.Directory]::CreateDirectory(
        (Join-Path $Root 'Cockpit\Scripts')) | Out-Null
    [IO.Directory]::CreateDirectory(
        (Join-Path $Root 'Input')) | Out-Null
    Copy-Item -LiteralPath (
        Join-Path $resolvedRoot 'src\efm\F-CK-1C_EFM\DcsIds\CommandIds.json') `
        -Destination (
            Join-Path $Root 'src\efm\F-CK-1C_EFM\DcsIds\CommandIds.json') -Force
}

function Test-FixtureGeneration {
    param([string]$Root)

    & $generator -RepoRoot $Root | Out-Null
    $cpp = [IO.File]::ReadAllText(
        (Join-Path $Root 'src\efm\F-CK-1C_EFM\DcsIds\CockpitParams.g.h'))
    $lua = [IO.File]::ReadAllText(
        (Join-Path $Root 'Cockpit\Scripts\generated\CockpitParams.g.lua'))
    if ($cpp -notmatch 'namespace RawCockpitParams' -or
        $cpp -notmatch 'static constexpr Entry Catalog') {
        throw 'C++ parameter catalog metadata was not generated.'
    }
    if ($lua -notmatch 'cockpit_params' -or
        $lua -notmatch 'dcs_cockpit_params') {
        throw 'Lua parameter constants were not generated.'
    }
}

function Test-DuplicateParameterRejected {
    param([string]$Root)

    $path = Join-Path $Root 'src\efm\F-CK-1C_EFM\DcsIds\CommandIds.json'
    $catalog = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
    $catalog.cockpit_params[1].value = $catalog.cockpit_params[0].value
    [IO.File]::WriteAllText(
        $path,
        ($catalog | ConvertTo-Json -Depth 10),
        $utf8NoBom)
    try {
        & $generator -RepoRoot $Root | Out-Null
        throw 'Generator accepted duplicate cockpit parameter values.'
    }
    catch {
        if ($_.Exception.Message -notlike '*Duplicate cockpit parameter value*') {
            throw
        }
    }
}

function Test-RawMetadataRejected {
    param([string]$Root)

    $path = Join-Path $Root 'src\efm\F-CK-1C_EFM\DcsIds\CommandIds.json'
    $catalog = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
    $catalog.raw_dcs_cockpit_params[0].axis_convention = ''
    [IO.File]::WriteAllText(
        $path,
        ($catalog | ConvertTo-Json -Depth 10),
        $utf8NoBom)
    try {
        & $generator -RepoRoot $Root | Out-Null
        throw 'Generator accepted missing raw DCS parameter metadata.'
    }
    catch {
        if ($_.Exception.Message -notlike (
            '*Missing raw DCS parameter axis convention*')) {
            throw
        }
    }
}

$temporaryRoot = [IO.Path]::GetFullPath(
    (Join-Path ([IO.Path]::GetTempPath()) (
        'fck1c_dcs_ids_' + [Guid]::NewGuid())))
try {
    Test-IdempotentGeneration $resolvedRoot
    New-GeneratorFixture $temporaryRoot
    Test-FixtureGeneration $temporaryRoot
    Test-RawMetadataRejected $temporaryRoot
    New-GeneratorFixture $temporaryRoot
    Test-DuplicateParameterRejected $temporaryRoot
    Write-Output 'DCS ID generator fixtures passed.'
}
finally {
    $systemTemp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    if (-not $temporaryRoot.StartsWith(
        $systemTemp,
        [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Refusing to remove a fixture outside the system temp directory.'
    }
    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}
