param(
    [string]$RepoRoot = '',
    [string]$ParameterAccessPath = ''
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Get-ExpectedDeviceIds {
    return [ordered]@{
        Gear = 1
        Actuators = 2
        CMS = 3
        WEAPON_SYSTEM = 4
        HMCS = 5
        AAM_AUDIO = 6
        RADAR = 7
        RADAR_STATE = 8
        AUTOPILOT = 9
    }
}

function Resolve-RepositoryRoot {
    param([string]$RequestedRoot)

    if ($RequestedRoot) {
        return [IO.Path]::GetFullPath($RequestedRoot)
    }
    return [IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
}

function Get-RelativePath {
    param(
        [string]$Root,
        [string]$Path
    )

    return $Path.Substring($Root.Length + 1).Replace('\', '/')
}

function Get-CatalogMaps {
    param([object]$Catalog)

    $customByValue = @{}
    $customByName = @{}
    foreach ($parameter in $Catalog.cockpit_params) {
        $customByValue[[string]$parameter.value] = $parameter
        $customByName[[string]$parameter.name] = $parameter
    }
    $rawByValue = @{}
    $rawByName = @{}
    foreach ($parameter in $Catalog.raw_dcs_cockpit_params) {
        $rawByValue[[string]$parameter.value] = $parameter
        $rawByName[[string]$parameter.name] = $parameter
    }
    return @{
        CustomByValue = $customByValue
        CustomByName = $customByName
        RawByValue = $rawByValue
        RawByName = $rawByName
    }
}

function Get-CppWriterRows {
    param(
        [string]$Root,
        [hashtable]$Maps
    )

    $cppRoot = Join-Path $Root 'src\efm\F-CK-1C_EFM'
    $writerPattern = (
        'cockpit_parameter_writer\s*\(\s*' +
        '(?:DcsIds::)?CockpitParams::([A-Za-z_][A-Za-z0-9_]*)\s*\)')
    foreach ($file in Get-ChildItem $cppRoot -Recurse -File -Include '*.cpp','*.h') {
        $text = [IO.File]::ReadAllText($file.FullName)
        $relative = Get-RelativePath $Root $file.FullName
        $isAllowedApiOwner = $relative -ceq (
            'src/efm/F-CK-1C_EFM/DcsBridge/Internal/' +
            'CockpitParameterEndpoint.h')
        $isDcsSdkHeader = $relative -clike (
            'src/efm/F-CK-1C_EFM/include/Cockpit/*')
        if ($text -match 'pfn_ed_cockpit_update_parameter_with_number' -and
            -not $isAllowedApiOwner -and
            -not $isDcsSdkHeader) {
            throw (
                "Direct cockpit numeric update API is forbidden; " +
                "use CockpitParameterEndpoint: $($file.FullName)")
        }
        $writerMatches = [regex]::Matches($text, $writerPattern)
        foreach ($match in $writerMatches) {
            $name = $match.Groups[1].Value
            if (-not $Maps.CustomByName.ContainsKey($name)) {
                throw "C++ registers unknown cockpit writer parameter: $name"
            }
            [pscustomobject]@{
                Parameter = $Maps.CustomByName[$name].value
                Access = 'Write'
                Layer = 'C++'
                Source = $relative
            }
        }
    }
}

function Test-DeviceIds {
    param([string]$Root)

    $expectedDeviceIds = Get-ExpectedDeviceIds
    $path = Join-Path $Root 'Cockpit\Scripts\devices.lua'
    $text = [IO.File]::ReadAllText($path)
    foreach ($entry in $expectedDeviceIds.GetEnumerator()) {
        $pattern = "(?m)^\s*$([regex]::Escape($entry.Key))\s*=\s*$($entry.Value),?\s*$"
        if ($text -notmatch $pattern) {
            Write-Output (
                "Stable device ID missing or changed: $($entry.Key)=$($entry.Value)")
        }
    }
    $assignments = [regex]::Matches(
        $text,
        '(?m)^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(\d+),?\s*$')
    if ($assignments.Count -ne $expectedDeviceIds.Count) {
        Write-Output (
            "Device table has $($assignments.Count) explicit IDs; " +
            "expected $($expectedDeviceIds.Count).")
    }
}

function Test-RemovedCockpitDevices {
    param([string]$Root)

    $removedPaths = @(
        'Cockpit\Scripts\Systems\gear_system.lua',
        'Cockpit\Scripts\Systems\actuators.lua',
        'Cockpit\Scripts\Systems\actuators_system.lua',
        'Cockpit\Scripts\Systems\autopilot_system.lua'
    )
    foreach ($relativePath in $removedPaths) {
        if (Test-Path -LiteralPath (Join-Path $Root $relativePath)) {
            Write-Output "Removed cockpit device file was restored: $relativePath"
        }
    }
    $deviceInitPath = Join-Path $Root 'Cockpit\Scripts\device_init.lua'
    $deviceInit = [IO.File]::ReadAllText($deviceInitPath)
    if ($deviceInit -match 'devices\.(Gear|Actuators|AUTOPILOT)' -or
        $deviceInit -match '(gear_system|actuators_system|autopilot_system)\.lua') {
        Write-Output 'A removed Gear, Actuators, or Autopilot device is still registered.'
    }
}

function Find-CatalogParameter {
    param(
        [hashtable]$Maps,
        [string]$Value
    )

    if ($Maps.CustomByValue.ContainsKey($Value)) {
        return $Maps.CustomByValue[$Value]
    }
    foreach ($parameter in $Maps.CustomByValue.Values) {
        if ($Value.EndsWith('*') -and
            $Value.TrimEnd('*') -eq [string]$parameter.value) {
            return $parameter
        }
    }
    return $null
}

function Test-SingleWriters {
    param(
        [object[]]$Rows,
        [hashtable]$Maps
    )

    $writerRows = @($Rows | Where-Object { $_.Access -match 'Write' })
    foreach ($group in $writerRows | Group-Object Parameter) {
        $sources = @($group.Group.Source | Sort-Object -Unique)
        if ($sources.Count -ne 1) {
            Write-Output (
                "Parameter has multiple writers: $($group.Name) -> " +
                ($sources -join ', '))
            continue
        }
        $parameter = Find-CatalogParameter $Maps $group.Name
        if ($null -eq $parameter) {
            Write-Output "Written parameter is absent from catalog: $($group.Name)"
            continue
        }
        if ([string]$parameter.writer -cne [string]$sources[0]) {
            Write-Output (
                "Catalog writer mismatch: $($group.Name) expected " +
                "$($parameter.writer), found $($sources[0])")
        }
    }
}

function Test-CatalogWritersObserved {
    param(
        [object[]]$Rows,
        [object]$Catalog
    )

    $writerRows = @($Rows | Where-Object { $_.Access -match 'Write' })
    foreach ($parameter in $Catalog.cockpit_params) {
        if ($null -eq $parameter.writer) { continue }
        $value = [string]$parameter.value
        $observed = @($writerRows | Where-Object {
            $_.Parameter -eq $value -or $_.Parameter -eq ($value + '*')
        })
        if ($observed.Count -eq 0) {
            Write-Output (
                "Catalog writer was not observed: $value -> $($parameter.writer)")
        }
    }
}

function Test-ParameterReferences {
    param(
        [string]$Root,
        [hashtable]$Maps
    )

    $scriptRoot = Join-Path $Root 'Cockpit\Scripts'
    foreach ($file in Get-ChildItem $scriptRoot -Recurse -File -Filter '*.lua') {
        $text = [IO.File]::ReadAllText($file.FullName)
        $relative = Get-RelativePath $Root $file.FullName
        foreach ($match in [regex]::Matches(
            $text, 'get_param_handle\s*\(\s*"([A-Z][A-Z0-9_]*)"')) {
            $value = $match.Groups[1].Value
            if (-not $Maps.CustomByValue.ContainsKey($value) -and
                -not $Maps.RawByValue.ContainsKey($value)) {
                Write-Output "Uncatalogued parameter literal: $value in $relative"
            }
        }
        foreach ($match in [regex]::Matches(
            $text,
            '(?<![A-Za-z0-9_])cockpit_params\.([A-Za-z_][A-Za-z0-9_]*)')) {
            if (-not $Maps.CustomByName.ContainsKey($match.Groups[1].Value)) {
                Write-Output (
                    "Unknown generated parameter symbol in $relative`: " +
                    $match.Groups[1].Value)
            }
        }
        foreach ($match in [regex]::Matches(
            $text,
            '(?<![A-Za-z0-9_])dcs_cockpit_params\.([A-Za-z_][A-Za-z0-9_]*)')) {
            if (-not $Maps.RawByName.ContainsKey($match.Groups[1].Value)) {
                Write-Output (
                    "Unknown raw DCS parameter symbol in $relative`: " +
                    $match.Groups[1].Value)
            }
        }
    }
}

function Test-ExclusiveLuaApi {
    param(
        [string]$Root,
        [object]$Rule
    )

    $scriptRoot = Join-Path $Root 'Cockpit\Scripts'
    foreach ($file in Get-ChildItem $scriptRoot -Recurse -File -Filter '*.lua') {
        $text = [IO.File]::ReadAllText($file.FullName)
        if ($text -notmatch $Rule.Pattern) { continue }
        $relative = Get-RelativePath $Root $file.FullName
        if ($relative -cne $Rule.AllowedPath) {
            Write-Output "$($Rule.Label) is used outside its adapter: $relative"
        }
    }
}

function Test-AdapterOwnership {
    param([string]$Root)

    $rules = @(
        [pscustomobject]@{
            Pattern = '\bcreate_sound_host\s*\('
            AllowedPath = 'Cockpit/Scripts/Systems/aam_audio_system.lua'
            Label = 'create_sound_host'
        },
        [pscustomobject]@{
            Pattern = '\bset_power\s*\('
            AllowedPath = 'Cockpit/Scripts/RADAR/FCK1C_Radar.lua'
            Label = 'radar.set_power'
        },
        [pscustomobject]@{
            Pattern = '\bWeaponSystem[.:](get_station_info|select_station)\b'
            AllowedPath = 'Cockpit/Scripts/Systems/weapon_system.lua'
            Label = 'weapon station API'
        },
        [pscustomobject]@{
            Pattern = '\bdispatch_action\s*\('
            AllowedPath = 'Cockpit/Scripts/Systems/cms_system.lua'
            Label = 'dispatch_action'
        }
    )
    foreach ($rule in $rules) {
        Test-ExclusiveLuaApi $Root $rule
    }
}

function Test-WeaponStationFailureContract {
    param([string]$Root)

    $path = Join-Path $Root 'Cockpit\Scripts\Systems\weapon_system.lua'
    $text = [IO.File]::ReadAllText($path)
    $requirements = @(
        'if\s+not\s+ok\s+or\s+info\s*==\s*nil\s+then',
        'publish_station_observation\s*\(\s*false\s*,\s*0\s*,\s*NO_STATION\s*\)'
    )
    foreach ($requirement in $requirements) {
        if ($text -notmatch $requirement) {
            Write-Output (
                'Selected-station API failure must publish an unavailable ' +
                'weapon observation.')
            return
        }
    }
}

$resolvedRoot = Resolve-RepositoryRoot $RepoRoot
$catalogPath = Join-Path (
    $resolvedRoot) 'src\efm\F-CK-1C_EFM\DcsIds\CommandIds.json'
$catalog = Get-Content -LiteralPath $catalogPath -Raw | ConvertFrom-Json
$maps = Get-CatalogMaps $catalog
$accessPath = if ($ParameterAccessPath) {
    [IO.Path]::GetFullPath($ParameterAccessPath)
} else {
    Join-Path $resolvedRoot 'docs\cockpit-baseline\generated\parameter-access.csv'
}
if (-not (Test-Path -LiteralPath $accessPath -PathType Leaf)) {
    throw "Parameter access inventory is missing: $accessPath"
}
$baselineRows = @(Import-Csv -LiteralPath $accessPath)
$cppWriterRows = @(Get-CppWriterRows $resolvedRoot $maps)
$parameterRows = @(
    $baselineRows | Where-Object {
        $_.Layer -cne 'C++' -or $_.Access -notmatch 'Write'
    }
    $cppWriterRows
)

$findings = @(
    Test-DeviceIds $resolvedRoot
    Test-RemovedCockpitDevices $resolvedRoot
    Test-SingleWriters $parameterRows $maps
    Test-CatalogWritersObserved $parameterRows $catalog
    Test-ParameterReferences $resolvedRoot $maps
    Test-AdapterOwnership $resolvedRoot
    Test-WeaponStationFailureContract $resolvedRoot
)

if ($findings.Count -ne 0) {
    throw "Cockpit architecture check failed:`n$($findings -join "`n")"
}
Write-Output (
    "Cockpit architecture check passed " +
    "($($catalog.cockpit_params.Count) custom parameters, " +
    "$($catalog.raw_dcs_cockpit_params.Count) raw DCS parameters).")
