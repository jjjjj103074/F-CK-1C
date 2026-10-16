param([string]$ToolchainRoot = '', [string]$SdkRoot = '', [string]$RepoRoot = '', [string]$MsBuildPath = '')

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'efm-build-common.ps1')
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent $PSScriptRoot }
if ($ToolchainRoot) {
    $profile = Get-EfmBuildProfile $RepoRoot
    Assert-EfmToolchain -Profile $profile -ToolchainRoot $ToolchainRoot -SdkRoot $SdkRoot -MsBuildPath $MsBuildPath
}
else {
    $selected = Find-EfmBuildTools -RepoRoot $RepoRoot -MsBuildPath $MsBuildPath -SdkRoot $SdkRoot
    $profile = $selected.Profile
}
Write-Output "工具版本符合配方：MSBuild $($profile.EfmMsBuildVersion)；MSVC $($profile.EfmCompilerVersion)；SDK $($profile.EfmWindowsSdkVersion)。"
