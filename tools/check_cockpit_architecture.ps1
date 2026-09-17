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

function Test-DebugIndicatorFiles {
    param([string]$Directory)

    $expectedFiles = @('DebugIndicator_init.lua', 'DebugIndicator_page.lua')
    $files = @(Get-ChildItem $Directory -File -Filter '*.lua')
    $names = @($files.Name | Sort-Object)
    if (($names -join ',') -cne (($expectedFiles | Sort-Object) -join ',')) {
        Write-Output 'Debug Indicator must contain only its init and page Lua files.'
        return
    }
}

function Test-DebugIndicatorPage {
    param([string]$Directory)

    $init = [IO.File]::ReadAllText(
        (Join-Path $Directory 'DebugIndicator_init.lua'))
    $page = [IO.File]::ReadAllText(
        (Join-Path $Directory 'DebugIndicator_page.lua'))
    if ($init -notmatch (
        'purposes\s*=\s*\{\s*' +
        'render_purpose\.SCREENSPACE_INSIDE_COCKPIT\s*\}')) {
        Write-Output 'Debug Indicator must render only inside the cockpit.'
    }
    $forbidden = 'ControlsIndicator|avLuaDevice|get_param_handle|' +
        'GetSelf|listen_command|dispatch_action|make_default_activity'
    if (($init + $page) -match $forbidden) {
        Write-Output 'Debug Indicator Lua must remain display-only.'
    }

    $parameterMatches = [regex]::Matches(
        $page,
        'cockpit_params\.([A-Za-z_][A-Za-z0-9_]*)')
    $parameters = @($parameterMatches | ForEach-Object {
        $_.Groups[1].Value
    } | Sort-Object -Unique)
    if (($parameters -join ',') -cne
        ('DebugIndicatorStatus,DebugIndicatorText1,DebugIndicatorText2,' +
        'DebugIndicatorText3,DebugIndicatorText4,DebugIndicatorText5,' +
        'DebugIndicatorText6,DebugIndicatorVisible')) {
        Write-Output (
            'Debug Indicator must use only its visible, six text-block, ' +
            'and status parameters.')
    }
    if ([regex]::Matches($page, 'CreateElement\("ceStringPoly"\)').Count -ne 1) {
        Write-Output 'Debug Indicator must use one reusable text-element factory.'
    }
    if ($page -notmatch
        'for\s+index,\s*parameter\s+in\s+ipairs\(text_parameters\)') {
        Write-Output 'Debug Indicator text blocks must be generated from data.'
    }
    if ($page -notmatch 'LockOn_Options\.screen\.aspect' -or
        $page -notmatch 'local\s+column_count\s*=\s*2' -or
        $page -notmatch 'local\s+blocks_per_column\s*=\s*3') {
        Write-Output 'Debug Indicator must keep its responsive two-by-three layout.'
    }
}

function Test-DebugIndicatorRegistration {
    param([string]$Root)

    $deviceInit = [IO.File]::ReadAllText(
        (Join-Path $Root 'Cockpit\Scripts\device_init.lua'))
    $registrations = [regex]::Matches(
        $deviceInit,
        '(?m)^.*DebugIndicator/DebugIndicator_init\.lua.*$')
    if ($registrations.Count -ne 1 -or
        $registrations[0].Value -notmatch
            '\{\s*"ccControlsIndicatorBase"\s*,') {
        Write-Output (
            'Debug Indicator must be registered once with the ' +
            'screen-space ccControlsIndicatorBase host.')
    }
}

function Test-DebugIndicatorBindings {
    param([string]$Root)

    foreach ($profile in @('keyboard', 'joystick')) {
        $path = Join-Path $Root "Input\F-CK-1C\$profile\default.lua"
        $text = [IO.File]::ReadAllText($path)
        $bindings = [regex]::Matches(
            $text,
            '(?m)^.*down\s*=\s*device_commands\.DebugIndicatorToggle.*$')
        if ($bindings.Count -ne 1 -or
            $bindings[0].Value -match 'combos|cockpit_device_id|pressed|\bup\s*=' -or
            $bindings[0].Value -notmatch 'value_down\s*=\s*1\.0') {
            Write-Output (
                "Debug Indicator $profile binding must emit value_down=1.0 " +
                'with no default binding.')
        }
    }
}

function Test-DebugIndicatorBoundary {
    param([string]$Root)

    $directory = Join-Path $Root 'Cockpit\Scripts\DebugIndicator'
    Test-DebugIndicatorFiles $directory
    Test-DebugIndicatorPage $directory
    Test-DebugIndicatorRegistration $Root
    Test-DebugIndicatorBindings $Root
}

function Test-EfmAutopilotBindings {
    param([string]$Root)

    $commands = @(
        'APMasterOn',
        'APMasterOff',
        'APPitchAttitudeHold',
        'APPitchAltitudeHold',
        'APRollAttitudeHold',
        'APRollHeadingSelect',
        'APHeadingSetIncrease',
        'APHeadingSetDecrease'
    )
    foreach ($profile in @('keyboard', 'joystick')) {
        $path = Join-Path $Root "Input\F-CK-1C\$profile\default.lua"
        $text = [IO.File]::ReadAllText($path)
        foreach ($command in $commands) {
            $bindings = [regex]::Matches(
                $text,
                "(?m)^.*down\s*=\s*device_commands\.$command.*$")
            if ($bindings.Count -ne 1 -or
                $bindings[0].Value -notmatch 'value_down\s*=\s*1\.0') {
                Write-Output (
                    "EFM autopilot $profile binding $command must emit " +
                    'value_down=1.0 exactly once.')
            }
        }
    }
}

function Test-MagneticHeadingObservationBoundary {
    param([string]$Root)

    $systemPath = Join-Path $Root 'Cockpit\Scripts\Systems\hmcs_system.lua'
    $pagePath = Join-Path $Root 'Cockpit\Scripts\HMCS\HMCS_page.lua'
    $system = [IO.File]::ReadAllText($systemPath)
    $page = [IO.File]::ReadAllText($pagePath)
    if ($system -notmatch 'try_sensor_call\("getMagneticHeading"\)' -or
        $system -match 'try_sensor_call\("getHeading"\)') {
        Write-Output (
            'Heading observation must use getMagneticHeading with no ' +
            'world-yaw fallback.')
    }
    if ($system -notmatch 'HEADING_OBSERVATION_RATE_HZ\s*=\s*64' -or
        $system -notmatch 'cockpit_params\.MagneticHeadingAvailable' -or
        $system -notmatch 'cockpit_params\.MagneticHeadingRad') {
        Write-Output (
            'Heading observation adapter must publish the typed 64 Hz ' +
            'availability and magnetic-heading contract.')
    }
    if ($page -notmatch 'cockpit_params\.MagneticHeadingAvailable') {
        Write-Output 'HMCS heading presentation must honor heading availability.'
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
    Test-DebugIndicatorBoundary $resolvedRoot
    Test-EfmAutopilotBindings $resolvedRoot
    Test-MagneticHeadingObservationBoundary $resolvedRoot
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
