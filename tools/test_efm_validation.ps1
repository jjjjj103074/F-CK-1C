param([string]$DllPath = '')

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'efm-build-common.ps1')
$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $DllPath) { $DllPath = Join-Path $repoRoot 'bin\F-CK-1C_EFM.dll' }
$fixtureRoot = Join-Path $repoRoot ('tmp\efm-validation-tests-' + [Guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($fixtureRoot) | Out-Null
$checks = 0
function Expect-Failure {
    param([string]$Name, [scriptblock]$Action, [string]$Pattern)
    try { & $Action | Out-Null }
    catch {
        if ($_.Exception.Message -notmatch $Pattern) { throw "$Name failed for the wrong reason: $($_.Exception.Message)" }
        $script:checks++
        return
    }
    throw "$Name unexpectedly passed."
}
try {
    Expect-Failure 'Wrong submitted hash' { & (Join-Path $PSScriptRoot 'check_efm_dll.ps1') -DllPath $DllPath -ExpectedSha256 ('0' * 64) } '載入前 DLL 雜湊不符'
    $invalid = Join-Path $fixtureRoot 'invalid.dll'
    [IO.File]::WriteAllBytes($invalid, [Text.Encoding]::ASCII.GetBytes('not a Windows DLL'))
    Expect-Failure 'Invalid executable image' { & (Join-Path $PSScriptRoot 'check_efm_dll.ps1') -DllPath $invalid } 'DLL 載入失敗'
    $missingBaseline = Join-Path $fixtureRoot 'missing-export.txt'
    [IO.File]::WriteAllText($missingBaseline, 'ed_fm_missing_fixture_export')
    Expect-Failure 'Missing required export' { & (Join-Path $PSScriptRoot 'check_efm_dll.ps1') -DllPath $DllPath -BaselinePath $missingBaseline } '缺少 DLL 接口'
    $realTools = (Find-EfmBuildTools $repoRoot).Toolchain
    $profile = Get-EfmBuildProfile $repoRoot
    $fakeSdk = Join-Path $fixtureRoot 'sdk'
    Expect-Failure 'Missing requested SDK' {
        & (Join-Path $PSScriptRoot 'check_efm_toolchain.ps1') -ToolchainRoot $realTools -SdkRoot $fakeSdk
    } '缺少 Windows SDK'
    $fakeInstance = Join-Path $fixtureRoot 'incompatible-vs'
    $fakeBuild = Join-Path $fakeInstance 'MSBuild\Current\Bin\MSBuild.exe'
    [IO.Directory]::CreateDirectory((Split-Path -Parent $fakeBuild)) | Out-Null
    [IO.File]::WriteAllText($fakeBuild, 'fixture placeholder; never execute')
    $candidateTools = Join-Path $fakeInstance "VC\Tools\MSVC\$($profile.EfmVCToolsVersion)\bin\Hostx64\x64"
    [IO.Directory]::CreateDirectory($candidateTools) | Out-Null
    Copy-Item -LiteralPath (Join-Path $realTools 'bin\Hostx64\x64\cl.exe') -Destination (Join-Path $candidateTools 'cl.exe')
    $realInstance = [IO.Directory]::GetParent($realTools).Parent.Parent.Parent.FullName
    $candidates = @([pscustomobject]@{ installationPath = $fakeInstance }, [pscustomobject]@{ installationPath = $realInstance })
    $selected = Select-EfmBuildTools -RepoRoot $repoRoot -Instances $candidates
    if ($selected.Toolchain -ne $realTools) { throw 'Automatic selection did not skip an incompatible MSBuild version.' }
    $checks++
    Expect-Failure 'Explicit incompatible MSBuild must not fall back' {
        Select-EfmBuildTools -RepoRoot $repoRoot -Instances $candidates -MsBuildPath $fakeBuild
    } '工具版本不符：MSBuild.exe'
    $profileRepo = Join-Path $fixtureRoot 'profile-repo'
    $profileFile = Join-Path $profileRepo 'src\efm\EfmBuild.props'
    [IO.Directory]::CreateDirectory((Split-Path -Parent $profileFile)) | Out-Null
    $profileXml = [xml][IO.File]::ReadAllText((Join-Path $repoRoot 'src\efm\EfmBuild.props'))
    foreach ($entry in @(
        @('EfmMsBuildVersion', 'MSBuild.exe'), @('EfmCompilerVersion', 'cl.exe'), @('EfmLinkerVersion', 'link.exe'))) {
        $profileXml.GetElementsByTagName($entry[0])[0].InnerText = '0.0.0.0'
        $profileXml.Save($profileFile)
        Expect-Failure ('Wrong version: ' + $entry[1]) {
            & (Join-Path $PSScriptRoot 'check_efm_toolchain.ps1') -RepoRoot $profileRepo -ToolchainRoot $realTools
        } ('工具版本不符：' + [regex]::Escape($entry[1]))
        $profileXml.GetElementsByTagName($entry[0])[0].InnerText = $profile[$entry[0]]
    }
    $gitRepo = Join-Path $fixtureRoot 'merge-repo'
    [IO.Directory]::CreateDirectory((Join-Path $gitRepo 'bin')) | Out-Null
    [IO.Directory]::CreateDirectory((Join-Path $gitRepo 'tools')) | Out-Null
    & git -C $gitRepo init --quiet
    & git -C $gitRepo config user.name 'EFM fixture'
    & git -C $gitRepo config user.email 'fixture@example.invalid'
    [IO.File]::WriteAllText((Join-Path $gitRepo 'bin\F-CK-1C_EFM.dll'), 'original PR artifact')
    [IO.File]::WriteAllText((Join-Path $gitRepo 'tools\example.ps1'), '# fixture controller')
    & git -C $gitRepo add .
    & git -C $gitRepo -c core.hooksPath=/dev/null commit --quiet -m 'fixture initial state'
    $initial = (& git -C $gitRepo rev-parse HEAD).Trim()
    & git -C $gitRepo checkout --quiet -b fixture-base
    [IO.File]::WriteAllText((Join-Path $gitRepo 'bin\F-CK-1C_EFM.dll'), 'new base artifact')
    & git -C $gitRepo add .
    & git -C $gitRepo -c core.hooksPath=/dev/null commit --quiet -m 'fixture base DLL change'
    $base = (& git -C $gitRepo rev-parse HEAD).Trim()
    & git -C $gitRepo checkout --quiet --detach $initial
    $prepared = Join-Path $fixtureRoot 'prepared'
    & (Join-Path $PSScriptRoot 'prepare_efm_merge.ps1') -RepoRoot $gitRepo -HeadSha $initial -BaseSha $base -OutputDirectory $prepared
    if ([IO.File]::ReadAllText((Join-Path $prepared 'pr-uploaded.dll')) -ne 'original PR artifact' -or
        [IO.File]::ReadAllText((Join-Path $gitRepo 'bin\F-CK-1C_EFM.dll')) -ne 'new base artifact') { throw 'Merge fixture did not preserve the PR artifact before a base-only replacement.' }
    $checks++
    Write-Output "驗證工具測試通過：$checks 項。"
}
finally {
    $resolved = [IO.Path]::GetFullPath($fixtureRoot)
    if (-not $resolved.StartsWith($repoRoot + '\tmp\efm-validation-tests-', [StringComparison]::OrdinalIgnoreCase)) { throw 'Refusing to remove a fixture outside the expected workspace directory.' }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
