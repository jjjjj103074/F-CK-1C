Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$fixture = Join-Path ([System.IO.Path]::GetTempPath()) ('fck1c-mc-collection-' + [guid]::NewGuid().ToString('N'))
try {
    $saved = Join-Path $fixture 'Saved Games with spaces'
    $toolRoot = Join-Path $fixture 'test package'
    New-Item -ItemType Directory -Path $toolRoot -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'COLLECT-RESULTS.ps1') -Destination $toolRoot
    $files = @{
        'Logs\FCK1C-MC-TEST\mc-first.csv' = 'first mission';
        'Logs\FCK1C-MC-TEST\mc-second.csv' = 'second mission';
        'Logs\dcs.log' = 'DCS fixture';
        'Mods\aircraft\F-CK-1C\log\fck1c_efm.log' = @'
[0][INFO] native_cm_input stage=efm_entry callback=ed_fm_set_command command=3040 value=1 value_bits=0x3f800000 finite=1 requests=0 calls=0
[1][INFO] native_cm_input stage=efm_entry callback=ed_fm_set_command command=3150 value=1 value_bits=0x3f800000 finite=1 requests=0 calls=0
[1][INFO] native_cm_input stage=probe_result callback=ed_fm_set_command command=3150 value=1 value_bits=0x3f800000 finite=1 requests=1 calls=0
[2][INFO] native_cm_input stage=efm_entry callback=ed_fm_set_command command=3150 value=0 value_bits=0x00000000 finite=1 requests=1 calls=0
[3][INFO] native_cm_input stage=efm_entry callback=ed_fm_set_command command=3151 value=0 value_bits=0x00000000 finite=1 requests=1 calls=0
[3][INFO] native_cm_input stage=probe_result callback=ed_fm_set_command command=3151 value=0 value_bits=0x00000000 finite=1 requests=1 calls=0
[4][INFO] native_cm_probe reason=direct_call requests=1 calls=1
'@;
        'Mods\aircraft\F-CK-1C\bin\F-CK-1C_EFM.dll' = 'DLL fixture';
        'Config\Input\F-CK-1C\keyboard\Keyboard.diff.lua' = 'unchanged binding fixture';
        'Mods\aircraft\F-CK-1C\Input\F-CK-1C\keyboard\default.lua' = 'installed keyboard fixture';
        'Mods\aircraft\F-CK-1C\Input\F-CK-1C\mission_weapon_input.lua' = 'installed input helper fixture';
        'Mods\aircraft\F-CK-1C\Cockpit\Scripts\command_defs.lua' = 'installed ID fixture';
        'Mods\aircraft\F-CK-1C\Cockpit\Scripts\MCFlightTest\NativeCountermeasureKeys.lua' = 'installed key helper fixture';
    }
    foreach ($relative in $files.Keys) {
        $path = Join-Path $saved $relative
        New-Item -ItemType Directory -Path (Split-Path -Parent $path) -Force | Out-Null
        Set-Content -LiteralPath $path -Value $files[$relative] -Encoding ASCII
    }
    Set-Content -LiteralPath (Join-Path $toolRoot 'BUILD.json') -Value '{"fixture":true}' -Encoding UTF8
    & (Join-Path $toolRoot 'COLLECT-RESULTS.ps1') -SavedGamesRoot $saved
    $zips = @(Get-ChildItem -LiteralPath $toolRoot -Filter 'FCK1C-CM-DIRECT-RESULTS-*.zip')
    if ($zips.Count -ne 1) { throw 'Expected one collection archive.' }
    $unpacked = Join-Path $fixture 'unpacked'
    Expand-Archive -LiteralPath $zips[0].FullName -DestinationPath $unpacked
    $result = @(Get-ChildItem -LiteralPath $unpacked -Directory)[0].FullName
    foreach ($relative in @('MC-recordings\mc-first.csv', 'MC-recordings\mc-second.csv', 'dcs.log', 'EFM-log\fck1c_efm.log', 'keyboard-bindings\Keyboard.diff.lua', 'BUILD.json', 'EFM-COMMAND-TRACE.log', 'INPUT-SUMMARY.json', 'installed-module\Input\F-CK-1C\keyboard\default.lua')) {
        if (-not (Test-Path -LiteralPath (Join-Path $result $relative))) { throw "Missing collected file: $relative" }
    }
    $installed = Get-Content -LiteralPath (Join-Path $result 'installed.json') -Raw | ConvertFrom-Json
    $expected = (Get-FileHash -LiteralPath (Join-Path $saved 'Mods\aircraft\F-CK-1C\bin\F-CK-1C_EFM.dll') -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($installed.dll_sha256 -ne $expected) { throw 'Installed DLL hash mismatch.' }
    if (@($installed.missing) -notcontains 'Config\Input\F-CK-1C\joystick') { throw 'Missing optional evidence was not reported.' }
    if (@($installed.input_files).Count -ne 4) { throw 'Expected hashes for all four installed input scripts.' }
    foreach ($file in $installed.input_files) {
        $copied = Join-Path (Join-Path $result 'installed-module') $file.path
        if ((Get-FileHash -LiteralPath $copied -Algorithm SHA256).Hash.ToLowerInvariant() -ne $file.sha256) {
            throw 'Copied input script hash mismatch.'
        }
    }
    $summary = Get-Content -LiteralPath (Join-Path $result 'INPUT-SUMMARY.json') -Raw | ConvertFrom-Json
    if ($summary.raw_efm_entries -ne 4 -or $summary.direct_call_events -ne 1) { throw 'Wrong trace event counts.' }
    $flare = @($summary.commands | Where-Object { $_.command -eq 3150 })[0]
    $chaff = @($summary.commands | Where-Object { $_.command -eq 3151 })[0]
    if ($flare.efm_entries -ne 2 -or $flare.probe_results -ne 1 -or ($flare.raw_values -join ',') -ne '1,0') {
        throw 'Flare raw values or stages were lost.'
    }
    if ($chaff.efm_entries -ne 1 -or $chaff.raw_values[0] -ne '0') { throw 'Chaff release-only trace was lost.' }
    if (@($installed.missing) -notcontains 'Mods\aircraft\F-CK-1C\FM\native-countermeasure-probe.enabled') {
        throw 'Missing installed input file was not reported.'
    }
    Write-Output 'MC flight-test collection fixtures passed.'
}
finally {
    if (Test-Path -LiteralPath $fixture) { Remove-Item -LiteralPath $fixture -Recurse -Force }
}
