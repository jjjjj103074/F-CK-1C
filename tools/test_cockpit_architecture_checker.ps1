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
$checker = Join-Path $resolvedRoot 'tools\check_cockpit_architecture.ps1'
$utf8NoBom = [Text.UTF8Encoding]::new($false)

function Copy-CheckerFixture {
    param([string]$Root)

    [IO.Directory]::CreateDirectory($Root) | Out-Null
    Copy-Item -LiteralPath (
        Join-Path $resolvedRoot 'Cockpit') `
        -Destination (Join-Path $Root 'Cockpit') -Recurse
    $catalogDirectory = Join-Path $Root 'src\efm\F-CK-1C_EFM\DcsIds'
    [IO.Directory]::CreateDirectory($catalogDirectory) | Out-Null
    Copy-Item -LiteralPath (
        Join-Path $resolvedRoot 'src\efm\F-CK-1C_EFM\DcsIds\CommandIds.json') `
        -Destination $catalogDirectory
    $bridgeDirectory = Join-Path $Root 'src\efm\F-CK-1C_EFM\DcsBridge\Internal'
    [IO.Directory]::CreateDirectory($bridgeDirectory) | Out-Null
    foreach ($name in @('CockpitBridge.cpp', 'CockpitSnapshotExporter.cpp')) {
        Copy-Item -LiteralPath (
            Join-Path $resolvedRoot "src\efm\F-CK-1C_EFM\DcsBridge\Internal\$name") `
            -Destination $bridgeDirectory
    }
    $inventoryDirectory = Join-Path $Root 'docs\cockpit-baseline\generated'
    [IO.Directory]::CreateDirectory($inventoryDirectory) | Out-Null
    Copy-Item -LiteralPath (
        Join-Path $resolvedRoot 'docs\cockpit-baseline\generated\parameter-access.csv') `
        -Destination $inventoryDirectory
}

function Invoke-FixtureChecker {
    param([string]$Root)

    & $checker -RepoRoot $Root | Out-Null
}

function Assert-Rejected {
    param(
        [scriptblock]$Arrange,
        [string]$ExpectedMessage,
        [string]$Root
    )

    & $Arrange
    try {
        Invoke-FixtureChecker $Root
        throw "Checker accepted invalid fixture: $ExpectedMessage"
    }
    catch {
        if ($_.Exception.Message -notlike "*$ExpectedMessage*") {
            throw
        }
    }
}

$temporaryRoot = [IO.Path]::GetFullPath(
    (Join-Path ([IO.Path]::GetTempPath()) (
        'fck1c_cockpit_architecture_' + [Guid]::NewGuid())))
try {
    Copy-CheckerFixture $temporaryRoot
    Invoke-FixtureChecker $temporaryRoot
    $rawReferenceScript = Join-Path (
        $temporaryRoot) 'Cockpit\Scripts\RawReference.lua'
    [IO.File]::WriteAllText(
        $rawReferenceScript,
        "local raw = get_param_handle(dcs_cockpit_params.RadarMode)`r`n",
        $utf8NoBom)
    Invoke-FixtureChecker $temporaryRoot
    Remove-Item -LiteralPath $rawReferenceScript -Force
    $parameterInventory = Join-Path (
        $temporaryRoot) 'docs\cockpit-baseline\generated\parameter-access.csv'
    $originalInventory = [IO.File]::ReadAllText($parameterInventory)
    Assert-Rejected {
        [IO.File]::AppendAllText(
            $parameterInventory,
            "`"AIM9_TONE_STATE`",`"Write`",`"Lua`",`"Bad.lua`"`r`n",
            $utf8NoBom)
    } 'Parameter has multiple writers' $temporaryRoot
    [IO.File]::WriteAllText(
        $parameterInventory,
        $originalInventory,
        $utf8NoBom)

    $badWriter = Join-Path (
        $temporaryRoot) 'src\efm\F-CK-1C_EFM\DcsBridge\Internal\BadWriter.cpp'
    Assert-Rejected {
        [IO.File]::WriteAllText(
            $badWriter,
            'cockpit_parameter_writer(' +
            'DcsIds::CockpitParams::CockpitSnapshotAvailable);',
            $utf8NoBom)
    } 'Parameter has multiple writers' $temporaryRoot
    Remove-Item -LiteralPath $badWriter -Force

    Assert-Rejected {
        [IO.File]::WriteAllText(
            $badWriter,
            'pfn_ed_cockpit_update_parameter_with_number(handle, value);',
            $utf8NoBom)
    } 'Direct cockpit numeric update API is forbidden' $temporaryRoot
    Remove-Item -LiteralPath $badWriter -Force

    $weaponSystem = Join-Path (
        $temporaryRoot) 'Cockpit\Scripts\Systems\weapon_system.lua'
    $originalWeaponSystem = [IO.File]::ReadAllText($weaponSystem)
    Assert-Rejected {
        $withoutFailurePublish = $originalWeaponSystem.Replace(
            'publish_station_observation(false, 0, NO_STATION)',
            '-- unavailable observation removed')
        [IO.File]::WriteAllText(
            $weaponSystem,
            $withoutFailurePublish,
            $utf8NoBom)
    } 'Selected-station API failure must publish' $temporaryRoot
    [IO.File]::WriteAllText(
        $weaponSystem,
        $originalWeaponSystem,
        $utf8NoBom)

    $badScript = Join-Path $temporaryRoot 'Cockpit\Scripts\Bad.lua'
    Assert-Rejected {
        [IO.File]::WriteAllText(
            $badScript,
            "local bad = get_param_handle(`"NOT_CATALOGUED`")`r`n",
            $utf8NoBom)
    } 'Uncatalogued parameter literal' $temporaryRoot
    Remove-Item -LiteralPath $badScript -Force

    Assert-Rejected {
        [IO.File]::WriteAllText(
            $badScript,
            "local host = create_sound_host(`"BAD`", `"HEADPHONES`", 0, 0, 0)`r`n",
            $utf8NoBom)
    } 'create_sound_host is used outside its adapter' $temporaryRoot
    Remove-Item -LiteralPath $badScript -Force

    $devicesPath = Join-Path $temporaryRoot 'Cockpit\Scripts\devices.lua'
    $devices = [IO.File]::ReadAllText($devicesPath).Replace('Gear = 1', 'Gear = 10')
    Assert-Rejected {
        [IO.File]::WriteAllText($devicesPath, $devices, $utf8NoBom)
    } 'Stable device ID missing or changed' $temporaryRoot

    $removedGear = Join-Path (
        $temporaryRoot) 'Cockpit\Scripts\Systems\gear_system.lua'
    Assert-Rejected {
        [IO.File]::WriteAllText(
            $removedGear,
            'local dev = GetSelf()',
            $utf8NoBom)
    } 'Removed cockpit device file was restored' $temporaryRoot

    $removedAutopilot = Join-Path (
        $temporaryRoot) 'Cockpit\Scripts\Systems\autopilot_system.lua'
    Assert-Rejected {
        [IO.File]::WriteAllText(
            $removedAutopilot,
            'local dev = GetSelf()',
            $utf8NoBom)
    } 'Removed cockpit device file was restored' $temporaryRoot
    Write-Output 'Cockpit architecture checker fixtures passed.'
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
