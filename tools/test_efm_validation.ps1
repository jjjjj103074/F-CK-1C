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
    Expect-Failure 'Wrong submitted hash' { & (Join-Path $PSScriptRoot 'check_efm_dll.ps1') -DllPath $DllPath -ExpectedSha256 ('0' * 64) } 'DLL changed before loading'
    $invalid = Join-Path $fixtureRoot 'invalid.dll'
    [IO.File]::WriteAllBytes($invalid, [Text.Encoding]::ASCII.GetBytes('not a Windows DLL'))
    Expect-Failure 'Invalid executable image' { & (Join-Path $PSScriptRoot 'check_efm_dll.ps1') -DllPath $invalid } 'Windows DLL load failed'
    $missingBaseline = Join-Path $fixtureRoot 'missing-export.txt'
    [IO.File]::WriteAllText($missingBaseline, 'ed_fm_missing_fixture_export')
    Expect-Failure 'Missing required export' { & (Join-Path $PSScriptRoot 'check_efm_dll.ps1') -DllPath $DllPath -BaselinePath $missingBaseline } 'Missing DLL export'
    $fakeTools = Join-Path $fixtureRoot 'tools\bin\Hostx64\x64'
    [IO.Directory]::CreateDirectory($fakeTools) | Out-Null
    $realTools = (Find-EfmBuildTools $repoRoot).Toolchain
    foreach ($name in @('cl.exe', 'link.exe')) { Copy-Item -LiteralPath (Join-Path $realTools "bin\Hostx64\x64\$name") -Destination (Join-Path $fakeTools $name) }
    [IO.File]::WriteAllText((Join-Path $fakeTools 'c1xx.dll'), 'wrong compiler frontend')
    Expect-Failure 'Modified compiler frontend' { & (Join-Path $PSScriptRoot 'check_efm_toolchain.ps1') -ToolchainRoot (Join-Path $fixtureRoot 'tools') } 'Toolchain mismatch: c1xx.dll'
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
    Write-Output "EFM validation fixtures passed: $checks rejection/artifact-preservation checks."
}
finally {
    $resolved = [IO.Path]::GetFullPath($fixtureRoot)
    if (-not $resolved.StartsWith($repoRoot + '\tmp\efm-validation-tests-', [StringComparison]::OrdinalIgnoreCase)) { throw 'Refusing to remove a fixture outside the expected workspace directory.' }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
