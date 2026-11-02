param(
    [Parameter(Mandatory = $true)][string]$RepositoryRoot,
    [Parameter(Mandatory = $true)][string]$OverlayRoot,
    [Parameter(Mandatory = $true)][string]$ArtifactDirectory,
    [Parameter(Mandatory = $true)][string]$OverlayCommit
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$baseline = '32db27850f4e503e980aacd280782fa25d2a3f4b'
$repoRoot = [System.IO.Path]::GetFullPath($RepositoryRoot)
$overlay = [System.IO.Path]::GetFullPath($OverlayRoot)
$overlayHead = (& git -C $overlay rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $overlayHead -ne $OverlayCommit) { throw 'Diagnostic source revision mismatch.' }
$baseTree = (& git -C $repoRoot rev-parse 'HEAD:src/efm').Trim()
if ($LASTEXITCODE -ne 0) { throw 'Unable to read baseline EFM source.' }
$overlayTree = (& git -C $overlay rev-parse 'HEAD:src/efm').Trim()
if ($LASTEXITCODE -ne 0 -or $overlayTree -ne $baseTree) { throw 'Diagnostic overlay changes the validated EFM source.' }
$version = 'ammo-probe-r1-' + $OverlayCommit.Substring(0, 8)
& (Join-Path $overlay 'tools/package_mc_stores_test.ps1') -RepositoryRoot $repoRoot -Version $version -ArtifactDirectory $ArtifactDirectory -SourceCommit $baseline
$bundle = Join-Path $repoRoot "dist/$version"
New-Item -ItemType Directory -Path $bundle -Force | Out-Null
$module = Join-Path $bundle 'F-CK-1C'
Copy-Item -LiteralPath (Join-Path $repoRoot 'dist/F-CK-1C') -Destination $module -Recurse
$overlays = @(
    'Cockpit/Scripts/Stores/AmmoProbe.lua',
    'Cockpit/Scripts/Systems/cms_system.lua',
    'Cockpit/Scripts/Systems/weapon_system.lua'
)
foreach ($file in $overlays) { Copy-Item -LiteralPath (Join-Path $overlay $file) -Destination (Join-Path $module $file) -Force }
Copy-Item -LiteralPath (Join-Path $overlay 'tools/dcs-ammo-probe/runtime/enabled.lua') -Destination (Join-Path $module 'Cockpit/Scripts/Stores/AmmoProbeEnabled.lua') -Force
$probeRoot = Join-Path $bundle 'AMMO-TEST'
New-Item -ItemType Directory -Path $probeRoot -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $overlay 'tools/dcs-ammo-probe/runtime') -Destination (Join-Path $probeRoot 'runtime') -Recurse
Copy-Item -LiteralPath (Join-Path $overlay 'tools/dcs-ammo-probe/Manage-AmmoProbe.ps1') -Destination $probeRoot
foreach ($file in @('Install.cmd', 'Collect.cmd', 'Remove.cmd')) { Copy-Item -LiteralPath (Join-Path $overlay "tools/dcs-ammo-probe/$file") -Destination (Join-Path $bundle $file) }
Copy-Item -LiteralPath (Join-Path $overlay 'docs/DCS_AMMO_PROBE_GUIDE.zh-TW.html') -Destination (Join-Path $bundle 'START-HERE.zh-TW.html')
Copy-Item -LiteralPath (Join-Path $overlay 'docs/DCS_AMMO_PROBE_GUIDE.zh-TW.html') -Destination (Join-Path $module 'TESTING/START-HERE.zh-TW.html') -Force
Copy-Item -LiteralPath (Join-Path $overlay 'docs/DCS_AMMO_PROBE_GUIDE.zh-TW.md') -Destination (Join-Path $bundle 'TESTING.zh-TW.md')
$manifest = Get-Content -LiteralPath (Join-Path $ArtifactDirectory 'manifest.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$hashes = [ordered]@{}
Get-ChildItem -LiteralPath $bundle -Recurse -File | ForEach-Object {
    $relative = $_.FullName.Substring($bundle.Length + 1).Replace('\', '/')
    $hashes[$relative] = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
}
$build = [ordered]@{
    package = $version
    diagnostic_source_commit = $OverlayCommit
    aircraft_baseline_commit = $baseline
    validated_efm_tree = $baseTree
    dll_sha256 = $manifest.dll_sha256
    dll_validation_run = $manifest.run_id
    dcs_runtime = 'awaiting_user_capture'
    files_sha256 = $hashes
}
$build | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $bundle 'AMMO-BUILD.json') -Encoding UTF8
Copy-Item -LiteralPath (Join-Path $bundle 'AMMO-BUILD.json') -Destination (Join-Path $module 'TESTING/AMMO-BUILD.json') -Force
& (Join-Path $overlay 'tools/dcs-ammo-probe/Test-AmmoProbeInstaller.ps1') -PackageRoot $bundle

Add-Type -AssemblyName System.IO.Compression.FileSystem
Add-Type -AssemblyName System.IO.Compression
$zipPath = Join-Path $repoRoot "dist/F-CK-1C-$version.zip"
if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath }
$archive = [System.IO.Compression.ZipFile]::Open($zipPath, [System.IO.Compression.ZipArchiveMode]::Create)
try {
    foreach ($file in Get-ChildItem -LiteralPath $bundle -Recurse -File) {
        $name = $file.FullName.Substring($bundle.Length + 1).Replace('\', '/')
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, $file.FullName, $name, [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
}
finally { $archive.Dispose() }
$archive = [System.IO.Compression.ZipFile]::OpenRead($zipPath)
try {
    foreach ($required in @('Install.cmd', 'Collect.cmd', 'Remove.cmd', 'START-HERE.zh-TW.html', 'AMMO-BUILD.json', 'AMMO-TEST/Manage-AmmoProbe.ps1', 'AMMO-TEST/runtime/hook.lua', 'AMMO-TEST/runtime/export.lua', 'AMMO-TEST/runtime/mission-query.lua', 'F-CK-1C/Cockpit/Scripts/Stores/AmmoProbeEnabled.lua')) {
        if ($null -eq $archive.GetEntry($required)) { throw "Missing test entry: $required" }
    }
    foreach ($relative in $hashes.Keys) {
        $entry = $archive.GetEntry($relative)
        if ($null -eq $entry) { throw "Missing module entry: $relative" }
        $stream = $entry.Open()
        $sha = [System.Security.Cryptography.SHA256]::Create()
        try {
            $actual = [System.BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '').ToLowerInvariant()
            if ($actual -ne $hashes[$relative]) { throw "Archive hash mismatch: $relative" }
        }
        finally { $stream.Dispose(); $sha.Dispose() }
    }
}
finally { $archive.Dispose() }
$zipHash = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
Set-Content -LiteralPath "$zipPath.sha256" -Value "$zipHash *$(Split-Path -Leaf $zipPath)" -Encoding ASCII
Write-Output "Validated ammunition test package: $zipPath"
Write-Output "Diagnostic source: $OverlayCommit; DLL SHA256: $($manifest.dll_sha256); ZIP SHA256: $zipHash"
