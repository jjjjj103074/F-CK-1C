Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'efm-build-common.ps1')
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$instances = @((& $vswhere -all -products '*' -requires Microsoft.Component.MSBuild -format json) | ConvertFrom-Json)
foreach ($instance in $instances) {
    $root = Join-Path $instance.installationPath 'MSBuild\Microsoft\VC'
    if (-not (Test-Path -LiteralPath (Join-Path $instance.installationPath 'VC\Tools\MSVC\14.29.30133\bin\Hostx64\x64\cl.exe'))) { continue }
    foreach ($file in Get-EfmCppTargetFiles $root) {
        $relative = $file.FullName.Substring($root.Length + 1).Replace('\', '/').ToLowerInvariant()
        Write-Output ('CPP-FILE ' + $relative + ' ' + (Get-EfmSha256 $file.FullName))
    }
    break
}
throw 'Diagnostic completed; resolve C++ build-file differences before full validation.'
