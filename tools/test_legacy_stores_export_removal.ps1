Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = Join-Path ([IO.Path]::GetTempPath()) ('fck-legacy-export-' + [guid]::NewGuid().ToString('N'))
$checks = 0
function Assert-Check([bool]$Value, [string]$Message) {
    $script:checks++
    if (-not $Value) { throw $Message }
}
try {
    [IO.Directory]::CreateDirectory((Join-Path $root 'Scripts')) | Out-Null
    $file = Join-Path $root 'Scripts/Export.lua'
    $remover = Join-Path $PSScriptRoot '../InstallSupport/RemoveLegacyStoresExport.ps1'
    & $remover -SavedGamesRoot $root
    Assert-Check (-not (Test-Path -LiteralPath $file)) 'Absent Export was created.'
    $block = @'

-- FCK1C_STORES_EXPORT_BEGIN
do
    local ok, err = pcall(dofile, lfs.writedir() .. [[Mods/aircraft/F-CK-1C/Export/StoresExport.lua]])
    if not ok and log and log.write then log.write("FCK1C_STORES_EXPORT", log.ERROR, tostring(err)) end
end
-- FCK1C_STORES_EXPORT_END

'@ -replace "`r?`n", "`r`n"
    $original = [Text.Encoding]::UTF8.GetBytes("-- Another exporter: $([char]0x6e2c)$([char]0x8a66)`nLuaExportStart = function() end")
    $combined = [byte[]]($original + [Text.Encoding]::ASCII.GetBytes($block))
    [IO.File]::WriteAllBytes($file, $combined)
    & $remover -SavedGamesRoot $root
    Assert-Check ([Convert]::ToBase64String([IO.File]::ReadAllBytes($file)) -eq [Convert]::ToBase64String($original)) 'Other exporter bytes changed.'
    $backups = @(Get-ChildItem (Join-Path $root 'FCK1C-stores-backups') -File)
    Assert-Check ($backups.Count -eq 1) 'Original Export was not backed up.'
    Assert-Check ([Convert]::ToBase64String([IO.File]::ReadAllBytes($backups[0].FullName)) -eq [Convert]::ToBase64String($combined)) 'Backup differs from original.'
    & $remover -SavedGamesRoot $root
    Assert-Check ([Convert]::ToBase64String([IO.File]::ReadAllBytes($file)) -eq [Convert]::ToBase64String($original)) 'Repeated cleanup changed another exporter.'
    [IO.File]::WriteAllText($file, $block, [Text.Encoding]::ASCII)
    & $remover -SavedGamesRoot $root
    Assert-Check (-not (Test-Path -LiteralPath $file)) 'Only legacy block left an empty Export.'
    $edited = '-- FCK1C_STORES_EXPORT_BEGIN edited'
    [IO.File]::WriteAllText($file, $edited)
    $failed = $false
    try { & $remover -SavedGamesRoot $root } catch { $failed = $true }
    Assert-Check $failed 'Edited legacy block was overwritten.'
    Assert-Check ([IO.File]::ReadAllText($file) -eq $edited) 'Edited legacy bytes changed.'
    $mixed = $block + $edited
    [IO.File]::WriteAllText($file, $mixed)
    $failed = $false
    try { & $remover -SavedGamesRoot $root } catch { $failed = $true }
    Assert-Check $failed 'A mixed original and edited legacy block was partially changed.'
    Assert-Check ([IO.File]::ReadAllText($file) -eq $mixed) 'Mixed legacy blocks changed.'
    [IO.File]::WriteAllText($file, '-- unrelated UTF-16 exporter', [Text.Encoding]::Unicode)
    $bytes = [IO.File]::ReadAllBytes($file)
    & $remover -SavedGamesRoot $root
    Assert-Check ([Convert]::ToBase64String([IO.File]::ReadAllBytes($file)) -eq [Convert]::ToBase64String($bytes)) 'Unrelated UTF-16 exporter changed.'
    $ammoBlock = @'

-- FCK1C_AMMO_PROBE_BEGIN
do
    local ok, err = pcall(dofile, lfs.writedir() .. [[Scripts/FCK1CAmmoProbe/export.lua]])
    if not ok and log and log.write then log.write("FCK1C_AMMO", log.ERROR, tostring(err)) end
end
-- FCK1C_AMMO_PROBE_END

'@ -replace "`r?`n", "`r`n"
    $hookText = 'local ok, err = pcall(dofile, lfs.writedir() .. [[Scripts/FCK1CAmmoProbe/hook.lua]])' + "`n" +
        'if not ok and log and log.write then log.write("FCK1C_AMMO", log.ERROR, tostring(err)) end' + "`n"
    $hook = Join-Path $root 'Scripts/Hooks/FCK1C_AmmoProbe.lua'
    [IO.Directory]::CreateDirectory((Split-Path -Parent $hook)) | Out-Null
    $runtime = Join-Path $root 'Scripts/FCK1CAmmoProbe'
    [IO.Directory]::CreateDirectory($runtime) | Out-Null
    [IO.File]::WriteAllText((Join-Path $runtime 'state.json'), '{"version":"ammo-probe-r1"}')
    [IO.File]::WriteAllText((Join-Path $runtime 'export.lua'), '-- old probe runtime')
    [IO.File]::WriteAllText((Join-Path $runtime 'custom.txt'), 'preserve custom notes')
    [IO.File]::WriteAllText($hook, $hookText, [Text.UTF8Encoding]::new($false))
    $combined = [byte[]]($original + [Text.Encoding]::ASCII.GetBytes($block + $ammoBlock))
    [IO.File]::WriteAllBytes($file, $combined)
    & $remover -SavedGamesRoot $root
    Assert-Check ([Convert]::ToBase64String([IO.File]::ReadAllBytes($file)) -eq [Convert]::ToBase64String($original)) 'Ammo cleanup changed another exporter.'
    Assert-Check (-not (Test-Path -LiteralPath $hook)) 'Old ammo GameGUI hook remained active.'
    Assert-Check (-not (Test-Path -LiteralPath (Join-Path $runtime 'export.lua'))) 'Old ammo runtime was not archived.'
    Assert-Check ([IO.File]::ReadAllText((Join-Path $runtime 'custom.txt')) -eq 'preserve custom notes') 'Custom probe notes changed.'
    $archived = @(Get-ChildItem (Join-Path $root 'FCK1C-stores-backups') -Recurse -Filter 'export.lua' -File)
    Assert-Check ($archived.Count -eq 1 -and [IO.File]::ReadAllText($archived[0].FullName) -eq '-- old probe runtime') 'Ammo runtime backup missing.'
    & $remover -SavedGamesRoot $root
    Assert-Check ([Convert]::ToBase64String([IO.File]::ReadAllBytes($file)) -eq [Convert]::ToBase64String($original)) 'Repeated ammo cleanup changed another exporter.'
    $editedAmmo = $block + '-- FCK1C_AMMO_PROBE_BEGIN edited'
    [IO.File]::WriteAllText($file, $editedAmmo)
    $failed = $false
    try { & $remover -SavedGamesRoot $root } catch { $failed = $true }
    Assert-Check $failed 'Edited ammo block was overwritten.'
    Assert-Check ([IO.File]::ReadAllText($file) -eq $editedAmmo) 'Stores block was partially removed before rejecting edited ammo.'
    [IO.File]::WriteAllText($file, $ammoBlock)
    [IO.File]::WriteAllText($hook, '-- custom hook with different behavior')
    $failed = $false
    try { & $remover -SavedGamesRoot $root } catch { $failed = $true }
    Assert-Check $failed 'Custom hook was overwritten.'
    Assert-Check ([IO.File]::ReadAllText($file) -eq $ammoBlock) 'Export changed before checking the custom hook.'
    Remove-Item -LiteralPath $hook
    [IO.File]::WriteAllText($file, ($ammoBlock -replace "`r`n", "`n"), [Text.Encoding]::ASCII)
    & $remover -SavedGamesRoot $root
    Assert-Check (-not (Test-Path -LiteralPath $file)) 'LF ammo block remained active.'
    Write-Output "Legacy Export cleanup: $checks checks, 0 failures"
}
finally {
    if (Test-Path -LiteralPath $root) { Remove-Item -LiteralPath $root -Recurse -Force }
}
