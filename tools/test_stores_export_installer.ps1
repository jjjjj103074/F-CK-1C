Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = Join-Path ([IO.Path]::GetTempPath()) ('fck1c-stores-export-' + [guid]::NewGuid().ToString('N'))
$checks = 0
function Assert-Condition([bool]$Condition, [string]$Message) {
    $script:checks++
    if (-not $Condition) { throw $Message }
}
try {
    $module = Join-Path $root 'Mods/aircraft/F-CK-1C'
    foreach ($file in @('Export/StoresExport.lua', 'Cockpit/Scripts/Stores/GunAmmunition.lua')) {
        $destination = Join-Path $module $file
        [IO.Directory]::CreateDirectory((Split-Path -Parent $destination)) | Out-Null
        [IO.File]::WriteAllText($destination, '-- fixture')
    }
    $scripts = Join-Path $root 'Scripts'
    [IO.Directory]::CreateDirectory($scripts) | Out-Null
    $export = Join-Path $scripts 'Export.lua'
    $original = [Text.Encoding]::UTF8.GetBytes("-- existing exporter and non-ASCII: 測試`nLuaExportStart = function() end")
    [IO.File]::WriteAllBytes($export, $original)
    $installer = Join-Path $PSScriptRoot 'install_stores_export.ps1'
    & $installer -SavedGamesRoot $root
    $installed = [IO.File]::ReadAllBytes($export)
    Assert-Condition ($installed.Length -gt $original.Length) 'Observer was not installed.'
    Assert-Condition ([Convert]::ToBase64String($installed[0..($original.Length - 1)]) -eq [Convert]::ToBase64String($original)) 'Existing Export bytes changed.'
    & $installer -SavedGamesRoot $root
    Assert-Condition ([Convert]::ToBase64String([IO.File]::ReadAllBytes($export)) -eq [Convert]::ToBase64String($installed)) 'Repeated installation changed Export.'
    & $installer -SavedGamesRoot $root -Action Remove
    Assert-Condition ([Convert]::ToBase64String([IO.File]::ReadAllBytes($export)) -eq [Convert]::ToBase64String($original)) 'Removal changed the original Export.'
    Remove-Item -LiteralPath $export
    & $installer -SavedGamesRoot $root
    Assert-Condition (Test-Path -LiteralPath $export) 'A missing Export was not created.'
    & $installer -SavedGamesRoot $root -Action Remove
    Assert-Condition (-not (Test-Path -LiteralPath $export)) 'Removing the only observer left an empty Export.'
    [IO.File]::WriteAllText($export, '-- FCK1C_STORES_EXPORT_BEGIN edited')
    $failed = $false
    try { & $installer -SavedGamesRoot $root } catch { $failed = $true }
    Assert-Condition $failed 'An edited managed block was overwritten.'
    Assert-Condition ([IO.File]::ReadAllText($export) -eq '-- FCK1C_STORES_EXPORT_BEGIN edited') 'Edited content changed.'
    [IO.File]::WriteAllText($export, '-- UTF-16 fixture', [Text.Encoding]::Unicode)
    $original = [IO.File]::ReadAllBytes($export)
    $failed = $false
    try { & $installer -SavedGamesRoot $root } catch { $failed = $true }
    Assert-Condition $failed 'UTF-16 content was not preserved.'
    Assert-Condition ([Convert]::ToBase64String([IO.File]::ReadAllBytes($export)) -eq [Convert]::ToBase64String($original)) 'UTF-16 bytes changed.'
    Write-Output "Stores Export installer: $checks checks, 0 failures"
} finally {
    if (Test-Path -LiteralPath $root) { Remove-Item -LiteralPath $root -Recurse -Force }
}
