param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[A-Za-z0-9._-]+$')]
    [string]$Version,
    [Parameter(Mandatory = $true)]
    [string]$ArtifactDirectory,
    [Parameter(Mandatory = $true)]
    [string]$SourceCommit,
    [string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot)
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repoRoot = [System.IO.Path]::GetFullPath($RepositoryRoot)
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
& (Join-Path $repoRoot 'tools/package_release.ps1') -Version $Version -AllowDirty
$stage = Join-Path $repoRoot 'dist/F-CK-1C'
$testing = Join-Path $stage 'TESTING'
New-Item -ItemType Directory -Path $testing -Force | Out-Null
$guide = Get-Content -LiteralPath (Join-Path $repoRoot 'docs/MC_STORES_PACKAGE_GUIDE.zh-TW.html') -Raw -Encoding UTF8
$guide = $guide.Replace('@@SOURCE_COMMIT@@', $SourceCommit).Replace('@@DLL_SHA256@@', $dllHash)
$guide | Set-Content -LiteralPath (Join-Path $testing 'START-HERE.zh-TW.html') -Encoding UTF8
Copy-Item -LiteralPath $manifestPath -Destination (Join-Path $testing 'BUILD.json') -Force
$notices = Join-Path $stage 'THIRD-PARTY-NOTICES'
New-Item -ItemType Directory -Path $notices -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $repoRoot 'src/efm/LICENSE') -Destination (Join-Path $notices 'EFM-MIT.txt') -Force
$zipPath = Join-Path $repoRoot "dist/F-CK-1C-$Version.zip"
Compress-Archive -Path $stage -DestinationPath $zipPath -CompressionLevel Optimal -Force
Add-Type -AssemblyName System.IO.Compression.FileSystem
Add-Type -AssemblyName System.IO.Compression
$normalized = "$zipPath.normalized-$([guid]::NewGuid().ToString('N'))"
$inputArchive = [System.IO.Compression.ZipFile]::OpenRead($zipPath)
try {
    $outputArchive = [System.IO.Compression.ZipFile]::Open($normalized, [System.IO.Compression.ZipArchiveMode]::Create)
    try {
        $seen = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
        foreach ($entry in $inputArchive.Entries) {
            $name = $entry.FullName.Replace('\', '/')
            if (-not $seen.Add($name)) { throw "Duplicate archive entry: $name" }
            $copy = $outputArchive.CreateEntry($name, [System.IO.Compression.CompressionLevel]::Optimal)
            $copy.LastWriteTime = $entry.LastWriteTime
            $inputStream = $entry.Open()
            try {
                $outputStream = $copy.Open()
                try { $inputStream.CopyTo($outputStream) }
                finally { $outputStream.Dispose() }
            }
            finally { $inputStream.Dispose() }
        }
    }
    finally { $outputArchive.Dispose() }
}
finally { $inputArchive.Dispose() }
Move-Item -LiteralPath $normalized -Destination $zipPath -Force
$zip = [System.IO.Compression.ZipFile]::OpenRead($zipPath)
try {
    foreach ($relative in @('bin/F-CK-1C_EFM.dll', 'bin/F-CK-1C_EFM.validation.json',
        'Cockpit/Scripts/Stores/ObservationDiagnostics.lua', 'Cockpit/Scripts/Stores/StationDefinitions.lua',
        'Cockpit/Scripts/Systems/weapon_system.lua', 'Cockpit/Scripts/Systems/cms_system.lua',
        'TESTING/START-HERE.zh-TW.html', 'TESTING/BUILD.json', 'THIRD-PARTY-NOTICES/EFM-MIT.txt')) {
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
