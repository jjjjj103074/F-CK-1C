param(
    [ValidateSet('Install', 'Collect', 'Remove')]
    [string]$Action = 'Install',
    [string]$SavedGamesRoot,
    [string]$PackageRoot = (Split-Path -Parent $PSScriptRoot)
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$utf8 = New-Object System.Text.UTF8Encoding($false)
$latin = [System.Text.Encoding]::GetEncoding(28591)
$begin = '-- FCK1C_AMMO_PROBE_BEGIN'
$end = '-- FCK1C_AMMO_PROBE_END'
$exportBlock = @'

-- FCK1C_AMMO_PROBE_BEGIN
do
    local ok, err = pcall(dofile, lfs.writedir() .. [[Scripts/FCK1CAmmoProbe/export.lua]])
    if not ok and log and log.write then log.write("FCK1C_AMMO", log.ERROR, tostring(err)) end
end
-- FCK1C_AMMO_PROBE_END

'@ -replace "`r?`n", "`r`n"

function Get-SavedGamesRoot {
    if ($SavedGamesRoot) { return [System.IO.Path]::GetFullPath($SavedGamesRoot) }
    $savedGames = Join-Path ([Environment]::GetFolderPath('UserProfile')) 'Saved Games'
    $registry = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders'
    $settings = Get-ItemProperty -LiteralPath $registry -ErrorAction SilentlyContinue
    if ($settings) {
        $property = $settings.PSObject.Properties['{4C5C32FF-BB9D-43B0-B5B4-2D72E54EAAA4}']
        if ($property -and $property.Value) { $savedGames = [Environment]::ExpandEnvironmentVariables($property.Value) }
    }
    $roots = @(Get-ChildItem -LiteralPath $savedGames -Directory -ErrorAction SilentlyContinue | Where-Object { $_.Name -like 'DCS*' })
    if ($roots.Count -eq 1) { return $roots[0].FullName }
    if ($roots.Count -gt 1) {
        for ($i = 0; $i -lt $roots.Count; $i++) { Write-Host "[$($i + 1)] $($roots[$i].FullName)" }
        $choice = 0
        if (-not [int]::TryParse((Read-Host 'Choose the DCS Saved Games folder number'), [ref]$choice) -or $choice -lt 1 -or $choice -gt $roots.Count) {
            throw 'Invalid DCS folder selection.'
        }
        return $roots[$choice - 1].FullName
    }
    throw 'DCS Saved Games folder not found. Run this script with -SavedGamesRoot <your DCS folder>.'
}

function Read-ExportBytes {
    if (-not (Test-Path -LiteralPath $exportPath)) { return '' }
    $bytes = [System.IO.File]::ReadAllBytes($exportPath)
    if ($bytes.Length -ge 2 -and (($bytes[0] -eq 255 -and $bytes[1] -eq 254) -or ($bytes[0] -eq 254 -and $bytes[1] -eq 255))) {
        throw 'Export.lua uses UTF-16. No changes made; provide this file for a compatible installation.'
    }
    return $latin.GetString($bytes)
}

function Write-ExportBytes([string]$Text) {
    [System.IO.File]::WriteAllBytes($exportPath, $latin.GetBytes($Text))
}

$dcsRoot = Get-SavedGamesRoot
if (-not (Test-Path -LiteralPath $dcsRoot -PathType Container)) { throw "DCS folder not found: $dcsRoot" }
$scriptsRoot = Join-Path $dcsRoot 'Scripts'
$runtimeRoot = Join-Path $scriptsRoot 'FCK1CAmmoProbe'
$exportPath = Join-Path $scriptsRoot 'Export.lua'
$hookPath = Join-Path $scriptsRoot 'Hooks/FCK1C_AmmoProbe.lua'
$modRoot = Join-Path $dcsRoot 'Mods/aircraft/F-CK-1C'
$statePath = Join-Path $runtimeRoot 'state.json'
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'

if ($Action -eq 'Install') {
    if (Get-Process -Name DCS -ErrorAction SilentlyContinue) { throw 'Close DCS before installing the test package.' }
    $moduleSource = Join-Path $PackageRoot 'F-CK-1C'
    $runtimeSource = Join-Path $PSScriptRoot 'runtime'
    if ([System.IO.Path]::GetFullPath($moduleSource).TrimEnd('\', '/') -eq [System.IO.Path]::GetFullPath($modRoot).TrimEnd('\', '/')) {
        throw 'Extract this test package outside Mods/aircraft, then run Install.cmd again. Existing module preserved.'
    }
    foreach ($file in @('entry.lua', 'bin/F-CK-1C_EFM.dll', 'Cockpit/Scripts/Stores/AmmoProbe.lua', 'Cockpit/Scripts/Stores/AmmoProbeEnabled.lua')) {
        if (-not (Test-Path -LiteralPath (Join-Path $moduleSource $file) -PathType Leaf)) { throw "Incomplete module: $file" }
    }
    foreach ($file in @('host.lua', 'export.lua', 'hook.lua', 'mission-query.lua')) {
        if (-not (Test-Path -LiteralPath (Join-Path $runtimeSource $file) -PathType Leaf)) { throw "Incomplete diagnostic runtime: $file" }
    }
    $currentExport = Read-ExportBytes
    if (($currentExport.Contains($begin) -or $currentExport.Contains($end)) -and -not $currentExport.Contains($exportBlock)) {
        throw 'An edited probe block exists in Export.lua. No changes made; provide the file for review.'
    }
    if ((Test-Path -LiteralPath $hookPath) -and -not ([System.IO.File]::ReadAllText($hookPath).Contains('FCK1CAmmoProbe/hook.lua'))) {
        throw 'A different script already uses FCK1C_AmmoProbe.lua. No changes made.'
    }
    $backupRoot = Join-Path $dcsRoot "FCK1C-ammo-backups/$stamp"
    New-Item -ItemType Directory -Path $backupRoot -Force | Out-Null
    $exportExisted = Test-Path -LiteralPath $exportPath
    if ($exportExisted) { Copy-Item -LiteralPath $exportPath -Destination (Join-Path $backupRoot 'Export.lua') }
    if (Test-Path -LiteralPath $statePath) {
        $oldState = Get-Content -LiteralPath $statePath -Raw -Encoding UTF8 | ConvertFrom-Json
        $exportExisted = [bool]$oldState.export_existed
    }
    if (Test-Path -LiteralPath $modRoot) {
        Move-Item -LiteralPath $modRoot -Destination (Join-Path $backupRoot 'F-CK-1C')
    }
    New-Item -ItemType Directory -Path $modRoot -Force | Out-Null
    Get-ChildItem -LiteralPath $moduleSource -Force | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $modRoot -Recurse -Force }
    New-Item -ItemType Directory -Path $runtimeRoot -Force | Out-Null
    foreach ($file in @('host.lua', 'export.lua', 'hook.lua', 'mission-query.lua')) {
        Copy-Item -LiteralPath (Join-Path $runtimeSource $file) -Destination (Join-Path $runtimeRoot $file) -Force
    }
    Copy-Item -LiteralPath (Join-Path $moduleSource 'Cockpit/Scripts/Stores/AmmoProbe.lua') -Destination (Join-Path $runtimeRoot 'AmmoProbe.lua') -Force
    New-Item -ItemType Directory -Path (Split-Path -Parent $hookPath) -Force | Out-Null
    $hook = 'local ok, err = pcall(dofile, lfs.writedir() .. [[Scripts/FCK1CAmmoProbe/hook.lua]])' + "`n" + 'if not ok and log and log.write then log.write("FCK1C_AMMO", log.ERROR, tostring(err)) end' + "`n"
    [System.IO.File]::WriteAllText($hookPath, $hook, $utf8)
    if (-not $currentExport.Contains($exportBlock)) { Write-ExportBytes ($currentExport + $exportBlock) }
    @{ version = 'ammo-probe-r1'; export_existed = $exportExisted; backup = $backupRoot } | ConvertTo-Json | Set-Content -LiteralPath $statePath -Encoding UTF8
    Write-Host "Installed test module and all diagnostic routes: $dcsRoot"
    Write-Host "Previous module and Export.lua backup: $backupRoot"
}
elseif ($Action -eq 'Collect') {
    $stage = Join-Path ([System.IO.Path]::GetTempPath()) "FCK1C-AMMO-RESULTS-$stamp"
    New-Item -ItemType Directory -Path $stage -Force | Out-Null
    $sources = @(
        @{ directory = (Join-Path $dcsRoot 'Logs'); filter = 'dcs.log*'; target = 'DCS-Logs' },
        @{ directory = (Join-Path $dcsRoot 'Logs/FCK1C-ammo'); filter = '*.jsonl'; target = 'Ammo-Probe' },
        @{ directory = (Join-Path $modRoot 'log'); filter = 'debug.csv*'; target = 'MC-CSV' },
        @{ directory = (Join-Path $modRoot 'TESTING'); filter = '*BUILD*.json'; target = 'Build' }
    )
    foreach ($source in $sources) {
        if (-not (Test-Path -LiteralPath $source.directory)) { continue }
        $target = Join-Path $stage $source.target
        New-Item -ItemType Directory -Path $target -Force | Out-Null
        Get-ChildItem -LiteralPath $source.directory -Filter $source.filter -File | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $target -Force }
    }
    $zipPath = Join-Path $PackageRoot "FCK1C-AMMO-RESULTS-$stamp.zip"
    Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zipPath -CompressionLevel Optimal
    Remove-Item -LiteralPath $stage -Recurse -Force
    Write-Host "Send this results file: $zipPath"
}
else {
    if (Get-Process -Name DCS -ErrorAction SilentlyContinue) { throw 'Close DCS before removing the probe.' }
    $exportText = Read-ExportBytes
    if (($exportText.Contains($begin) -or $exportText.Contains($end)) -and -not $exportText.Contains($exportBlock)) {
        throw 'The probe registration was edited. Files preserved; provide Export.lua for review.'
    }
    if ($exportText.Contains($exportBlock)) {
        $cleaned = $exportText.Replace($exportBlock, '')
        $existed = $true
        if (Test-Path -LiteralPath $statePath) {
            $state = Get-Content -LiteralPath $statePath -Raw -Encoding UTF8 | ConvertFrom-Json
            $existed = [bool]$state.export_existed
        }
        if (-not $existed -and $cleaned.Length -eq 0) { Remove-Item -LiteralPath $exportPath }
        else { Write-ExportBytes $cleaned }
    }
    if ((Test-Path -LiteralPath $hookPath) -and ([System.IO.File]::ReadAllText($hookPath).Contains('FCK1CAmmoProbe/hook.lua'))) { Remove-Item -LiteralPath $hookPath }
    $enabledPath = Join-Path $modRoot 'Cockpit/Scripts/Stores/AmmoProbeEnabled.lua'
    if (Test-Path -LiteralPath $enabledPath) { Remove-Item -LiteralPath $enabledPath }
    foreach ($file in @('host.lua', 'export.lua', 'hook.lua', 'mission-query.lua', 'AmmoProbe.lua', 'state.json')) {
        $path = Join-Path $runtimeRoot $file
        if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path }
    }
    Write-Host 'Diagnostic hooks removed. Existing Export settings and test results preserved.'
}
