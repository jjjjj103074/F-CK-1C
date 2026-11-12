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
        'Mods\aircraft\F-CK-1C\log\fck1c_efm.log' = 'EFM fixture';
        'Mods\aircraft\F-CK-1C\bin\F-CK-1C_EFM.dll' = 'DLL fixture';
        'Config\Input\F-CK-1C\keyboard\Keyboard.diff.lua' = 'unchanged binding fixture';
    }
    foreach ($relative in $files.Keys) {
        $path = Join-Path $saved $relative
        New-Item -ItemType Directory -Path (Split-Path -Parent $path) -Force | Out-Null
        Set-Content -LiteralPath $path -Value $files[$relative] -Encoding ASCII
    }
    Set-Content -LiteralPath (Join-Path $toolRoot 'BUILD.json') -Value '{"fixture":true}' -Encoding UTF8
    & (Join-Path $toolRoot 'COLLECT-RESULTS.ps1') -SavedGamesRoot $saved
    $zips = @(Get-ChildItem -LiteralPath $toolRoot -Filter 'FCK1C-MC-RESULTS-*.zip')
    if ($zips.Count -ne 1) { throw 'Expected one collection archive.' }
    $unpacked = Join-Path $fixture 'unpacked'
    Expand-Archive -LiteralPath $zips[0].FullName -DestinationPath $unpacked
    $result = @(Get-ChildItem -LiteralPath $unpacked -Directory)[0].FullName
    foreach ($relative in @('MC-recordings\mc-first.csv', 'MC-recordings\mc-second.csv', 'dcs.log', 'EFM-log\fck1c_efm.log', 'keyboard-bindings\Keyboard.diff.lua', 'BUILD.json')) {
        if (-not (Test-Path -LiteralPath (Join-Path $result $relative))) { throw "Missing collected file: $relative" }
    }
    $installed = Get-Content -LiteralPath (Join-Path $result 'installed.json') -Raw | ConvertFrom-Json
    $expected = (Get-FileHash -LiteralPath (Join-Path $saved 'Mods\aircraft\F-CK-1C\bin\F-CK-1C_EFM.dll') -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($installed.dll_sha256 -ne $expected) { throw 'Installed DLL hash mismatch.' }
    if (@($installed.missing) -notcontains 'Config\Input\F-CK-1C\joystick') { throw 'Missing optional evidence was not reported.' }
    Write-Output 'MC flight-test collection fixtures passed.'
}
finally {
    if (Test-Path -LiteralPath $fixture) { Remove-Item -LiteralPath $fixture -Recurse -Force }
}
