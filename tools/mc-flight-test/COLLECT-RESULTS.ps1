param([string]$SavedGamesRoot = (Join-Path $env:USERPROFILE 'Saved Games\DCS'))
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$SavedGamesRoot = [System.IO.Path]::GetFullPath($SavedGamesRoot)
if (-not (Test-Path -LiteralPath $SavedGamesRoot -PathType Container)) {
    throw "DCS Saved Games folder not found: $SavedGamesRoot"
}
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$target = Join-Path $PSScriptRoot "FCK1C-MC-RESULTS-$stamp"
New-Item -ItemType Directory -Path $target | Out-Null
$missing = [System.Collections.Generic.List[string]]::new()
function Copy-Evidence([string]$Relative, [string]$Name) {
    $source = Join-Path $SavedGamesRoot $Relative
    if (Test-Path -LiteralPath $source) {
        Copy-Item -LiteralPath $source -Destination (Join-Path $target $Name) -Recurse
    } else {
        $missing.Add($Relative)
    }
}
Copy-Evidence 'Logs\FCK1C-MC-TEST' 'MC-recordings'
Copy-Evidence 'Logs\dcs.log' 'dcs.log'
Copy-Evidence 'Logs\dcs.log.old' 'dcs.log.old'
Copy-Evidence 'Mods\aircraft\F-CK-1C\log' 'EFM-log'
Copy-Evidence 'Config\Input\F-CK-1C\keyboard' 'keyboard-bindings'
Copy-Evidence 'Config\Input\F-CK-1C\joystick' 'joystick-bindings'
$module = Join-Path $SavedGamesRoot 'Mods\aircraft\F-CK-1C'
$installedDll = Join-Path $module 'bin\F-CK-1C_EFM.dll'
$installed = [ordered]@{ saved_games_root = $SavedGamesRoot; collected_at = (Get-Date).ToString('o'); missing = $missing.ToArray() }
if (Test-Path -LiteralPath $installedDll) {
    $installed.dll_sha256 = (Get-FileHash -LiteralPath $installedDll -Algorithm SHA256).Hash.ToLowerInvariant()
}
$installed | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $target 'installed.json') -Encoding UTF8
foreach ($name in @('BUILD.json', 'RESULTS.csv')) {
    $path = Join-Path $PSScriptRoot $name
    if (Test-Path -LiteralPath $path) { Copy-Item -LiteralPath $path -Destination $target }
}
$archive = "$target.zip"
Compress-Archive -LiteralPath $target -DestinationPath $archive -CompressionLevel Optimal
Write-Output "Results: $archive"
if ($missing.Count) { Write-Output ('Missing evidence: ' + ($missing -join ', ')) }
