Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'efm-build-common.ps1')
$repo = Split-Path -Parent $PSScriptRoot
$profile = Get-EfmBuildProfile $repo
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$instances = @((& $vswhere -all -products '*' -requires Microsoft.Component.MSBuild -format json) | ConvertFrom-Json)
$toolchain = $null
foreach($instance in $instances) {
    $candidate = Join-Path $instance.installationPath ('VC\Tools\MSVC\'+$profile.EfmVCToolsVersion)
    $compiler = Join-Path $candidate 'bin\Hostx64\x64\cl.exe'
    if ((Test-Path -LiteralPath $compiler) -and (Get-EfmSha256 $compiler) -eq $profile.EfmCompilerSha256) { $toolchain=$candidate; break }
}
if(-not $toolchain) { throw 'No matching compiler.' }
$sdk=Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10'
$groups=Get-EfmToolchainFingerprint -ToolchainRoot $toolchain -SdkRoot $sdk -SdkVersion $profile.EfmWindowsSdkVersion
foreach($name in $groups.Keys) { Write-Output ('NATIVE-GROUP '+$name+' '+$groups[$name]) }
$bin=Join-Path $toolchain 'bin\Hostx64\x64'
foreach($file in Get-ChildItem -LiteralPath $bin -File -Recurse) {
    $relative=$file.FullName.Substring($bin.Length+1).Replace('\','/').ToLowerInvariant()
    if ($relative -match '(^|/)\d{4}/' -or $relative.EndsWith('.resources.dll')) {continue}
    Write-Output ('NATIVE-TOOL '+$relative+' '+(Get-EfmSha256 $file.FullName))
}