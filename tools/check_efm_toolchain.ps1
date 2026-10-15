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
    if (-not (Test-Path -LiteralPath $file)) { throw "缺少編譯工具：$file" }
    if ($tool.Version) {
        $version = (Get-Item -LiteralPath $file).VersionInfo
        $actual = "$($version.FileMajorPart).$($version.FileMinorPart).$($version.FileBuildPart).$($version.FilePrivatePart)"
        if ($actual -ne $tool.Version) { throw "工具版本不符：$($tool.File)。請依 docs/BUILD_DLL.md 安裝。`n預期：$($tool.Version)`n目前：$actual" }
    }
    $hash = Get-EfmSha256 $file
    if ($hash -ne $tool.Hash) { throw "工具鏈不符：$($tool.File)。請依 docs/BUILD_DLL.md 安裝。`n預期 SHA256：$($tool.Hash)`n目前 SHA256：$hash" }
}
foreach ($directory in @('Include', 'Lib')) {
    $sdkPath = Join-Path $SdkRoot "$directory\$($profile.EfmWindowsSdkVersion)"
    if (-not (Test-Path -LiteralPath $sdkPath -PathType Container)) { throw "缺少 Windows SDK $($profile.EfmWindowsSdkVersion)：$sdkPath" }
}
$libraries = @(
    @{ Root = $ToolchainRoot; File = 'lib\x64\libcmt.lib'; Hash = $profile.EfmLibcmtSha256 },
    @{ Root = $ToolchainRoot; File = 'lib\x64\libcpmt.lib'; Hash = $profile.EfmLibcpmtSha256 },
    @{ Root = $ToolchainRoot; File = 'lib\x64\libvcruntime.lib'; Hash = $profile.EfmLibvcruntimeSha256 },
    @{ Root = $SdkRoot; File = "Lib\$($profile.EfmWindowsSdkVersion)\ucrt\x64\libucrt.lib"; Hash = $profile.EfmLibucrtSha256 }
)
foreach ($library in $libraries) {
    $file = Join-Path $library.Root $library.File
    if (-not (Test-Path -LiteralPath $file)) { throw "缺少執行期函式庫：$file" }
    $hash = Get-EfmSha256 $file
    if ($hash -ne $library.Hash) { throw "工具鏈不符：$($library.File)。請依 docs/BUILD_DLL.md 安裝。`n預期 SHA256：$($library.Hash)`n目前 SHA256：$hash" }
}
Write-Output '編譯工具與執行期函式庫一致。'
