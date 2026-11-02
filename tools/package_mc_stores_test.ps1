param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[A-Za-z0-9._-]+$')]
    [string]$Version,
    [Parameter(Mandatory = $true)]
    [string]$ArtifactDirectory,
    [Parameter(Mandatory = $true)]
    [string]$SourceCommit
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$head = (& git -C $repoRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $head -ne $SourceCommit) { throw 'Source commit mismatch.' }
$tree = (& git -C $repoRoot rev-parse 'HEAD^{tree}').Trim()
if ($LASTEXITCODE -ne 0) { throw 'Unable to read source tree.' }
$manifestPath = Join-Path $ArtifactDirectory 'manifest.json'
$manifest = Get-Content -LiteralPath $manifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
$dllPath = Join-Path $ArtifactDirectory 'F-CK-1C_EFM.dll'
$dllHash = (Get-FileHash -LiteralPath $dllPath -Algorithm SHA256).Hash.ToLowerInvariant()
if ($manifest.source_commit -ne $SourceCommit -or $manifest.source_tree -ne $tree -or
    $manifest.dll_sha256 -ne $dllHash -or $manifest.validation -ne 'passed') {
    throw 'Validated source/DLL manifest mismatch.'
}
Copy-Item -LiteralPath $dllPath -Destination (Join-Path $repoRoot 'bin/F-CK-1C_EFM.dll') -Force
Copy-Item -LiteralPath $manifestPath -Destination (Join-Path $repoRoot 'bin/F-CK-1C_EFM.validation.json') -Force
& (Join-Path $PSScriptRoot 'package_release.ps1') -Version $Version -AllowDirty
$stage = Join-Path $repoRoot 'dist/F-CK-1C'
$testing = Join-Path $stage 'TESTING'
New-Item -ItemType Directory -Path $testing -Force | Out-Null
$guide = Get-Content -LiteralPath (Join-Path $repoRoot 'docs/MC_STORES_PACKAGE_GUIDE.zh-TW.html') -Raw -Encoding UTF8
$guide = $guide.Replace('@@SOURCE_COMMIT@@', $SourceCommit).Replace('@@DLL_SHA256@@', $dllHash)
$guide | Set-Content -LiteralPath (Join-Path $testing 'START-HERE.zh-TW.html') -Encoding UTF8
Copy-Item -LiteralPath $manifestPath -Destination (Join-Path $testing 'BUILD.json') -Force
$zipPath = Join-Path $repoRoot "dist/F-CK-1C-$Version.zip"
Compress-Archive -Path $stage -DestinationPath $zipPath -CompressionLevel Optimal -Force
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [System.IO.Compression.ZipFile]::OpenRead($zipPath)
try {
    foreach ($relative in @('bin/F-CK-1C_EFM.dll', 'bin/F-CK-1C_EFM.validation.json',
        'Cockpit/Scripts/Stores/ObservationDiagnostics.lua', 'Cockpit/Scripts/Stores/StationDefinitions.lua',
        'Cockpit/Scripts/Systems/weapon_system.lua', 'Cockpit/Scripts/Systems/cms_system.lua',
        'TESTING/START-HERE.zh-TW.html', 'TESTING/BUILD.json')) {
        $entry = $zip.GetEntry("F-CK-1C/$relative")
        if ($null -eq $entry) { throw "Missing archive entry: $relative" }
    }
    $stream = $zip.GetEntry('F-CK-1C/bin/F-CK-1C_EFM.dll').Open()
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        $packedHash = [System.BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '').ToLowerInvariant()
        if ($packedHash -ne $dllHash) { throw 'Packaged DLL hash mismatch.' }
    }
    finally { $stream.Dispose(); $sha.Dispose() }
}
finally { $zip.Dispose() }
$zipHash = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
$zipName = Split-Path -Leaf $zipPath
Set-Content -LiteralPath "$zipPath.sha256" -Value "$zipHash *$zipName" -Encoding ASCII
Write-Output "Verified test package: $zipPath"
Write-Output "Source: $SourceCommit; DLL SHA256: $dllHash; ZIP SHA256: $zipHash"
