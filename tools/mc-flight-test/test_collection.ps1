Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$fixture = Join-Path ([System.IO.Path]::GetTempPath()) ('fck1c-mc-collection-' + [guid]::NewGuid().ToString('N'))
try {
    $saved = Join-Path $fixture 'Saved Games with spaces'
    $toolRoot = Join-Path $fixture 'test package'
    New-Item -ItemType Directory -Path $toolRoot -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'COLLECT-RESULTS.ps1') -Destination $toolRoot
    $files = @{
        'Mods\aircraft\F-CK-1C\log\flight-tests\mc-aaaaaaaaaaaa-first.csv' = '"sample","model_time_s"';
        'Mods\aircraft\F-CK-1C\log\flight-tests\mc-aaaaaaaaaaaa-second.csv' = '"sample","model_time_s"' + "`r`n" + '"1","1.0"';
        'Mods\aircraft\F-CK-1C\log\flight-tests\mc-old.csv' = 'history fixture';
        'Mods\aircraft\F-CK-1C\log\flight-tests\mc-bbbbbbbbbbbb-old.csv' = 'other bundle fixture';
        'Logs\dcs.log' = 'DCS fixture';
        'Mods\aircraft\F-CK-1C\log\fck1c_efm.log' = 'EFM fixture';
        'Mods\aircraft\F-CK-1C\bin\F-CK-1C_EFM.dll' = 'DLL fixture';
        'Config\Input\F-CK-1C\keyboard\Keyboard.diff.lua' = 'unchanged binding fixture';
    }
    foreach ($relative in $files.Keys) {
        $path = Join-Path $saved $relative
        New-Item -ItemType Directory -Path (Split-Path -Parent $path) -Force | Out-Null
        Set-Content -LiteralPath $path -Value $files[$relative] -Encoding ASCII
    }
    $sourceLogHash = (Get-FileHash -LiteralPath (Join-Path $saved 'Logs\dcs.log') -Algorithm SHA256).Hash
    Set-Content -LiteralPath (Join-Path $toolRoot 'BUILD.json') -Value '{"runtime_source_commit":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"}' -Encoding UTF8
    & (Join-Path $toolRoot 'COLLECT-RESULTS.ps1') -SavedGamesRoot $saved
    $outputRoot = Join-Path $saved 'Mods\aircraft\F-CK-1C\log\collections'
    $zips = @(Get-ChildItem -LiteralPath $outputRoot -Filter 'FCK1C-MC-RESULTS-*.zip')
    if ($zips.Count -ne 1) { throw 'Expected one collection archive.' }
    $unpacked = Join-Path $fixture 'unpacked'
    Expand-Archive -LiteralPath $zips[0].FullName -DestinationPath $unpacked
    $result = @(Get-ChildItem -LiteralPath $unpacked -Directory)[0].FullName
    foreach ($relative in @('MC-recordings\mc-aaaaaaaaaaaa-first.csv', 'MC-recordings\mc-aaaaaaaaaaaa-second.csv', 'dcs.log', 'EFM-log\fck1c_efm.log', 'keyboard-bindings\Keyboard.diff.lua', 'BUILD.json')) {
        if (-not (Test-Path -LiteralPath (Join-Path $result $relative))) { throw "Missing collected file: $relative" }
    }
    if (Test-Path -LiteralPath (Join-Path $result 'MC-recordings\mc-old.csv')) { throw 'Historical recordings were collected.' }
    if (Test-Path -LiteralPath (Join-Path $result 'MC-recordings\mc-bbbbbbbbbbbb-old.csv')) { throw 'Another bundle was collected.' }
    $installed = Get-Content -LiteralPath (Join-Path $result 'installed.json') -Raw | ConvertFrom-Json
    $expected = (Get-FileHash -LiteralPath (Join-Path $saved 'Mods\aircraft\F-CK-1C\bin\F-CK-1C_EFM.dll') -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($installed.dll_sha256 -ne $expected) { throw 'Installed DLL hash mismatch.' }
    if (@($installed.missing) -notcontains 'Config\Input\F-CK-1C\joystick') { throw 'Missing optional evidence was not reported.' }
    $health = Get-Content -LiteralPath (Join-Path $result 'recording-health.json') -Raw | ConvertFrom-Json
    if ($health.Count -ne 2 -or $health[0].status -ne 'header_only' -or $health[0].sample_present) { throw 'Empty recorder was not diagnosed.' }
    if ($health[1].status -ne 'samples_present' -or -not $health[1].sample_present) { throw 'Recorder sample count is incorrect.' }
    if ($installed.recording_scope -ne 'current_bundle' -or $installed.recording_files -ne 2) { throw 'Collection scope was not reported.' }
    & (Join-Path $toolRoot 'COLLECT-RESULTS.ps1') -SavedGamesRoot $saved -AllRecordings
    $all = @(Get-ChildItem -LiteralPath $outputRoot -Directory -Filter 'FCK1C-MC-RESULTS-*' | Sort-Object Name)[-1].FullName
    if (@(Get-ChildItem -LiteralPath (Join-Path $all 'MC-recordings') -Filter '*.csv').Count -ne 4) { throw 'Explicit history collection omitted records.' }
    Set-Content -LiteralPath (Join-Path $toolRoot 'BUILD.json') -Value '{"runtime_source_commit":"cccccccccccccccccccccccccccccccccccccccc"}' -Encoding UTF8
    & (Join-Path $toolRoot 'COLLECT-RESULTS.ps1') -SavedGamesRoot $saved
    $empty = @(Get-ChildItem -LiteralPath $outputRoot -Directory -Filter 'FCK1C-MC-RESULTS-*' | Sort-Object Name)[-1].FullName
    $emptyInfo = Get-Content -LiteralPath (Join-Path $empty 'installed.json') -Raw | ConvertFrom-Json
    if ($emptyInfo.recording_files -ne 0 -or @($emptyInfo.missing) -notcontains 'Current bundle flight recordings') { throw 'Missing current recordings were not diagnosed.' }
    if (-not (Test-Path -LiteralPath (Join-Path $empty 'dcs.log'))) { throw 'A registration failure must still return DCS logs.' }
    if ((Get-FileHash -LiteralPath (Join-Path $saved 'Logs\dcs.log') -Algorithm SHA256).Hash -ne $sourceLogHash) { throw 'Collection modified the DCS log.' }
    if (@(Get-ChildItem -LiteralPath $toolRoot -Filter 'FCK1C-MC-RESULTS-*').Count) { throw 'Collection output was written outside the module.' }
    Write-Output 'MC flight-test collection fixtures passed.'
}
finally {
    if (Test-Path -LiteralPath $fixture) { Remove-Item -LiteralPath $fixture -Recurse -Force }
}
