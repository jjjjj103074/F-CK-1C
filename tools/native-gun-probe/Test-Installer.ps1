Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$root = Join-Path ([System.IO.Path]::GetTempPath()) ('fck-native-installer-' + [guid]::NewGuid().ToString('N'))
$package = Join-Path $root 'package'
$dcs = Join-Path $root 'DCS'
$module = Join-Path $dcs 'Mods/aircraft/F-CK-1C'
$source = Join-Path $package 'F-CK-1C'
$checks = 0
function Assert-Check([bool]$Value, [string]$Message) {
    $script:checks++
    if (-not $Value) { throw $Message }
}
try {
    foreach ($path in @("$source/bin", "$source/FM", "$source/TESTING", $module, "$dcs/Logs")) { New-Item -ItemType Directory -Path $path -Force | Out-Null }
    Set-Content "$module/old.cfg" 'previous module' -Encoding ASCII
    Set-Content "$source/entry.lua" 'return true' -Encoding ASCII
    Set-Content "$source/bin/F-CK-1C_EFM.dll" 'fixture DLL bytes' -Encoding ASCII
    Set-Content "$source/FM/native-gun-probe.enabled" '' -Encoding ASCII
    $hash = (Get-FileHash "$source/bin/F-CK-1C_EFM.dll" -Algorithm SHA256).Hash.ToLowerInvariant()
    @{ dll_sha256 = $hash } | ConvertTo-Json | Set-Content "$source/TESTING/NATIVE-GUN-BUILD.json" -Encoding UTF8
    $manager = Join-Path $PSScriptRoot 'Manage.ps1'
    & $manager -Action Install -SavedGamesRoot $dcs -PackageRoot $package
    Assert-Check (Test-Path "$module/FM/native-gun-probe.enabled") 'Probe flag was not installed.'
    $state = Get-Content "$dcs/FCK1C-native-gun-test/state.json" -Raw | ConvertFrom-Json
    Assert-Check (Test-Path (Join-Path $state.backup 'F-CK-1C/old.cfg')) 'Previous module was not backed up.'
    Assert-Check (-not (Test-Path "$dcs/Scripts/Export.lua")) 'Installer unexpectedly created Export.lua.'
    & $manager -Action Install -SavedGamesRoot $dcs -PackageRoot $package
    $again = Get-Content "$dcs/FCK1C-native-gun-test/state.json" -Raw | ConvertFrom-Json
    Assert-Check ($again.backup -eq $state.backup) 'Repeated installation replaced the original backup.'
    New-Item -ItemType Directory -Path "$module/log" -Force | Out-Null
    $binary = Join-Path $root 'CockpitBase.dll'
    Set-Content -LiteralPath $binary 'native fixture binary' -Encoding ASCII
    $hostRecord = @{ event = 'host'; module_path = $binary; matched = $false } | ConvertTo-Json -Compress
    $sample = @{ event = 'sample'; status = 'unsupported_build'; raw_available = $false; raw_count = $null } | ConvertTo-Json -Compress
    $earlierSuccess = '{"event":"sample","raw_available":true,"raw_count":523}'
    $flightStart = '{"event":"flight_start","flight":2}'
    Set-Content "$module/log/fck1c_efm.log" @("native_gun_probe $earlierSuccess", "native_gun_probe $flightStart", "native_gun_probe $hostRecord", "native_gun_probe $sample") -Encoding UTF8
    Set-Content "$module/log/debug.csv" 'sequence,mc_stores_gun_count' -Encoding ASCII
    Set-Content "$dcs/Logs/dcs.log" 'fixture game log' -Encoding ASCII
    & $manager -Action Collect -SavedGamesRoot $dcs -PackageRoot $package
    $zip = Get-ChildItem -LiteralPath $package -Filter 'FCK1C-NATIVE-GUN-RESULTS-*.zip' | Select-Object -Last 1
    $archive = [System.IO.Compression.ZipFile]::OpenRead($zip.FullName)
    try {
        $names = @($archive.Entries | ForEach-Object { $_.FullName.Replace('\', '/') })
        Assert-Check ($names -contains 'CockpitBase.dll') 'Failed native read omitted the binary.'
        Assert-Check ($names -contains 'native-gun-records.json') 'Parsed records are missing.'
        Assert-Check ($names -contains 'Module-Logs/debug.csv') 'MC CSV is missing.'
        Assert-Check ($names -contains 'Build/NATIVE-GUN-BUILD.json') 'Build provenance is missing.'
    }
    finally { $archive.Dispose() }
    $sample = @{ event = 'sample'; status = 'available'; raw_available = $true; raw_count = 0 } | ConvertTo-Json -Compress
    Set-Content "$module/log/fck1c_efm.log" @("native_gun_probe $hostRecord", "native_gun_probe $sample") -Encoding UTF8
    & $manager -Action Collect -SavedGamesRoot $dcs -PackageRoot $package
    $zip = Get-ChildItem -LiteralPath $package -Filter 'FCK1C-NATIVE-GUN-RESULTS-*.zip' | Sort-Object Name | Select-Object -Last 1
    $archive = [System.IO.Compression.ZipFile]::OpenRead($zip.FullName)
    try { Assert-Check ($null -eq $archive.GetEntry('CockpitBase.dll')) 'Successful zero count collected a binary unnecessarily.' }
    finally { $archive.Dispose() }
    & $manager -Action Restore -SavedGamesRoot $dcs -PackageRoot $package
    Assert-Check (Test-Path "$module/old.cfg") 'Previous module was not restored.'
    Assert-Check (-not (Test-Path "$module/FM/native-gun-probe.enabled")) 'Restored module still contains the probe flag.'
    Remove-Item -LiteralPath $module -Recurse -Force
    & $manager -Action Install -SavedGamesRoot $dcs -PackageRoot $package
    Assert-Check (Test-Path "$module/entry.lua") 'First installation failed.'
    & $manager -Action Restore -SavedGamesRoot $dcs -PackageRoot $package
    Assert-Check (-not (Test-Path -LiteralPath $module)) 'Restore did not recover the original absence of a module.'
    Assert-Check (-not (Test-Path "$dcs/FCK1C-native-gun-test/state.json")) 'Restore retained installation state.'
    Write-Output "Native gun installer: $checks checks passed."
}
finally {
    if (Test-Path -LiteralPath $root) { Remove-Item -LiteralPath $root -Recurse -Force }
}
