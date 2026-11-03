param(
    [ValidateSet('Install', 'Collect', 'Restore')][string]$Action = 'Install',
    [string]$SavedGamesRoot,
    [string]$PackageRoot = (Split-Path -Parent $PSScriptRoot)
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not $SavedGamesRoot) {
    $saved = Join-Path ([Environment]::GetFolderPath('UserProfile')) 'Saved Games'
    $settings = Get-ItemProperty 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders' -ErrorAction SilentlyContinue
    if ($settings) {
        $property = $settings.PSObject.Properties['{4C5C32FF-BB9D-43B0-B5B4-2D72E54EAAA4}']
        if ($property -and $property.Value) { $saved = [Environment]::ExpandEnvironmentVariables($property.Value) }
    }
    $roots = @(Get-ChildItem -LiteralPath $saved -Directory | Where-Object { $_.Name -like 'DCS*' })
    if ($roots.Count -eq 1) { $SavedGamesRoot = $roots[0].FullName }
    elseif ($roots.Count -gt 1) {
        for ($i = 0; $i -lt $roots.Count; $i++) { Write-Host "[$($i + 1)] $($roots[$i].FullName)" }
        $choice = 0
        if (-not [int]::TryParse((Read-Host 'DCS Saved Games folder number'), [ref]$choice) -or $choice -lt 1 -or $choice -gt $roots.Count) { throw 'Invalid selection.' }
        $SavedGamesRoot = $roots[$choice - 1].FullName
    }
    else { throw 'DCS Saved Games folder not found.' }
}
$dcs = [System.IO.Path]::GetFullPath($SavedGamesRoot)
if (-not (Test-Path -LiteralPath $dcs -PathType Container)) { throw 'DCS Saved Games folder does not exist.' }
$module = Join-Path $dcs 'Mods/aircraft/F-CK-1C'
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$state = Join-Path $dcs 'FCK1C-native-gun-test/state.json'

