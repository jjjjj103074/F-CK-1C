param(
    [string]$SavedGamesRoot = (Join-Path $env:USERPROFILE 'Saved Games\DCS'),
    [switch]$AllRecordings
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$watch = [Diagnostics.Stopwatch]::StartNew()
$SavedGamesRoot = [System.IO.Path]::GetFullPath($SavedGamesRoot)
if (-not (Test-Path -LiteralPath $SavedGamesRoot -PathType Container)) {
    throw "DCS Saved Games folder not found: $SavedGamesRoot"
}
$build = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'BUILD.json') -Raw | ConvertFrom-Json
$revision = [string]$build.runtime_source_commit
if ($revision -notmatch '^[0-9a-f]{40}$') { throw 'BUILD.json must identify the runtime source revision.' }
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
# 依測試包版本選取飛行紀錄，明確回報收集範圍與資料量。
$recordingRoot = Join-Path $SavedGamesRoot 'Logs\FCK1C-MC-TEST'
$pattern = if ($AllRecordings) { '*.csv' } else { 'mc-' + $revision.Substring(0, 12) + '-*.csv' }
$selected = @()
if (Test-Path -LiteralPath $recordingRoot) {
    $selected = @(Get-ChildItem -LiteralPath $recordingRoot -Filter $pattern -File | Sort-Object Name)
}
$totalBytes = 0
if ($selected.Count) { $totalBytes = ($selected | Measure-Object Length -Sum).Sum }
Write-Output ('Collecting {0} recording(s), {1:N1} MiB.' -f $selected.Count, ($totalBytes / 1MB))
$recordings = @()
if ($selected.Count) {
    $destination = Join-Path $target 'MC-recordings'
    New-Item -ItemType Directory -Path $destination | Out-Null
    foreach ($path in $selected) {
        Copy-Item -LiteralPath $path.FullName -Destination $destination
        $reader = $null
        $sample = $false
        $errorText = $null
        try {
            $reader = [IO.StreamReader]::new($path.FullName, [Text.Encoding]::UTF8, $true)
            $null = $reader.ReadLine()
            while (-not $reader.EndOfStream) {
                if (-not [string]::IsNullOrWhiteSpace($reader.ReadLine())) { $sample = $true; break }
            }
        }
        catch { $errorText = $_.Exception.Message }
        finally { if ($reader) { $reader.Dispose() } }
        $status = if ($errorText) { 'read_error' } elseif ($sample) { 'samples_present' } else { 'header_only' }
        $recordings += [ordered]@{ file = $path.Name; sample_present = $sample; size_bytes = $path.Length; status = $status; error = $errorText }
    }
} else {
    $missing.Add('Current bundle flight recordings')
}
Copy-Evidence 'Logs\dcs.log' 'dcs.log'
Copy-Evidence 'Logs\dcs.log.old' 'dcs.log.old'
Copy-Evidence 'Mods\aircraft\F-CK-1C\log' 'EFM-log'
Copy-Evidence 'Config\Input\F-CK-1C\keyboard' 'keyboard-bindings'
Copy-Evidence 'Config\Input\F-CK-1C\joystick' 'joystick-bindings'
ConvertTo-Json -InputObject @($recordings) -Depth 4 | Set-Content -LiteralPath (Join-Path $target 'recording-health.json') -Encoding UTF8
$module = Join-Path $SavedGamesRoot 'Mods\aircraft\F-CK-1C'
$installedDll = Join-Path $module 'bin\F-CK-1C_EFM.dll'
$installed = [ordered]@{
    saved_games_root = $SavedGamesRoot; collected_at = (Get-Date).ToString('o'); missing = $missing.ToArray()
    recording_scope = $(if ($AllRecordings) { 'all' } else { 'current_bundle' })
    runtime_source_commit = $revision; recording_files = $selected.Count; recording_bytes = $totalBytes
}
if (Test-Path -LiteralPath $installedDll) {
    $installed.dll_sha256 = (Get-FileHash -LiteralPath $installedDll -Algorithm SHA256).Hash.ToLowerInvariant()
}
$installed | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $target 'installed.json') -Encoding UTF8
foreach ($name in @('BUILD.json', 'RESULTS.csv')) {
    $path = Join-Path $PSScriptRoot $name
    if (Test-Path -LiteralPath $path) { Copy-Item -LiteralPath $path -Destination $target }
}
$archive = "$target.zip"
Write-Output 'Compressing selected evidence...'
Compress-Archive -LiteralPath $target -DestinationPath $archive -CompressionLevel Fastest
Write-Output "Results: $archive"
Write-Output ('Completed in {0:N1} seconds.' -f $watch.Elapsed.TotalSeconds)
if ($missing.Count) { Write-Output ('Missing evidence: ' + ($missing -join ', ')) }
