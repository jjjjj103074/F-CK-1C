param([string]$SavedGamesRoot = (Join-Path $env:USERPROFILE 'Saved Games\DCS'))
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$SavedGamesRoot = [System.IO.Path]::GetFullPath($SavedGamesRoot)
if (-not (Test-Path -LiteralPath $SavedGamesRoot -PathType Container)) {
    throw "DCS Saved Games folder not found: $SavedGamesRoot"
}
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$target = Join-Path $PSScriptRoot "FCK1C-CM-DIRECT-RESULTS-$stamp"
New-Item -ItemType Directory -Path $target | Out-Null
$missing = [System.Collections.Generic.List[string]]::new()
function Copy-Evidence([string]$Relative, [string]$Name) {
    $source = Join-Path $SavedGamesRoot $Relative
    if (Test-Path -LiteralPath $source) {
        Copy-Item -LiteralPath $source -Destination (Join-Path $target $Name) -Recurse
    } else {
        $missing.Add($Relative)
    }
}
Copy-Evidence 'Logs\FCK1C-MC-TEST' 'MC-recordings'
Copy-Evidence 'Logs\dcs.log' 'dcs.log'
Copy-Evidence 'Logs\dcs.log.old' 'dcs.log.old'
Copy-Evidence 'Mods\aircraft\F-CK-1C\log' 'EFM-log'
Copy-Evidence 'Config\Input\F-CK-1C\keyboard' 'keyboard-bindings'
Copy-Evidence 'Config\Input\F-CK-1C\joystick' 'joystick-bindings'
$module = Join-Path $SavedGamesRoot 'Mods\aircraft\F-CK-1C'
$installedDll = Join-Path $module 'bin\F-CK-1C_EFM.dll'
$installed = [ordered]@{ saved_games_root = $SavedGamesRoot; collected_at = (Get-Date).ToString('o'); missing = $missing.ToArray() }
if (Test-Path -LiteralPath $installedDll) {
    $installed.dll_sha256 = (Get-FileHash -LiteralPath $installedDll -Algorithm SHA256).Hash.ToLowerInvariant()
}
$installed.input_files = @()
foreach ($relative in @('Input\F-CK-1C\keyboard\default.lua', 'Input\F-CK-1C\mission_weapon_input.lua',
    'Cockpit\Scripts\command_defs.lua', 'Cockpit\Scripts\MCFlightTest\NativeCountermeasureKeys.lua',
    'FM\native-countermeasure-probe.enabled')) {
    $source = Join-Path $module $relative
    if (Test-Path -LiteralPath $source -PathType Leaf) {
        $destination = Join-Path (Join-Path $target 'installed-module') $relative
        New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
        Copy-Item -LiteralPath $source -Destination $destination
        $installed.input_files += [ordered]@{
            path = $relative.Replace('\', '/')
            sha256 = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash.ToLowerInvariant()
        }
    } else {
        $missing.Add('Mods\aircraft\F-CK-1C\' + $relative)
    }
}
$efmLog = Join-Path $module 'log\fck1c_efm.log'
$trace = @()
if (Test-Path -LiteralPath $efmLog -PathType Leaf) {
    $trace = @(Get-Content -LiteralPath $efmLog -Encoding UTF8 | Where-Object { $_ -match 'native_cm_(input|probe) ' })
}
($trace -join [Environment]::NewLine) | Set-Content -LiteralPath (Join-Path $target 'EFM-COMMAND-TRACE.log') -Encoding UTF8
$entries = @($trace | Where-Object { $_ -match 'native_cm_input stage=efm_entry ' })
$commands = @()
foreach ($id in @(3040, 3150, 3151)) {
    $pattern = ' command=' + $id + ' value=(\S+)'
    $received = @($entries | Where-Object { $_ -match $pattern })
    $values = @($received | ForEach-Object { [regex]::Match($_, $pattern).Groups[1].Value })
    $results = @($trace | Where-Object { $_ -match ('native_cm_input stage=probe_result .* command=' + $id + ' ') })
    $commands += [ordered]@{ command = $id; efm_entries = $received.Count; raw_values = $values; probe_results = $results.Count }
}
[ordered]@{
    raw_efm_entries = $entries.Count
    commands = $commands
    direct_call_events = @($trace | Where-Object { $_ -match 'native_cm_probe reason=direct_call ' }).Count
    trace_file = 'EFM-COMMAND-TRACE.log'
} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $target 'INPUT-SUMMARY.json') -Encoding UTF8
$installed.missing = $missing.ToArray()
$installed | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $target 'installed.json') -Encoding UTF8
foreach ($name in @('BUILD.json', 'RESULTS.csv')) {
    $path = Join-Path $PSScriptRoot $name
    if (Test-Path -LiteralPath $path) { Copy-Item -LiteralPath $path -Destination $target }
}
$archive = "$target.zip"
Compress-Archive -LiteralPath $target -DestinationPath $archive -CompressionLevel Optimal
Write-Output "Results: $archive"
if ($missing.Count) { Write-Output ('Missing evidence: ' + ($missing -join ', ')) }
