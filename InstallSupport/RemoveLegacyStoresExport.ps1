<#
.SYNOPSIS
    備份並移除 F-CK-1C 舊掛載匯出與彈量診斷的載入入口。
#>
param([Parameter(Mandatory = $true)][string]$SavedGamesRoot)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (Get-Process -Name DCS -ErrorAction SilentlyContinue) { throw 'Close DCS before changing Export.lua.' }
$dcsRoot = (Resolve-Path -LiteralPath $SavedGamesRoot).Path
$scriptsRoot = Join-Path $dcsRoot 'Scripts'
$exportPath = Join-Path $scriptsRoot 'Export.lua'
$hookPath = Join-Path $scriptsRoot 'Hooks/FCK1C_AmmoProbe.lua'
$runtimeRoot = Join-Path $scriptsRoot 'FCK1CAmmoProbe'
$stores = @'

-- FCK1C_STORES_EXPORT_BEGIN
do
    local ok, err = pcall(dofile, lfs.writedir() .. [[Mods/aircraft/F-CK-1C/Export/StoresExport.lua]])
    if not ok and log and log.write then log.write("FCK1C_STORES_EXPORT", log.ERROR, tostring(err)) end
end
-- FCK1C_STORES_EXPORT_END

'@
$ammo = @'

-- FCK1C_AMMO_PROBE_BEGIN
do
    local ok, err = pcall(dofile, lfs.writedir() .. [[Scripts/FCK1CAmmoProbe/export.lua]])
    if not ok and log and log.write then log.write("FCK1C_AMMO", log.ERROR, tostring(err)) end
end
-- FCK1C_AMMO_PROBE_END

'@
$markers = @('-- FCK1C_STORES_EXPORT_BEGIN', '-- FCK1C_STORES_EXPORT_END',
    '-- FCK1C_AMMO_PROBE_BEGIN', '-- FCK1C_AMMO_PROBE_END')
# 位元組逐一轉換，保存其他匯出器的編碼、換行與內容。
$encoding = [Text.Encoding]::GetEncoding(28591)
$bytes = [byte[]]@()
if (Test-Path -LiteralPath $exportPath) { $bytes = [IO.File]::ReadAllBytes($exportPath) }
if ($bytes.Length -ge 2 -and (($bytes[0] -eq 255 -and $bytes[1] -eq 254) -or ($bytes[0] -eq 254 -and $bytes[1] -eq 255))) {
    $utf16 = if ($bytes[0] -eq 255) { [Text.Encoding]::Unicode } else { [Text.Encoding]::BigEndianUnicode }
    $decoded = $utf16.GetString($bytes)
    foreach ($marker in $markers) {
        if ($decoded.Contains($marker)) { throw 'Legacy F-CK-1C Export block uses UTF-16; existing content was preserved.' }
    }
}
$text = $encoding.GetString($bytes)
$updated = $text
foreach ($block in @($stores, $ammo)) {
    $lf = $block -replace "`r`n", "`n"
    $updated = $updated.Replace(($lf -replace "`n", "`r`n"), '').Replace($lf, '')
}
foreach ($marker in $markers) {
    if ($updated.Contains($marker)) { throw 'A legacy F-CK-1C Export block was edited; existing content was preserved.' }
}
$hookBytes = $null
if (Test-Path -LiteralPath $hookPath) {
    $hookBytes = [IO.File]::ReadAllBytes($hookPath)
    $hookText = [Text.Encoding]::UTF8.GetString($hookBytes).TrimStart([char]0xfeff) -replace "`r`n", "`n"
    $expectedHook = 'local ok, err = pcall(dofile, lfs.writedir() .. [[Scripts/FCK1CAmmoProbe/hook.lua]])' + "`n" +
        'if not ok and log and log.write then log.write("FCK1C_AMMO", log.ERROR, tostring(err)) end' + "`n"
    if ($hookText -ne $expectedHook) { throw 'The legacy ammo hook was edited; existing files were preserved.' }
}
$runtimeFiles = @()
$statePath = Join-Path $runtimeRoot 'state.json'
if (Test-Path -LiteralPath $statePath) {
    $state = Get-Content -LiteralPath $statePath -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($state.PSObject.Properties['version'] -and $state.version -eq 'ammo-probe-r1') {
        foreach ($name in @('host.lua', 'export.lua', 'hook.lua', 'mission-query.lua', 'AmmoProbe.lua', 'state.json')) {
            $path = Join-Path $runtimeRoot $name
            if (Test-Path -LiteralPath $path -PathType Leaf) { $runtimeFiles += $path }
        }
    }
}
if ($updated -eq $text -and $null -eq $hookBytes -and $runtimeFiles.Count -eq 0) {
    Write-Output 'Legacy F-CK-1C Export and ammo probe are already removed.'
    return
}
$backupRoot = Join-Path $dcsRoot 'FCK1C-stores-backups'
[IO.Directory]::CreateDirectory($backupRoot) | Out-Null
$stamp = (Get-Date -Format 'yyyyMMdd-HHmmss-fff') + '-' + [guid]::NewGuid().ToString('N')
if ($updated -ne $text) {
    [IO.File]::WriteAllBytes((Join-Path $backupRoot ($stamp + '.lua')), $bytes)
    if ($updated.Length -eq 0) { Remove-Item -LiteralPath $exportPath }
    else { [IO.File]::WriteAllBytes($exportPath, $encoding.GetBytes($updated)) }
}
if ($null -ne $hookBytes) {
    [IO.File]::WriteAllBytes((Join-Path $backupRoot ($stamp + '-FCK1C_AmmoProbe.lua')), $hookBytes)
    Remove-Item -LiteralPath $hookPath
}
if ($runtimeFiles.Count) {
    $runtimeBackup = Join-Path $backupRoot ($stamp + '-FCK1CAmmoProbe')
    [IO.Directory]::CreateDirectory($runtimeBackup) | Out-Null
    foreach ($path in $runtimeFiles) { Move-Item -LiteralPath $path -Destination $runtimeBackup }
    if (@(Get-ChildItem -LiteralPath $runtimeRoot -Force).Count -eq 0) { Remove-Item -LiteralPath $runtimeRoot }
}
Write-Output 'Legacy F-CK-1C Export and ammo probe removed; original files backed up.'
