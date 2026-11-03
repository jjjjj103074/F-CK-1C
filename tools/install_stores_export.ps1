<#
.SYNOPSIS
    在既有 DCS Export.lua 安裝或移除 F-CK-1C 掛載觀測器。
#>
param(
    [Parameter(Mandatory = $true)][string]$SavedGamesRoot,
    [ValidateSet('Install', 'Remove')][string]$Action = 'Install'
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (Get-Process -Name DCS -ErrorAction SilentlyContinue) { throw 'Close DCS before changing Export.lua.' }
$dcsRoot = (Resolve-Path -LiteralPath $SavedGamesRoot).Path
$scriptsRoot = Join-Path $dcsRoot 'Scripts'
$exportPath = Join-Path $scriptsRoot 'Export.lua'
$begin = '-- FCK1C_STORES_EXPORT_BEGIN'
$end = '-- FCK1C_STORES_EXPORT_END'
$block = @'

-- FCK1C_STORES_EXPORT_BEGIN
do
    local ok, err = pcall(dofile, lfs.writedir() .. [[Mods/aircraft/F-CK-1C/Export/StoresExport.lua]])
    if not ok and log and log.write then log.write("FCK1C_STORES_EXPORT", log.ERROR, tostring(err)) end
end
-- FCK1C_STORES_EXPORT_END

'@ -replace "`r?`n", "`r`n"
# 位元組逐一轉換，保存既有 Export 的文字編碼及其他匯出器內容。
$encoding = [Text.Encoding]::GetEncoding(28591)
$bytes = [byte[]]@()
if (Test-Path -LiteralPath $exportPath) { $bytes = [IO.File]::ReadAllBytes($exportPath) }
if ($bytes.Length -ge 2 -and (($bytes[0] -eq 255 -and $bytes[1] -eq 254) -or ($bytes[0] -eq 254 -and $bytes[1] -eq 255))) {
    throw 'Export.lua uses UTF-16; existing content was preserved.'
}
$text = $encoding.GetString($bytes)
if (($text.Contains($begin) -or $text.Contains($end)) -and -not $text.Contains($block)) {
    throw 'The existing F-CK-1C stores block was edited; existing content was preserved.'
}
if ($Action -eq 'Install') {
    foreach ($file in @('Export/StoresExport.lua', 'Cockpit/Scripts/Stores/GunAmmunition.lua')) {
        if (-not (Test-Path -LiteralPath (Join-Path $dcsRoot "Mods/aircraft/F-CK-1C/$file") -PathType Leaf)) {
            throw "Install the matching F-CK-1C module first: $file"
        }
    }
    if ($text.Contains($block)) { Write-Output 'F-CK-1C stores Export is already installed.'; return }
    $updated = $text + $block
} else {
    if (-not $text.Contains($block)) { Write-Output 'F-CK-1C stores Export is already removed.'; return }
    $updated = $text.Replace($block, '')
}
if ($bytes.Length) {
    $backupRoot = Join-Path $dcsRoot 'FCK1C-stores-backups'
    [IO.Directory]::CreateDirectory($backupRoot) | Out-Null
    $backup = Join-Path $backupRoot ((Get-Date -Format 'yyyyMMdd-HHmmss-fff') + '-' + [guid]::NewGuid().ToString('N') + '.lua')
    [IO.File]::WriteAllBytes($backup, $bytes)
}
if ($updated.Length -eq 0) {
    if (Test-Path -LiteralPath $exportPath) { Remove-Item -LiteralPath $exportPath }
} else {
    [IO.Directory]::CreateDirectory($scriptsRoot) | Out-Null
    [IO.File]::WriteAllBytes($exportPath, $encoding.GetBytes($updated))
}
Write-Output "F-CK-1C stores Export: $Action completed."
