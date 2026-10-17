param(
    [string]$RepoRoot = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$customCommandMinimum = 3000
$customCommandMaximum = 3999
$validRoutes = @('efm', 'bridge', 'cockpit')
$validDirections = @('cpp_to_lua', 'lua_to_cpp', 'lua_internal')
$validRawDirections = @('dcs_to_cpp')
$validVerificationBases = @(
    'community_documentation',
    'existing_module_contract'
)
$validUnits = @(
    'boolean',
    'count',
    'counter',
    'degrees',
    'enum',
    'feet',
    'feet_per_minute',
    'kelvin',
    'knots',
    'meters',
    'meters_per_second',
    'normalized',
    'radians',
    'revision',
    'seconds',
    'station_index',
    'text'
)
$utf8NoBom = [Text.UTF8Encoding]::new($false)

function Resolve-RepositoryRoot {
    param([string]$RequestedRoot)

    if ($RequestedRoot) {
        return [IO.Path]::GetFullPath($RequestedRoot)
    }
    return [IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
}

function Assert-Symbol {
    param(
        [string]$Name,
        [string]$Kind
    )

    if ($Name -notmatch '^[A-Za-z_][A-Za-z0-9_]*$') {
        throw "Invalid $Kind name: $Name"
    }
}

function Assert-Unique {
    param(
        [hashtable]$Seen,
        [object]$Value,
        [string]$Message
    )

    if ($Seen.ContainsKey($Value)) {
        throw "$Message conflicts with $($Seen[$Value])"
    }
}

function Assert-OptionalText {
    param(
        [object]$Value,
        [string]$Message
    )

    if ($null -ne $Value -and [string]::IsNullOrWhiteSpace([string]$Value)) {
        throw $Message
    }
}

function Get-OptionalProperty {
    param(
        [object]$Value,
        [string]$Name
    )

    $property = $Value.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }
    return $property.Value
}

function Test-CommandCatalog {
    param([object[]]$Commands)

    $names = @{}
    $values = @{}
    foreach ($command in $Commands) {
        $name = [string]$command.name
        $value = [int]$command.value
        $route = [string]$command.route
        Assert-Symbol $name 'command'
        Assert-Unique $names $name "Duplicate command name: $name"
        Assert-Unique $values $value "Duplicate command ID: $name=$value"
        if ($value -lt $customCommandMinimum -or $value -gt $customCommandMaximum) {
            throw "Custom command ID outside reserved range: $name=$value"
        }
        if ($route -notin $validRoutes) {
            throw "Invalid command route: $name=$route"
        }
        $names[$name] = $value
        $values[$value] = $name
    }
    return $names
}

function Test-IgnoredCommandCatalog {
    param([object[]]$Commands)

    $names = @{}
    $values = @{}
    foreach ($command in $Commands) {
        $name = [string]$command.name
        $value = [int]$command.value
        Assert-Symbol $name 'ignored DCS command'
        Assert-Unique $names $name "Duplicate ignored DCS command name: $name"
        Assert-Unique $values $value "Duplicate ignored DCS command ID: $name=$value"
        if ($value -lt 0) {
            throw "Invalid ignored DCS command ID: $name=$value"
        }
        if ([string]::IsNullOrWhiteSpace([string]$command.reason)) {
            throw "Missing ignored DCS command reason: $name=$value"
        }
        $names[$name] = $value
        $values[$value] = $name
    }
    return $names
}

function Test-CockpitParameterCatalog {
    param([object[]]$Parameters)

    $names = @{}
    $values = @{}
    foreach ($parameter in $Parameters) {
        $name = [string]$parameter.name
        $value = [string]$parameter.value
        Assert-Symbol $name 'cockpit parameter'
        Assert-Unique $names $name "Duplicate cockpit parameter name: $name"
        Assert-Unique $values $value "Duplicate cockpit parameter value: $name=$value"
        if ([string]::IsNullOrWhiteSpace($value)) {
            throw "Empty cockpit parameter value: $name"
        }
        if ([string]$parameter.direction -notin $validDirections) {
            throw "Invalid cockpit parameter direction: $name=$($parameter.direction)"
        }
        if ([string]$parameter.unit -notin $validUnits) {
            throw "Invalid cockpit parameter unit: $name=$($parameter.unit)"
        }
        Assert-OptionalText $parameter.writer "Empty cockpit parameter writer: $name"
        Assert-OptionalText (
            Get-OptionalProperty $parameter 'cpp_reader') `
            "Empty cockpit parameter C++ reader: $name"
        if ([string]::IsNullOrWhiteSpace([string]$parameter.target_owner)) {
            throw "Missing cockpit parameter target owner: $name"
        }
        $names[$name] = $value
        $values[$value] = $name
    }
}

function Test-RawParameterCatalog {
    param(
        [object[]]$Parameters,
        [object[]]$CustomParameters
    )

    $names = @{}
    $values = @{}
    foreach ($custom in $CustomParameters) {
        $values[[string]$custom.value] = [string]$custom.name
    }
    foreach ($parameter in $Parameters) {
        $name = [string]$parameter.name
        $value = [string]$parameter.value
        Assert-Symbol $name 'raw DCS cockpit parameter'
        Assert-Unique $names $name "Duplicate raw DCS parameter name: $name"
        Assert-Unique $values $value "Duplicate cockpit parameter value: $name=$value"
        if ([string]::IsNullOrWhiteSpace($value)) {
            throw "Empty raw DCS cockpit parameter value: $name"
        }
        if ([string]$parameter.unit -notin $validUnits) {
            throw "Invalid raw DCS parameter unit: $name=$($parameter.unit)"
        }
        if ([string]$parameter.data_direction -notin $validRawDirections) {
            throw "Invalid raw DCS parameter direction: $name=$($parameter.data_direction)"
        }
        if ([string]::IsNullOrWhiteSpace([string]$parameter.axis_convention)) {
            throw "Missing raw DCS parameter axis convention: $name"
        }
        if ([string]::IsNullOrWhiteSpace([string]$parameter.source)) {
            throw "Missing raw DCS parameter source: $name"
        }
        if ([string]$parameter.verification_basis -notin $validVerificationBases) {
            throw (
                "Invalid raw DCS parameter verification basis: " +
                "$name=$($parameter.verification_basis)")
        }
        if ([string]::IsNullOrWhiteSpace([string]$parameter.reference)) {
            throw "Missing raw DCS parameter reference: $name"
        }
        Assert-OptionalText (
            Get-OptionalProperty $parameter 'cpp_reader') `
            "Empty raw DCS C++ reader: $name"
        $names[$name] = $value
        $values[$value] = $name
    }
}

function Convert-ToCppText {
    param([object]$Value)

    if ($null -eq $Value) {
        return 'nullptr'
    }
    return '"' + ([string]$Value).Replace('\', '\\').Replace('"', '\"') + '"'
}

function New-CommandCppLines {
    param(
        [object[]]$Commands,
        [object[]]$IgnoredCommands
    )

    $lines = [Collections.Generic.List[string]]::new()
    $lines.AddRange([string[]]@(
        '#pragma once', '',
        '// Generated by tools/generate_dcs_ids.ps1 from DcsIds/CommandIds.json.',
        '// Do not edit this file directly.', 'namespace DcsIds', '{',
        'namespace Commands', '{'))
    foreach ($command in $Commands) {
        $lines.Add("static constexpr int $($command.name) = $([int]$command.value);")
    }
    $lines.AddRange([string[]]@('}', '', 'namespace DcsCommands', '{'))
    foreach ($command in $IgnoredCommands) {
        $lines.Add("static constexpr int $($command.name) = $([int]$command.value);")
    }
    $lines.AddRange([string[]]@(
        '}', '', 'namespace CommandRouting', '{', 'enum class Route',
        '{', '    Efm,', '    Bridge,', '    Cockpit', '};', '', 'struct Entry', '{',
        '    int id;', '    Route route;', '};', '',
        'static constexpr Entry CustomCommands[] = {'))
    foreach ($command in $Commands) {
        $route = switch ([string]$command.route) {
            'efm' { 'Efm' }
            'bridge' { 'Bridge' }
            'cockpit' { 'Cockpit' }
        }
        $lines.Add("    { Commands::$($command.name), Route::$route },")
    }
    $lines.AddRange([string[]]@('};', '', 'static constexpr int IgnoredDcsCommands[] = {'))
    foreach ($command in $IgnoredCommands) {
        $lines.Add("    DcsCommands::$($command.name),")
    }
    $lines.AddRange([string[]]@('};', '}', '}', ''))
    return $lines.ToArray()
}

function New-CommandLuaLines {
    param(
        [object[]]$Commands,
        [object[]]$IgnoredCommands
    )

    $lines = [Collections.Generic.List[string]]::new()
    $lines.AddRange([string[]]@(
        '-- Generated by tools/generate_dcs_ids.ps1 from DcsIds/CommandIds.json.',
        '-- Do not edit this file directly.', 'device_commands = {'))
    foreach ($command in $Commands) {
        $lines.Add("    $($command.name) = $([int]$command.value),")
    }
    $lines.AddRange([string[]]@('}', '', 'dcs_commands = {'))
    foreach ($command in $IgnoredCommands) {
        $lines.Add("    $($command.name) = $([int]$command.value),")
    }
    $lines.AddRange([string[]]@('}', ''))
    return $lines.ToArray()
}

function New-CppParameterEntries {
    param([object[]]$Parameters)

    foreach ($parameter in $Parameters) {
        Write-Output (
            "    { $($parameter.name), " +
            "$(Convert-ToCppText $parameter.direction), " +
            "$(Convert-ToCppText $parameter.unit), " +
            "$(Convert-ToCppText $parameter.writer), " +
            "$(Convert-ToCppText $parameter.target_owner), " +
            "$(Convert-ToCppText (Get-OptionalProperty $parameter 'cpp_reader')) },")
    }
}

function New-RawCppParameterEntries {
    param([object[]]$Parameters)

    foreach ($parameter in $Parameters) {
        Write-Output (
            "    { $($parameter.name), " +
            "$(Convert-ToCppText $parameter.data_direction), " +
            "$(Convert-ToCppText $parameter.unit), " +
            "$(Convert-ToCppText $parameter.axis_convention), " +
            "$(Convert-ToCppText $parameter.source), " +
            "$(Convert-ToCppText $parameter.verification_basis), " +
            "$(Convert-ToCppText $parameter.reference), " +
            "$(Convert-ToCppText (Get-OptionalProperty $parameter 'cpp_reader')) },")
    }
}

function New-CockpitCppLines {
    param(
        [object[]]$Parameters,
        [object[]]$RawParameters
    )

    $lines = [Collections.Generic.List[string]]::new()
    $lines.AddRange([string[]]@(
        '#pragma once', '',
        '// Generated by tools/generate_dcs_ids.ps1 from DcsIds/CommandIds.json.',
        '// Do not edit this file directly.', 'namespace DcsIds', '{',
        'namespace CockpitParams', '{'))
    foreach ($parameter in $Parameters) {
        $lines.Add("static const char* const $($parameter.name) = `"$($parameter.value)`";")
    }
    $lines.AddRange([string[]]@(
        '', 'struct Entry', '{', '    const char* name;',
        '    const char* direction;', '    const char* unit;',
        '    const char* writer;', '    const char* target_owner;',
        '    const char* cpp_reader;', '};', '',
        'static constexpr Entry Catalog[] = {'))
    $lines.AddRange([string[]](New-CppParameterEntries $Parameters))
    $lines.AddRange([string[]]@('};', '}', '', 'namespace RawCockpitParams', '{'))
    foreach ($parameter in $RawParameters) {
        $lines.Add("static const char* const $($parameter.name) = `"$($parameter.value)`";")
    }
    $lines.AddRange([string[]]@(
        '', 'struct Entry', '{', '    const char* name;',
        '    const char* data_direction;', '    const char* unit;',
        '    const char* axis_convention;', '    const char* source;',
        '    const char* verification_basis;', '    const char* reference;',
        '    const char* cpp_reader;', '};', '',
        'static constexpr Entry Catalog[] = {'))
    $lines.AddRange([string[]](New-RawCppParameterEntries $RawParameters))
    $lines.AddRange([string[]]@('};', '}', '}', ''))
    return $lines.ToArray()
}

function New-CockpitLuaLines {
    param(
        [object[]]$Parameters,
        [object[]]$RawParameters
    )

    $lines = [Collections.Generic.List[string]]::new()
    $lines.AddRange([string[]]@(
        '-- Generated by tools/generate_dcs_ids.ps1 from DcsIds/CommandIds.json.',
        '-- Do not edit this file directly.', 'cockpit_params = {'))
    foreach ($parameter in $Parameters) {
        $lines.Add("    $($parameter.name) = `"$($parameter.value)`",")
    }
    $lines.AddRange([string[]]@('}', '', 'dcs_cockpit_params = {'))
    foreach ($parameter in $RawParameters) {
        $lines.Add("    $($parameter.name) = `"$($parameter.value)`",")
    }
    $lines.AddRange([string[]]@('}', ''))
    return $lines.ToArray()
}

function Test-LuaCommandReferencesInFile {
    param(
        [IO.FileInfo]$File,
        [hashtable]$CommandNames,
        [hashtable]$IgnoredNames
    )

    $content = Get-Content -LiteralPath $File.FullName -Raw
    foreach ($match in [regex]::Matches(
        $content, 'device_commands\.([A-Za-z_][A-Za-z0-9_]*)')) {
        if (-not $CommandNames.ContainsKey($match.Groups[1].Value)) {
            throw "Lua references unknown device command '$($match.Groups[1].Value)' in $($File.FullName)"
        }
    }
    foreach ($match in [regex]::Matches(
        $content, 'dcs_commands\.([A-Za-z_][A-Za-z0-9_]*)')) {
        if (-not $IgnoredNames.ContainsKey($match.Groups[1].Value)) {
            throw "Lua references undeclared DCS command '$($match.Groups[1].Value)' in $($File.FullName)"
        }
    }
}

function Test-LuaCommandReferences {
    param(
        [string]$Root,
        [hashtable]$CommandNames,
        [hashtable]$IgnoredNames
    )

    foreach ($relativeRoot in @('Cockpit', 'Input')) {
        $luaRoot = Join-Path $Root $relativeRoot
        foreach ($file in Get-ChildItem -LiteralPath $luaRoot -Filter '*.lua' -File -Recurse) {
            Test-LuaCommandReferencesInFile $file $CommandNames $IgnoredNames
        }
    }
}

function Write-GeneratedFile {
    param(
        [string]$Path,
        [Collections.Generic.List[string]]$Lines
    )

    [IO.Directory]::CreateDirectory((Split-Path -Parent $Path)) | Out-Null
    [IO.File]::WriteAllText($Path, ($Lines -join "`r`n"), $utf8NoBom)
    Write-Output "Generated: $Path"
}

$resolvedRoot = Resolve-RepositoryRoot $RepoRoot
$sourcePath = Join-Path $resolvedRoot 'src\efm\F-CK-1C_EFM\DcsIds\CommandIds.json'
if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
    throw "Command ID source not found: $sourcePath"
}

$document = Get-Content -LiteralPath $sourcePath -Raw | ConvertFrom-Json
$commands = @($document.commands)
$ignoredCommands = @($document.efm_ignored_dcs_commands)
$parameters = @($document.cockpit_params)
$rawParameters = @($document.raw_dcs_cockpit_params)
$commandNames = Test-CommandCatalog $commands
$ignoredNames = Test-IgnoredCommandCatalog $ignoredCommands
Test-CockpitParameterCatalog $parameters
Test-RawParameterCatalog $rawParameters $parameters

$commandCpp = [Collections.Generic.List[string]](
    [string[]](New-CommandCppLines $commands $ignoredCommands))
$commandLua = [Collections.Generic.List[string]](
    [string[]](New-CommandLuaLines $commands $ignoredCommands))
$cockpitCpp = [Collections.Generic.List[string]](
    [string[]](New-CockpitCppLines $parameters $rawParameters))
$cockpitLua = [Collections.Generic.List[string]](
    [string[]](New-CockpitLuaLines $parameters $rawParameters))

Write-GeneratedFile (
    Join-Path $resolvedRoot 'src\efm\F-CK-1C_EFM\DcsIds\CustomCommands.g.h') $commandCpp
Write-GeneratedFile (
    Join-Path $resolvedRoot 'Cockpit\Scripts\command_defs.lua') $commandLua
Write-GeneratedFile (
    Join-Path $resolvedRoot 'src\efm\F-CK-1C_EFM\DcsIds\CockpitParams.g.h') $cockpitCpp
Write-GeneratedFile (
    Join-Path $resolvedRoot 'Cockpit\Scripts\generated\CockpitParams.g.lua') $cockpitLua
Test-LuaCommandReferences $resolvedRoot $commandNames $ignoredNames
