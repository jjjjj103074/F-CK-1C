[CmdletBinding()]
param(
    [ValidateSet("Check", "Refresh")]
    [string]$Mode = "Check",
    [string]$RepoRoot = ""
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$BaselineRepoRoot = if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    Split-Path -Parent $PSScriptRoot
} else {
    [System.IO.Path]::GetFullPath($RepoRoot)
}
$ExpectedCockpitFileCount = 21
$ExpectedDeviceCount = 6
$ExpectedIndicatorCount = 3
$Utf8NoBom = [System.Text.UTF8Encoding]::new($false)
$OutputRoot = Join-Path $BaselineRepoRoot "docs\cockpit-baseline\generated"
$CommandCatalogPath = Join-Path $BaselineRepoRoot "src\efm\F-CK-1C_EFM\DcsIds\CommandIds.json"

function Get-RelativePath {
    param([string]$Path)

    $root = [System.IO.Path]::GetFullPath($BaselineRepoRoot).TrimEnd("\", "/") + "\"
    $fullPath = [System.IO.Path]::GetFullPath($Path)
    if (-not $fullPath.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Path is outside repository: $fullPath"
    }
    return $fullPath.Substring($root.Length).Replace("\", "/")
}

function Get-Text {
    param([string]$Path)

    return [System.IO.File]::ReadAllText($Path)
}

function Get-LineCount {
    param([string]$Text)

    if ($Text.Length -eq 0) {
        return 0
    }
    $lineBreaks = [regex]::Matches($Text, "\r\n|\n").Count
    if ($Text.EndsWith("`n")) {
        return $lineBreaks
    }
    return $lineBreaks + 1
}

function Get-BaselineSourceFiles {
    $cockpitFiles = @(Get-ChildItem (Join-Path $BaselineRepoRoot "Cockpit\Scripts") -Recurse -File |
        Sort-Object FullName)
    if ($cockpitFiles.Count -ne $ExpectedCockpitFileCount) {
        throw "Expected $ExpectedCockpitFileCount Cockpit files, found $($cockpitFiles.Count)."
    }
    $contractPaths = @(
        "F-CK-1C.lua",
        "Input\F-CK-1C\joystick\default.lua",
        "Input\F-CK-1C\keyboard\default.lua",
        "src\efm\F-CK-1C_EFM\DcsIds\CommandIds.json",
        "src\efm\F-CK-1C_EFM\DcsBridge\Internal\CockpitBridge.cpp",
        "src\efm\F-CK-1C_EFM\DcsBridge\Internal\CockpitBridge.h",
        "src\efm\F-CK-1C_EFM\DcsBridge\Internal\CockpitParameterEndpoint.h",
        "src\efm\F-CK-1C_EFM\DcsBridge\Internal\CockpitParameterEvents.h",
        "src\efm\F-CK-1C_EFM\DcsBridge\Internal\CockpitSnapshotExporter.cpp",
        "src\efm\F-CK-1C_EFM\DcsBridge\Internal\CockpitSnapshotExporter.h",
        "src\efm\F-CK-1C_EFM\DcsBridge\Internal\DcsCommandRouter.cpp",
        "src\efm\F-CK-1C_EFM\DcsBridge\Internal\DcsCommandRouter.h",
        "src\efm\F-CK-1C_EFM\Core\Contracts\CockpitContracts.h",
        "src\efm\F-CK-1C_EFM\Core\Contracts\Commands.h",
        "src\efm\F-CK-1C_EFM\Core\Contracts\FrameContracts.h"
    )
    $contractFiles = foreach ($relativePath in $contractPaths) {
        $path = Join-Path $BaselineRepoRoot $relativePath
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required baseline source is missing: $relativePath"
        }
        Get-Item -LiteralPath $path
    }
    return @($cockpitFiles + $contractFiles | Sort-Object FullName)
}

function ConvertTo-StableCsv {
    param([object[]]$Rows)

    if ($Rows.Count -eq 0) {
        throw "Baseline artifact cannot be empty."
    }
    $csv = $Rows | ConvertTo-Csv -NoTypeInformation
    return (($csv -join "`r`n") + "`r`n")
}

function New-SourceManifest {
    param([System.IO.FileInfo[]]$Files)

    $rows = foreach ($file in $Files) {
        $text = Get-Text $file.FullName
        [pscustomobject]@{
            Path = Get-RelativePath $file.FullName
            Bytes = $file.Length
            Lines = Get-LineCount $text
            SHA256 = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
        }
    }
    return ConvertTo-StableCsv @($rows)
}

function Get-DeviceVariableMap {
    param([string]$Text)

    $map = @{}
    $pattern = 'local\s+(\w+)\s*=\s*devices\s+and\s+devices\.(\w+)'
    foreach ($match in [regex]::Matches($Text, $pattern)) {
        $map[$match.Groups[1].Value] = $match.Groups[2].Value
    }
    return $map
}

function New-DeviceLoadChain {
    $path = Join-Path $BaselineRepoRoot "Cockpit\Scripts\device_init.lua"
    $text = Get-Text $path
    $deviceMap = Get-DeviceVariableMap $text
    $rows = [System.Collections.Generic.List[object]]::new()
    $creatorPattern = '(?s)creators\[(\w+)\]\s*=\s*\{\s*"([^"]+)"\s*,\s*LockOn_Options\.script_path\s*\.\.\s*"([^"]+)"'
    $order = 0
    foreach ($match in [regex]::Matches($text, $creatorPattern)) {
        $order++
        $variable = $match.Groups[1].Value
        $device = if ($deviceMap.ContainsKey($variable)) { $deviceMap[$variable] } else { $variable }
        $rows.Add([pscustomobject]@{
            Order = $order
            Kind = "Device"
            Device = $device
            Class = $match.Groups[2].Value
            Script = "Cockpit/Scripts/" + $match.Groups[3].Value
        })
    }
    $indicatorPattern = 'indicators\[#indicators\s*\+\s*1\]\s*=\s*\{\s*"([^"]+)"\s*,\s*LockOn_Options\.script_path\s*\.\.\s*"([^"]+)"'
    foreach ($match in [regex]::Matches($text, $indicatorPattern)) {
        $order++
        $rows.Add([pscustomobject]@{
            Order = $order
            Kind = "Indicator"
            Device = ""
            Class = $match.Groups[1].Value
            Script = "Cockpit/Scripts/" + $match.Groups[2].Value
        })
    }
    $deviceCount = @($rows | Where-Object Kind -eq "Device").Count
    $indicatorCount = @($rows | Where-Object Kind -eq "Indicator").Count
    if ($deviceCount -ne $ExpectedDeviceCount -or $indicatorCount -ne $ExpectedIndicatorCount) {
        throw "Expected $ExpectedDeviceCount devices and $ExpectedIndicatorCount indicators; found $deviceCount and $indicatorCount."
    }
    return ConvertTo-StableCsv @($rows)
}

function Get-ReferenceSummary {
    param(
        [System.IO.FileInfo[]]$Files,
        [string]$Pattern
    )

    $references = foreach ($file in $Files) {
        $count = [regex]::Matches((Get-Text $file.FullName), $Pattern).Count
        if ($count -gt 0) {
            "$(Get-RelativePath $file.FullName):$count"
        }
    }
    return @($references) -join ";"
}

function Get-CommandReferenceSummary {
    param(
        [System.IO.FileInfo[]]$Files,
        [string]$CommandName
    )

    $references = foreach ($file in $Files) {
        $text = Get-Text $file.FullName
        $aliases = @("device_commands")
        $aliasPattern = 'local\s+(\w+)\s*=\s*device_commands\b'
        $aliases += @([regex]::Matches($text, $aliasPattern) |
            ForEach-Object { $_.Groups[1].Value })
        $owners = @($aliases | Sort-Object -Unique | ForEach-Object { [regex]::Escape($_) })
        $pattern = "\b(?:$($owners -join '|'))\.$([regex]::Escape($CommandName))\b"
        $count = [regex]::Matches($text, $pattern).Count
        if ($count -gt 0) {
            "$(Get-RelativePath $file.FullName):$count"
        }
    }
    return @($references) -join ";"
}

function Get-CockpitTargets {
    param(
        [System.IO.FileInfo[]]$InputFiles,
        [string]$CommandName
    )

    $targets = [System.Collections.Generic.HashSet[string]]::new()
    foreach ($file in $InputFiles) {
        foreach ($line in [System.IO.File]::ReadAllLines($file.FullName)) {
            if ($line -notmatch "device_commands\.$([regex]::Escape($CommandName))\b") {
                continue
            }
            $match = [regex]::Match($line, 'cockpit_device_id\s*=\s*devices\.(\w+)')
            if ($match.Success) {
                [void]$targets.Add($match.Groups[1].Value)
            }
        }
    }
    return @($targets | Sort-Object) -join ";"
}

function New-CommandRouting {
    param([object]$Catalog)

    $inputFiles = @(Get-ChildItem (Join-Path $BaselineRepoRoot "Input\F-CK-1C") -Recurse -File -Filter "*.lua" |
        Sort-Object FullName)
    $cockpitFiles = @(Get-ChildItem (Join-Path $BaselineRepoRoot "Cockpit\Scripts") -Recurse -File -Filter "*.lua" |
        Sort-Object FullName)
    $rows = foreach ($command in $Catalog.commands) {
        [pscustomobject]@{
            Name = $command.name
            Id = $command.value
            Route = $command.route
            CockpitTarget = Get-CockpitTargets $inputFiles $command.name
            InputReferences = Get-CommandReferenceSummary $inputFiles $command.name
            CockpitLuaReferences = Get-CommandReferenceSummary $cockpitFiles $command.name
        }
    }
    return ConvertTo-StableCsv @($rows)
}

function New-DcsActionUsage {
    param([object]$Catalog)

    $files = @(Get-ChildItem (Join-Path $BaselineRepoRoot "Cockpit\Scripts") -Recurse -File -Filter "*.lua")
    $files += @(Get-ChildItem (Join-Path $BaselineRepoRoot "Input\F-CK-1C") -Recurse -File -Filter "*.lua")
    $files = @($files | Sort-Object FullName -Unique)
    $rows = foreach ($command in $Catalog.efm_ignored_dcs_commands) {
        $pattern = "\b$([regex]::Escape($command.name))\b"
        [pscustomobject]@{
            Name = $command.name
            Id = $command.value
            References = Get-ReferenceSummary $files $pattern
            Reason = $command.reason
        }
    }
    return ConvertTo-StableCsv @($rows)
}

function Get-ParamNameMap {
    param([object]$Catalog)

    $map = @{}
    foreach ($parameter in $Catalog.cockpit_params) {
        $map[$parameter.name] = $parameter.value
    }
    return $map
}

function Get-OptionalObjectProperty {
    param(
        [object]$Value,
        [string]$Name
    )

    $property = $Value.PSObject.Properties[$Name]
    if ($null -eq $property) { return $null }
    return $property.Value
}

function Get-HandleAccess {
    param(
        [string]$Text,
        [string]$Variable
    )

    $escaped = [regex]::Escape($Variable)
    $reads = [regex]::Matches($Text, "\b$escaped\s*:\s*get\s*\(").Count
    $reads += [regex]::Matches($Text, "\bnum\s*\(\s*$escaped\s*\)").Count
    $writes = [regex]::Matches($Text, "\b$escaped\s*:\s*set\s*\(").Count
    if ($reads -gt 0 -and $writes -gt 0) { return "ReadWrite" }
    if ($writes -gt 0) { return "Write" }
    if ($reads -gt 0) { return "Read" }
    return "HandleOnly"
}

function Get-LuaHandleRows {
    param(
        [System.IO.FileInfo]$File,
        [hashtable]$ParamMap
    )

    $rows = [System.Collections.Generic.List[object]]::new()
    $text = Get-Text $File.FullName
    $path = Get-RelativePath $File.FullName
    $literalPattern = 'local\s+(\w+)\s*=\s*get_param_handle\("([A-Z0-9_]+)"\)'
    foreach ($match in [regex]::Matches($text, $literalPattern)) {
        $access = Get-HandleAccess $text $match.Groups[1].Value
        $valueReadPattern = [regex]::Escape($match.Value) + '\s*:\s*get\s*\('
        if ($access -eq "HandleOnly" -and $text -match $valueReadPattern) {
            $access = "Read"
        }
        $rows.Add([pscustomobject]@{
            Parameter = $match.Groups[2].Value
            Access = $access
            Layer = "Lua"
            Source = $path
        })
    }
    $generatedPattern = 'local\s+(\w+)\s*=\s*get_param_handle\(cockpit_params\.(\w+)\)'
    foreach ($match in [regex]::Matches($text, $generatedPattern)) {
        $key = $match.Groups[2].Value
        if (-not $ParamMap.ContainsKey($key)) {
            throw "Unknown generated cockpit parameter '$key' in $path."
        }
        $rows.Add([pscustomobject]@{
            Parameter = $ParamMap[$key]
            Access = Get-HandleAccess $text $match.Groups[1].Value
            Layer = "Lua"
            Source = $path
        })
    }
    return @($rows)
}

function Get-DirectAndDynamicParamRows {
    param([System.IO.FileInfo]$File)

    $rows = [System.Collections.Generic.List[object]]::new()
    $text = Get-Text $File.FullName
    $path = Get-RelativePath $File.FullName
    $directPattern = 'get_param_handle\("([A-Z0-9_]+)"\)\s*:\s*get\s*\('
    foreach ($match in [regex]::Matches($text, $directPattern)) {
        $rows.Add([pscustomobject]@{
            Parameter = $match.Groups[1].Value
            Access = "Read"
            Layer = "Lua"
            Source = $path
        })
    }
    foreach ($prefix in @("HMCS_HDG_SLOT_TICK_", "HMCS_HDG_SLOT_LABEL_")) {
        if ($text.Contains('get_param_handle("' + $prefix + '" ..')) {
            $rows.Add([pscustomobject]@{
                Parameter = $prefix + "*"
                Access = "Write"
                Layer = "Lua"
                Source = $path
            })
        }
    }
    return @($rows)
}

function Get-PresentationRows {
    param(
        [object[]]$ExistingRows,
        [System.IO.FileInfo[]]$PresentationFiles
    )

    $existing = [System.Collections.Generic.HashSet[string]]::new()
    foreach ($row in $ExistingRows) {
        [void]$existing.Add("$($row.Parameter)|$($row.Source)")
    }
    $rows = [System.Collections.Generic.List[object]]::new()
    $pattern = '"((?:AP_|AIM9_|HMCS_|RADAR|WS_|FM_)[A-Z0-9_]+)"'
    foreach ($file in $PresentationFiles) {
        $text = Get-Text $file.FullName
        $path = Get-RelativePath $file.FullName
        foreach ($match in [regex]::Matches($text, $pattern)) {
            $parameter = $match.Groups[1].Value
            if ($parameter -in @("HMCS_HDG_SLOT_TICK_", "HMCS_HDG_SLOT_LABEL_")) {
                $parameter += "*"
            }
            if ($existing.Add("$parameter|$path")) {
                $rows.Add([pscustomobject]@{
                    Parameter = $parameter
                    Access = "PresentationRead"
                    Layer = "Lua"
                    Source = $path
                })
            }
        }
    }
    return @($rows)
}

function Get-GeneratedPresentationRows {
    param(
        [object[]]$ExistingRows,
        [System.IO.FileInfo[]]$PresentationFiles,
        [hashtable]$ParamMap
    )

    $existing = [System.Collections.Generic.HashSet[string]]::new()
    foreach ($row in $ExistingRows) {
        [void]$existing.Add("$($row.Parameter)|$($row.Source)")
    }
    $rows = [System.Collections.Generic.List[object]]::new()
    foreach ($file in $PresentationFiles) {
        $text = Get-Text $file.FullName
        $path = Get-RelativePath $file.FullName
        foreach ($match in [regex]::Matches(
            $text,
            '(?<![A-Za-z0-9_])cockpit_params\.([A-Za-z_][A-Za-z0-9_]*)')) {
            $name = $match.Groups[1].Value
            if (-not $ParamMap.ContainsKey($name)) {
                throw "Unknown generated cockpit parameter '$name' in $path."
            }
            $parameter = $ParamMap[$name]
            if ($existing.Add("$parameter|$path")) {
                $rows.Add([pscustomobject]@{
                    Parameter = $parameter
                    Access = "PresentationRead"
                    Layer = "Lua"
                    Source = $path
                })
            }
        }
    }
    return @($rows)
}

function Get-CppParameterRows {
    param([object]$Catalog)

    $rows = [Collections.Generic.List[object]]::new()
    $parametersByName = @{}
    foreach ($parameter in $Catalog.cockpit_params) {
        $parametersByName[[string]$parameter.name] = $parameter
    }
    $cppRoot = Join-Path $BaselineRepoRoot 'src\efm\F-CK-1C_EFM'
    $writerPattern = (
        'cockpit_parameter_writer\s*\(\s*' +
        '(?:DcsIds::)?CockpitParams::([A-Za-z_][A-Za-z0-9_]*)\s*\)')
    foreach ($file in Get-ChildItem $cppRoot -Recurse -File -Include '*.cpp','*.h') {
        $text = Get-Text $file.FullName
        foreach ($match in [regex]::Matches($text, $writerPattern)) {
            $name = $match.Groups[1].Value
            if (-not $parametersByName.ContainsKey($name)) {
                throw "C++ registers unknown cockpit writer parameter: $name"
            }
            $rows.Add([pscustomobject]@{
                Parameter = $parametersByName[$name].value
                Access = "Write"
                Layer = "C++"
                Source = Get-RelativePath $file.FullName
            })
        }
    }
    foreach ($parameter in $Catalog.cockpit_params) {
        $reader = Get-OptionalObjectProperty $parameter 'cpp_reader'
        if ($null -ne $reader) {
            $rows.Add([pscustomobject]@{
                Parameter = $parameter.value
                Access = "Read"
                Layer = "C++"
                Source = $reader
            })
        }
    }
    foreach ($parameter in $Catalog.raw_dcs_cockpit_params) {
        $reader = Get-OptionalObjectProperty $parameter 'cpp_reader'
        if ($null -eq $reader) { continue }
        $rows.Add([pscustomobject]@{
            Parameter = $parameter.value
            Access = "Read"
            Layer = "C++"
            Source = $reader
        })
    }
    return @($rows)
}

function New-ParameterAccess {
    param([object]$Catalog)

    $luaFiles = @(Get-ChildItem (Join-Path $BaselineRepoRoot "Cockpit\Scripts") -Recurse -File -Filter "*.lua" |
        Sort-Object FullName)
    $paramMap = Get-ParamNameMap $Catalog
    $handleRows = @(foreach ($file in $luaFiles) {
        Get-LuaHandleRows $file $paramMap
        Get-DirectAndDynamicParamRows $file
    })
    $presentationFiles = @($luaFiles | Where-Object {
        $path = Get-RelativePath $_.FullName
        $path -like "Cockpit/Scripts/HMCS/*_page.lua" -or
            $path -like "Cockpit/Scripts/ControlsIndicator/*_page.lua"
    })
    $literalPresentationRows = @(
        Get-PresentationRows $handleRows $presentationFiles)
    $generatedPresentationRows = @(Get-GeneratedPresentationRows (
        $handleRows + $literalPresentationRows) $presentationFiles $paramMap)
    $cppRows = @(Get-CppParameterRows $Catalog)
    $ordered = @(
        $handleRows +
        $literalPresentationRows +
        $generatedPresentationRows +
        $cppRows |
        Sort-Object Parameter, Layer, Source, Access -Unique)
    $unknownRows = @($ordered | Where-Object Access -eq "HandleOnly")
    if ($unknownRows.Count -gt 0) {
        $sources = @($unknownRows | ForEach-Object { "$($_.Parameter) in $($_.Source)" })
        throw "Unclassified cockpit parameter handles:`n - $($sources -join "`n - ")"
    }
    return ConvertTo-StableCsv $ordered
}

function Get-ArtifactContents {
    $catalog = Get-Content -LiteralPath $CommandCatalogPath -Raw | ConvertFrom-Json
    $files = Get-BaselineSourceFiles
    return [ordered]@{
        "source-manifest.csv" = New-SourceManifest $files
        "device-load-chain.csv" = New-DeviceLoadChain
        "command-routing.csv" = New-CommandRouting $catalog
        "dcs-action-usage.csv" = New-DcsActionUsage $catalog
        "parameter-access.csv" = New-ParameterAccess $catalog
    }
}

function Write-BaselineArtifacts {
    param([System.Collections.IDictionary]$Artifacts)

    [void](New-Item -ItemType Directory -Path $OutputRoot -Force)
    foreach ($name in $Artifacts.Keys) {
        $path = Join-Path $OutputRoot $name
        [System.IO.File]::WriteAllText($path, $Artifacts[$name], $Utf8NoBom)
        Write-Host "Refreshed $name"
    }
}

function Test-BaselineArtifacts {
    param([System.Collections.IDictionary]$Artifacts)

    $drift = [System.Collections.Generic.List[string]]::new()
    foreach ($name in $Artifacts.Keys) {
        $path = Join-Path $OutputRoot $name
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            $drift.Add("$name is missing")
            continue
        }
        if ((Get-Text $path) -cne $Artifacts[$name]) {
            $drift.Add("$name differs from current sources")
        }
    }
    if ($drift.Count -gt 0) {
        throw "Cockpit baseline drift detected:`n - $($drift -join "`n - ")`nRun with -Mode Refresh after reviewing the source change."
    }
    Write-Host "Cockpit baseline is reproducible: $($Artifacts.Count) artifacts match."
}

$artifacts = Get-ArtifactContents
if ($Mode -eq "Refresh") {
    Write-BaselineArtifacts $artifacts
} else {
    Test-BaselineArtifacts $artifacts
}
