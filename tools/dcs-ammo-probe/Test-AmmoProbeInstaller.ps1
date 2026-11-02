param([Parameter(Mandatory = $true)][string]$PackageRoot)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$script = Join-Path $PackageRoot 'AMMO-TEST/Manage-AmmoProbe.ps1'
$testRoot = Join-Path ([System.IO.Path]::GetTempPath()) ('FCK1C-ammo-install-test-' + [guid]::NewGuid().ToString('N'))
$dcsRoot = Join-Path $testRoot 'DCS'
$scriptsRoot = Join-Path $dcsRoot 'Scripts'
$exportPath = Join-Path $scriptsRoot 'Export.lua'
$utf8 = New-Object System.Text.UTF8Encoding($false)
$checks = 0
function Assert-Condition([bool]$Condition, [string]$Message) {
    $script:checks++
    if (-not $Condition) { throw $Message }
}
function Same-Bytes([byte[]]$A, [byte[]]$B) {
    return [Convert]::ToBase64String($A) -eq [Convert]::ToBase64String($B)
}
try {
    New-Item -ItemType Directory -Path (Join-Path $scriptsRoot 'Hooks') -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $dcsRoot 'Mods/aircraft/F-CK-1C') -Force | Out-Null
    [System.IO.File]::WriteAllText((Join-Path $dcsRoot 'Mods/aircraft/F-CK-1C/previous-module.txt'), 'previous module', $utf8)
    $existing = [byte[]](239, 187, 191) + $utf8.GetBytes("-- previous Export; accented e: $([char]233)`r`nLuaExportStart = function() end`r`n")
    [System.IO.File]::WriteAllBytes($exportPath, $existing)
    [System.IO.File]::WriteAllText((Join-Path $scriptsRoot 'Hooks/other.lua'), '-- unrelated hook', $utf8)
    & $script -Action Install -SavedGamesRoot $dcsRoot -PackageRoot $PackageRoot
    $first = [System.IO.File]::ReadAllBytes($exportPath)
    Assert-Condition (Same-Bytes $existing $first[0..($existing.Length - 1)]) 'Existing Export bytes changed.'
    Assert-Condition (Test-Path -LiteralPath (Join-Path $scriptsRoot 'Hooks/other.lua')) 'Unrelated hook disappeared.'
    Assert-Condition (Test-Path -LiteralPath (Join-Path $dcsRoot 'Mods/aircraft/F-CK-1C/Cockpit/Scripts/Stores/AmmoProbeEnabled.lua')) 'Cockpit observation was not enabled.'
    $backups = @(Get-ChildItem -LiteralPath (Join-Path $dcsRoot 'FCK1C-ammo-backups') -Filter previous-module.txt -File -Recurse)
    Assert-Condition ($backups.Count -eq 1) 'Previous module was not backed up.'
    Assert-Condition (-not (Test-Path -LiteralPath (Join-Path $dcsRoot 'Mods/aircraft/F-CK-1C/previous-module.txt'))) 'Old module leaked into the test module.'
    & $script -Action Install -SavedGamesRoot $dcsRoot -PackageRoot $PackageRoot
    Assert-Condition (Same-Bytes $first ([System.IO.File]::ReadAllBytes($exportPath))) 'Repeated install duplicated the Export block.'
    $addition = $utf8.GetBytes("`r`n-- later user customization`r`n")
    [System.IO.File]::WriteAllBytes($exportPath, $first + $addition)
    New-Item -ItemType Directory -Path (Join-Path $dcsRoot 'Logs/FCK1C-ammo') -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $dcsRoot 'Mods/aircraft/F-CK-1C/log') -Force | Out-Null
    foreach ($path in @('Logs/dcs.log', 'Logs/FCK1C-ammo/export-test.jsonl', 'Logs/FCK1C-ammo/hook-mission-test.jsonl', 'Mods/aircraft/F-CK-1C/log/debug.csv')) {
        [System.IO.File]::WriteAllText((Join-Path $dcsRoot $path), 'fixture', $utf8)
    }
    & $script -Action Collect -SavedGamesRoot $dcsRoot -PackageRoot $PackageRoot
    $collected = @(Get-ChildItem -LiteralPath $PackageRoot -Filter 'FCK1C-AMMO-RESULTS-*.zip' -File)
    Assert-Condition ($collected.Count -ge 1) 'Result archive missing.'
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = [System.IO.Compression.ZipFile]::OpenRead($collected[-1].FullName)
    try {
        $names = @($zip.Entries | ForEach-Object { $_.FullName.Replace('\', '/') })
        Assert-Condition ($names -contains 'DCS-Logs/dcs.log') 'DCS log not collected.'
        Assert-Condition ($names -contains 'Ammo-Probe/export-test.jsonl') 'Export source not collected.'
        Assert-Condition ($names -contains 'Ammo-Probe/hook-mission-test.jsonl') 'Mission source not collected.'
        Assert-Condition ($names -contains 'MC-CSV/debug.csv') 'MC CSV not collected.'
    }
    finally { $zip.Dispose() }
    & $script -Action Remove -SavedGamesRoot $dcsRoot -PackageRoot $PackageRoot
    Assert-Condition (Same-Bytes ($existing + $addition) ([System.IO.File]::ReadAllBytes($exportPath))) 'Removal changed user Export content.'
    Assert-Condition (Test-Path -LiteralPath (Join-Path $scriptsRoot 'Hooks/other.lua')) 'Removal changed another hook.'
    Assert-Condition (-not (Test-Path -LiteralPath (Join-Path $scriptsRoot 'Hooks/FCK1C_AmmoProbe.lua'))) 'Probe hook remained.'
    Assert-Condition (-not (Test-Path -LiteralPath (Join-Path $dcsRoot 'Mods/aircraft/F-CK-1C/Cockpit/Scripts/Stores/AmmoProbeEnabled.lua'))) 'Cockpit observation remained enabled.'
    Assert-Condition (Test-Path -LiteralPath (Join-Path $dcsRoot 'Logs/FCK1C-ammo/export-test.jsonl')) 'Removal deleted test results.'
    Remove-Item -LiteralPath $exportPath
    & $script -Action Install -SavedGamesRoot $dcsRoot -PackageRoot $PackageRoot
    & $script -Action Remove -SavedGamesRoot $dcsRoot -PackageRoot $PackageRoot
    Assert-Condition (-not (Test-Path -LiteralPath $exportPath)) 'A newly created empty Export file remained.'
    [System.IO.File]::WriteAllText($exportPath, "-- FCK1C_AMMO_PROBE_BEGIN`r`n-- edited registration`r`n", $utf8)
    $before = [System.IO.File]::ReadAllBytes($exportPath)
    $rejected = $false
    try { & $script -Action Install -SavedGamesRoot $dcsRoot -PackageRoot $PackageRoot }
    catch { $rejected = $true }
    Assert-Condition $rejected 'Edited registration was silently replaced.'
    Assert-Condition (Same-Bytes $before ([System.IO.File]::ReadAllBytes($exportPath))) 'Rejected install changed Export.lua.'
    Write-Output "Ammo installer: $checks checks, 0 failures"
}
finally {
    if (Test-Path -LiteralPath $testRoot) { Remove-Item -LiteralPath $testRoot -Recurse -Force }
    Get-ChildItem -LiteralPath $PackageRoot -Filter 'FCK1C-AMMO-RESULTS-*.zip' -File | Remove-Item -Force
}
