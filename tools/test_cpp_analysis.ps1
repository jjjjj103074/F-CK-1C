param([string]$RepoRoot = '')

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent $PSScriptRoot }
$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path
$fixtureRoot = Join-Path $RepoRoot ('tmp\cpp-analysis-tests-' + [Guid]::NewGuid().ToString('N'))
$projectRoot = Join-Path $fixtureRoot 'src\efm\F-CK-1C_EFM'
[IO.Directory]::CreateDirectory($projectRoot) | Out-Null
try {
    foreach ($file in @('src\efm\EfmBuild.props', 'src\efm\EfmBuild.targets',
            'tools\check_efm_toolchain.ps1', 'tools\efm-build-common.ps1')) {
        $destination = Join-Path $fixtureRoot $file
        [IO.Directory]::CreateDirectory((Split-Path -Parent $destination)) | Out-Null
        Copy-Item -LiteralPath (Join-Path $RepoRoot $file) -Destination $destination
    }
    $project = @'
<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup Label="ProjectConfigurations">
    <ProjectConfiguration Include="Release|x64"><Configuration>Release</Configuration><Platform>x64</Platform></ProjectConfiguration>
  </ItemGroup>
  <Import Project="..\EfmBuild.props" />
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
  <PropertyGroup Label="Configuration">
    <ConfigurationType>DynamicLibrary</ConfigurationType>
    <PlatformToolset>$(EfmPlatformToolset)</PlatformToolset>
    <UseDebugLibraries>false</UseDebugLibraries>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
  <PropertyGroup>
    <OutDir>$(ProjectDir)..\x64\Release\</OutDir>
    <IntDir>$(OutDir)obj\</IntDir>
  </PropertyGroup>
  <ItemDefinitionGroup><ClCompile><LanguageStandard>stdcpp17</LanguageStandard></ClCompile></ItemDefinitionGroup>
  <ItemGroup><ClCompile Include="probe.cpp" /></ItemGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
  <Import Project="..\EfmBuild.targets" />
</Project>
'@
    [IO.File]::WriteAllText((Join-Path $projectRoot 'F-CK-1C_EFM.vcxproj'), $project)
    foreach ($directory in @('ThirdParty', 'include')) {
        [IO.Directory]::CreateDirectory((Join-Path $projectRoot $directory)) | Out-Null
    }
    [IO.File]::WriteAllText((Join-Path $projectRoot 'ThirdParty\probe.h'), @'
struct ExternalState { int value; ExternalState() {} };
inline int external_callback(int unused) { return 1; }
'@)
    [IO.File]::WriteAllText((Join-Path $projectRoot 'include\probe.h'), 'struct SdkState { int value; SdkState() {} };')
    $cases = @(
        @{ Name = 'external'; Analyze = $true; Code = ''; Source = '#include <probe.h>' + "`n" + 'extern "C" __declspec(dllexport) int fixture() { ExternalState value; return external_callback(0); }' },
        @{ Name = 'compiler'; Analyze = $false; Code = 'C4100'; Source = 'extern "C" __declspec(dllexport) int fixture(int unused) { return 1; }' },
        @{ Name = 'analysis'; Analyze = $true; Code = 'C26495'; Source = 'struct OwnState { int value; OwnState() {} }; extern "C" __declspec(dllexport) int fixture() { OwnState value; return 1; }' },
        @{ Name = 'sdk'; Analyze = $true; Code = 'C26495'; Source = '#include "include/probe.h"' + "`n" + 'extern "C" __declspec(dllexport) int fixture() { SdkState value; return 1; }' }
    )
    foreach ($case in $cases) {
        [IO.File]::WriteAllText((Join-Path $projectRoot 'probe.cpp'), $case.Source)
        $logs = Join-Path $fixtureRoot $case.Name
        $arguments = @('-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass', '-File',
            (Join-Path $PSScriptRoot 'build_efm.ps1'), '-RepoRoot', $fixtureRoot, '-LogDirectory', $logs)
        if ($case.Analyze) { $arguments += '-Analyze' }
        try {
            # Windows PowerShell turns expected child stderr into error records.
            $ErrorActionPreference = 'Continue'
            $output = @(& powershell.exe @arguments 2>&1)
            $exitCode = $LASTEXITCODE
        }
        finally { $ErrorActionPreference = 'Stop' }
        $log = [IO.File]::ReadAllText((Join-Path $logs 'dll-build.log'))
        if ($case.Code) {
            if ($exitCode -eq 0 -or $log -notmatch $case.Code) { throw "$($case.Name): expected rejection for $($case.Code).`n$($output -join "`n")" }
        }
        elseif ($exitCode -ne 0 -or $log -match '\b(warning|error) [A-Z]+\d+:') {
            throw "External headers were not excluded.`n$($output -join "`n")"
        }
    }
    Write-Output 'C++ analysis fixtures passed: 4 checks.'
}
finally {
    $resolved = [IO.Path]::GetFullPath($fixtureRoot)
    if (-not $resolved.StartsWith($RepoRoot.TrimEnd('\') + '\tmp\cpp-analysis-tests-', [StringComparison]::OrdinalIgnoreCase)) { throw 'Fixture path is outside the workspace temporary directory.' }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