if ($Action -eq 'Install') {
    if (Get-Process -Name DCS -ErrorAction SilentlyContinue) { throw 'Close DCS before installing.' }
    $source = Join-Path $PackageRoot 'F-CK-1C'
    foreach ($name in @('entry.lua', 'bin/F-CK-1C_EFM.dll', 'FM/native-gun-probe.enabled', 'TESTING/NATIVE-GUN-BUILD.json')) {
        if (-not (Test-Path -LiteralPath (Join-Path $source $name) -PathType Leaf)) { throw "Incomplete package: $name" }
    }
    if ([System.IO.Path]::GetFullPath($source).TrimEnd('\', '/') -eq [System.IO.Path]::GetFullPath($module).TrimEnd('\', '/')) { throw 'Extract the package outside the installed module.' }
    $manifest = Get-Content (Join-Path $source 'TESTING/NATIVE-GUN-BUILD.json') -Raw | ConvertFrom-Json
    $actual = (Get-FileHash (Join-Path $source 'bin/F-CK-1C_EFM.dll') -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $manifest.dll_sha256) { throw 'Test DLL hash mismatch.' }
    if (Test-Path -LiteralPath $state) {
        $installed = Join-Path $module 'bin/F-CK-1C_EFM.dll'
        if ((Test-Path -LiteralPath $installed) -and
            (Test-Path -LiteralPath (Join-Path $module 'FM/native-gun-probe.enabled')) -and
            (Get-FileHash -LiteralPath $installed -Algorithm SHA256).Hash.ToLowerInvariant() -eq $actual) {
            Write-Host 'This diagnostic package is already installed.'
            return
        }
        throw 'Restore the previous diagnostic installation before installing a different package.'
    }
    $backup = Join-Path $dcs "FCK1C-native-gun-backups/$stamp"
    New-Item -ItemType Directory -Path $backup -Force | Out-Null
    if (Test-Path -LiteralPath $module) { Move-Item -LiteralPath $module -Destination (Join-Path $backup 'F-CK-1C') }
    try {
        New-Item -ItemType Directory -Path $module -Force | Out-Null
        Get-ChildItem -LiteralPath $source -Force | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $module -Recurse -Force }
    }
    catch {
        if (Test-Path -LiteralPath $module) { Remove-Item -LiteralPath $module -Recurse -Force }
        if (Test-Path (Join-Path $backup 'F-CK-1C')) { Move-Item (Join-Path $backup 'F-CK-1C') $module }
        throw
    }
    New-Item -ItemType Directory -Path (Split-Path -Parent $state) -Force | Out-Null
    @{ backup = $backup; module = $module } | ConvertTo-Json | Set-Content -LiteralPath $state -Encoding UTF8
    Write-Host "Installed native gun test: $module"
    Write-Host "Previous module: $backup"
}
elseif ($Action -eq 'Collect') {
    $stage = Join-Path ([System.IO.Path]::GetTempPath()) "FCK1C-NATIVE-GUN-$stamp"
    New-Item -ItemType Directory -Path $stage -Force | Out-Null
    foreach ($item in @(
        @{ path = (Join-Path $module 'log'); filter = '*'; name = 'Module-Logs' },
        @{ path = (Join-Path $dcs 'Logs'); filter = 'dcs.log*'; name = 'DCS-Logs' },
        @{ path = (Join-Path $module 'TESTING'); filter = 'NATIVE-GUN-BUILD.json'; name = 'Build' }
    )) {
        if (-not (Test-Path -LiteralPath $item.path)) { continue }
        $target = Join-Path $stage $item.name
        New-Item -ItemType Directory -Path $target -Force | Out-Null
        Get-ChildItem -LiteralPath $item.path -File -Filter $item.filter | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $target }
    }
    $records = @()
    $log = Join-Path $module 'log/fck1c_efm.log'
    if (Test-Path -LiteralPath $log) {
        foreach ($line in Get-Content -LiteralPath $log -Encoding UTF8) {
            $index = $line.IndexOf('native_gun_probe ')
            if ($index -ge 0 -and $line.Substring($index + 17).StartsWith('{')) { $records += ($line.Substring($index + 17) | ConvertFrom-Json) }
        }
    }
    ConvertTo-Json -InputObject @($records) -Depth 8 | Set-Content (Join-Path $stage 'native-gun-records.json') -Encoding UTF8
    $hostRecord = @($records | Where-Object { $_.event -eq 'host' } | Select-Object -Last 1)
    $binary = ''
    if ($hostRecord.Count -eq 1) { $binary = [string]$hostRecord[0].module_path }
    if (-not $binary) {
        $dcsLog = Join-Path $dcs 'Logs/dcs.log'
        if (Test-Path -LiteralPath $dcsLog) {
            foreach ($line in Get-Content -LiteralPath $dcsLog -Encoding UTF8) {
                if ($line -match 'Loaded (.+[\\/]CockpitBase\.dll)') { $binary = $Matches[1] }
            }
        }
    }
    if ($binary -and (Test-Path -LiteralPath $binary -PathType Leaf)) {
        @{ path = $binary; sha256 = (Get-FileHash -LiteralPath $binary -Algorithm SHA256).Hash.ToLowerInvariant(); version = (Get-Item -LiteralPath $binary).VersionInfo.FileVersion } |
            ConvertTo-Json | Set-Content (Join-Path $stage 'cockpitbase-version.json') -Encoding UTF8
        $samples = @($records | Where-Object { $_.event -eq 'sample' })
        if (@($samples | Where-Object { $_.raw_available }).Count -eq 0) {
            Copy-Item -LiteralPath $binary -Destination (Join-Path $stage 'CockpitBase.dll')
        }
    }
    $zip = Join-Path $PackageRoot "FCK1C-NATIVE-GUN-RESULTS-$stamp.zip"
    Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -CompressionLevel Optimal
    Remove-Item -LiteralPath $stage -Recurse -Force
    Write-Host "Send this results file: $zip"
}
else {
    if (Get-Process -Name DCS -ErrorAction SilentlyContinue) { throw 'Close DCS before restoring.' }
    if (-not (Test-Path -LiteralPath $state)) { throw 'Installation state not found.' }
    $previous = Get-Content -LiteralPath $state -Raw | ConvertFrom-Json
    $backupModule = Join-Path $previous.backup 'F-CK-1C'
    if (-not (Test-Path -LiteralPath $backupModule)) { throw 'Previous module backup not found; installed module preserved.' }
    if (Test-Path -LiteralPath $module) {
        $kept = Join-Path $dcs "FCK1C-native-gun-backups/test-$stamp"
        Move-Item -LiteralPath $module -Destination $kept
    }
    Move-Item -LiteralPath $backupModule -Destination $module
    Remove-Item -LiteralPath $state
    Write-Host 'Previous module restored.'
}
