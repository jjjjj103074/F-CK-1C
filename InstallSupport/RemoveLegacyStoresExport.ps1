<#
.SYNOPSIS
    備份 Export.lua 並移除舊 F-CK-1C 彈量載入區塊。
#>
param(
    [Parameter(Mandatory = $true)][string]$SavedGamesRoot
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
    $utf16 = if ($bytes[0] -eq 255) { [Text.Encoding]::Unicode } else { [Text.Encoding]::BigEndianUnicode }
    $decoded = $utf16.GetString($bytes)
    if (-not $decoded.Contains($begin) -and -not $decoded.Contains($end)) { return }
    throw 'Legacy F-CK-1C stores block uses UTF-16; remove that block before installation.'
}
$text = $encoding.GetString($bytes)
$remaining = $text.Replace($block, '')
if ($remaining.Contains($begin) -or $remaining.Contains($end)) {
    throw 'The existing F-CK-1C stores block was edited; existing content was preserved.'
}
if (-not $text.Contains($block)) { Write-Output 'Legacy F-CK-1C stores Export is already removed.'; return }
$updated = $text.Replace($block, '')
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
Write-Output 'Legacy F-CK-1C stores Export removed.'
