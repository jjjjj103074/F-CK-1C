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
    Write-Output "Legacy Export cleanup: $checks checks, 0 failures"
}
finally {
    if (Test-Path -LiteralPath $root) { Remove-Item -LiteralPath $root -Recurse -Force }
}
