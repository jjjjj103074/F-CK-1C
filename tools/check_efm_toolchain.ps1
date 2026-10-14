param([string]$ToolchainRoot = '', [string]$SdkRoot = '', [string]$RepoRoot = '')

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'efm-build-common.ps1')
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent $PSScriptRoot }
$profile = Get-EfmBuildProfile $RepoRoot
if (-not $ToolchainRoot) { $ToolchainRoot = (Find-EfmBuildTools $RepoRoot).Toolchain }
if (-not $SdkRoot) { $SdkRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10' }
$tools = @(
    @{ File = 'cl.exe'; Hash = $profile.EfmCompilerSha256; Version = $profile.EfmCompilerVersion },
    @{ File = 'link.exe'; Hash = $profile.EfmLinkerSha256; Version = $profile.EfmLinkerVersion },
    @{ File = 'c1xx.dll'; Hash = $profile.EfmFrontendSha256; Version = '' },
    @{ File = 'c2.dll'; Hash = $profile.EfmBackendSha256; Version = '' }
)
foreach ($tool in $tools) {
    $file = Join-Path $ToolchainRoot ('bin\Hostx64\x64\' + $tool.File)
    if (-not (Test-Path -LiteralPath $file)) { throw "Pinned tool is missing: $file" }
    if ($tool.Version) {
        $version = (Get-Item -LiteralPath $file).VersionInfo
        $actual = "$($version.FileMajorPart).$($version.FileMinorPart).$($version.FileBuildPart).$($version.FilePrivatePart)"
        if ($actual -ne $tool.Version) { throw "Toolchain mismatch: $($tool.File) expected $($tool.Version), found $actual." }
    }
    $hash = Get-EfmSha256 $file
    if ($hash -ne $tool.Hash) { throw "Toolchain mismatch: $($tool.File) SHA256 expected $($tool.Hash), found $hash. Do not bypass this check; see docs/BUILD_DLL.md." }
}
foreach ($directory in @('Include', 'Lib')) {
    $sdkPath = Join-Path $SdkRoot "$directory\$($profile.EfmWindowsSdkVersion)"
    if (-not (Test-Path -LiteralPath $sdkPath -PathType Container)) { throw "Windows SDK $($profile.EfmWindowsSdkVersion) is missing: $sdkPath" }
}
$libraries = @(
    @{ Root = $ToolchainRoot; File = 'lib\x64\libcmt.lib'; Hash = $profile.EfmLibcmtSha256 },
    @{ Root = $ToolchainRoot; File = 'lib\x64\libcpmt.lib'; Hash = $profile.EfmLibcpmtSha256 },
    @{ Root = $ToolchainRoot; File = 'lib\x64\libvcruntime.lib'; Hash = $profile.EfmLibvcruntimeSha256 },
    @{ Root = $SdkRoot; File = "Lib\$($profile.EfmWindowsSdkVersion)\ucrt\x64\libucrt.lib"; Hash = $profile.EfmLibucrtSha256 }
)
foreach ($library in $libraries) {
    $file = Join-Path $library.Root $library.File
    if (-not (Test-Path -LiteralPath $file)) { throw "Pinned runtime library is missing: $file" }
    $hash = Get-EfmSha256 $file
    if ($hash -ne $library.Hash) { throw "Toolchain mismatch: $($library.File) SHA256 expected $($library.Hash), found $hash. Matching version labels alone are insufficient; see docs/BUILD_DLL.md." }
}
Write-Output "EFM toolchain matched: MSVC $($profile.EfmCompilerVersion), linker $($profile.EfmLinkerVersion), SDK $($profile.EfmWindowsSdkVersion), pinned static runtime libraries."
