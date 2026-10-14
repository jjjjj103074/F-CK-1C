<#
.SYNOPSIS
    Rebuild the pinned Release/x64 EFM DLL and publish it to the mod bin folder.
#>
param([string]$MsBuildPath = '')

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'efm-build-common.ps1')
$repoRoot = Split-Path -Parent $PSScriptRoot
$result = Invoke-EfmNativeBuild -RepoRoot $repoRoot -Target Dll -MsBuildPath $MsBuildPath
$destination = Join-Path $repoRoot 'bin\F-CK-1C_EFM.dll'
[IO.Directory]::CreateDirectory((Split-Path -Parent $destination)) | Out-Null
Copy-Item -LiteralPath $result.Output -Destination $destination -Force
$sourceHash = Get-EfmSha256 $result.Output
$runtimeHash = Get-EfmSha256 $destination
if ($sourceHash -ne $runtimeHash) { throw 'Runtime DLL hash does not match the build output.' }
Write-Output "Built and copied: $destination"
Write-Output "SRC HASH: $sourceHash"
Write-Output "BIN HASH: $runtimeHash"
