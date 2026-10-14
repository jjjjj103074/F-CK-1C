<#
.SYNOPSIS
    Validate a PR submission or its simulated merge without replacing bin/F-CK-1C_EFM.dll.
#>
param([ValidateSet('Submission', 'Merge')][string]$Phase = 'Submission',
    [string]$RepoRoot = '', [string]$ReportDirectory = '', [string]$ExpectedHead = '', [string]$SubmittedDllPath = '')

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'efm-build-common.ps1')
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent $PSScriptRoot }
$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path
if (-not $ReportDirectory) { $ReportDirectory = Join-Path $RepoRoot ('tmp\pr-validation\' + $Phase.ToLowerInvariant()) }
[IO.Directory]::CreateDirectory($ReportDirectory) | Out-Null
$report = [ordered]@{ phase = $Phase; status = 'running'; dcs = 'not_run'; steps = [Collections.Generic.List[object]]::new(); hashes = @{} }
$failure = $null

function Invoke-ValidationStep {
    param([string]$Name, [scriptblock]$Action)

    $watch = [Diagnostics.Stopwatch]::StartNew()
    $step = [ordered]@{ name = $Name; status = 'running'; seconds = 0 }
    $report.steps.Add($step)
    try { & $Action | Out-Host; $step.status = 'passed' }
    catch { $step.status = 'failed'; $step['error'] = $_.Exception.Message; throw }
    finally { $watch.Stop(); $step.seconds = [math]::Round($watch.Elapsed.TotalSeconds, 2) }
}

function Assert-ExternalSuccess {
    param([string]$Name)
    if ($LASTEXITCODE -ne 0) { throw "$Name failed (exit $LASTEXITCODE)." }
}

try {
    if ($ExpectedHead) {
        Invoke-ValidationStep 'Pinned PR source' {
            if ($ExpectedHead -notmatch '^[a-fA-F0-9]{40}$') { throw 'ExpectedHead must be a full commit SHA.' }
            $actual = (& git -C $RepoRoot rev-parse HEAD).Trim()
            Assert-ExternalSuccess 'Read source commit'
            if ($actual -ne $ExpectedHead) { throw "Source commit mismatch: expected $ExpectedHead, found $actual." }
            if ($Phase -eq 'Submission') {
                & git -C $RepoRoot diff --exit-code HEAD
                Assert-ExternalSuccess 'Clean PR checkout'
            }
            $untracked = @(& git -C $RepoRoot ls-files --others --exclude-standard)
            Assert-ExternalSuccess 'Untracked source check'
            if ($untracked.Count) { throw "Unexpected untracked inputs: $($untracked -join ', ')" }
            $ignoredSources = @(& git -C $RepoRoot ls-files --others --ignored --exclude-standard -- src/efm/F-CK-1C_EFM src/efm/F-CK-1C_EFM_Tests | Where-Object { $_ -match '\.(c|cc|cpp|cxx|h|hh|hpp|hxx|inc|inl|ipp|tpp)$' })
            Assert-ExternalSuccess 'Ignored source check'
            if ($ignoredSources.Count) { throw "Ignored C++ inputs are not allowed: $($ignoredSources -join ', ')" }
            $report['head'] = $actual
        }
    }
    Invoke-ValidationStep 'Pinned Windows toolchain' {
        $report['toolchain'] = Get-EfmBuildProfile $RepoRoot
        & (Join-Path $PSScriptRoot 'check_efm_toolchain.ps1') -RepoRoot $RepoRoot
    }
    $sourceFingerprint = Get-EfmSourceFingerprint $RepoRoot
    $uploadedPath = if ($SubmittedDllPath) { (Resolve-Path -LiteralPath $SubmittedDllPath).Path } else { Join-Path $RepoRoot 'bin\F-CK-1C_EFM.dll' }
    $uploadedCopy = Join-Path $ReportDirectory 'uploaded.dll'
    if (Test-Path -LiteralPath $uploadedCopy) { Remove-Item -LiteralPath $uploadedCopy -Force }
    Copy-Item -LiteralPath $uploadedPath -Destination $uploadedCopy
    $uploadedHash = Get-EfmSha256 $uploadedCopy
    $report.hashes['uploaded'] = $uploadedHash
    $originalHash = Get-EfmSha256 $uploadedPath
    $buildResult = $null
    Invoke-ValidationStep 'Complete DLL rebuild' {
        $script:buildResult = Invoke-EfmNativeBuild -RepoRoot $RepoRoot -Target Dll -LogDirectory $ReportDirectory
    }
    Invoke-ValidationStep 'Whole DLL SHA256' {
        $rebuiltHash = Get-EfmSha256 $script:buildResult.Output
        $report.hashes['rebuilt'] = $rebuiltHash
        if ((Get-EfmSha256 $uploadedPath) -ne $originalHash -or (Get-EfmSha256 $uploadedCopy) -ne $uploadedHash) { throw 'Build modified the uploaded DLL; validation does not publish or replace it.' }
        if ($rebuiltHash -ne $uploadedHash) {
            $reason = if ($Phase -eq 'Merge') { 'Merge changes the submitted DLL result. Update the PR branch, rebuild and submit the DLL, then rerun validation.' } else { 'Cannot verify that the uploaded DLL matches this source and build environment.' }
            throw "$reason`nUploaded SHA256: $uploadedHash`nRebuilt SHA256: $rebuiltHash"
        }
        Write-Output "DLL SHA256 matched: $uploadedHash"
    }
    if ($Phase -eq 'Submission') {
        Invoke-ValidationStep 'Uploaded DLL exports' {
            & (Join-Path $PSScriptRoot 'check_efm_exports.ps1') -DllPath $uploadedCopy -BaselinePath (Join-Path $RepoRoot 'src\efm\F-CK-1C_EFM\DcsBridge\EfmExports.baseline.txt') -DumpbinPath (Join-Path $script:buildResult.Toolchain 'bin\Hostx64\x64\dumpbin.exe')
        }
        Invoke-ValidationStep 'Uploaded DLL load and unload (no DCS)' {
            & (Join-Path $PSScriptRoot 'check_efm_dll.ps1') -DllPath $uploadedCopy -BaselinePath (Join-Path $RepoRoot 'src\efm\F-CK-1C_EFM\DcsBridge\EfmExports.baseline.txt') -ExpectedSha256 $uploadedHash
        }
        Invoke-ValidationStep 'Architecture and generator fixtures' {
            foreach ($scriptName in @('test_efm_architecture_checker.ps1', 'test_system_catalog_generator.ps1', 'test_dcs_id_generator.ps1')) {
                & powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot $scriptName)
                Assert-ExternalSuccess $scriptName
            }
        }
    }
    if ($Phase -eq 'Submission') {
        Invoke-ValidationStep 'DLL validation rejection fixtures' {
            & powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'test_efm_validation.ps1') -DllPath $uploadedCopy
            Assert-ExternalSuccess 'DLL validation fixtures'
        }
    }
    Invoke-ValidationStep 'Native source tests' {
        $tests = Invoke-EfmNativeBuild -RepoRoot $RepoRoot -Target Tests -LogDirectory $ReportDirectory
        & $tests.Output 2>&1 | Tee-Object -FilePath (Join-Path $ReportDirectory 'native-tests.log') | Out-Host
        Assert-ExternalSuccess 'Native tests'
    }
    Invoke-ValidationStep 'Source unchanged by checks' {
        if ((Get-EfmSourceFingerprint $RepoRoot) -ne $sourceFingerprint) { throw 'Build or tests changed source/build/test inputs during validation.' }
        if ((Get-EfmSha256 $uploadedPath) -ne $originalHash -or (Get-EfmSha256 $uploadedCopy) -ne $uploadedHash) { throw 'Checks modified the uploaded DLL.' }
    }
    $report.status = 'passed'
}
catch { $report.status = 'failed'; $report['error'] = $_.Exception.Message; $failure = $_ }
finally {
    $encoding = [Text.UTF8Encoding]::new($false)
    [IO.File]::WriteAllText((Join-Path $ReportDirectory 'report.json'), ($report | ConvertTo-Json -Depth 8), $encoding)
    $lines = @("### EFM $Phase validation: $($report.status)", '', '| Check | Result | Seconds |', '|---|---|---:|')
    foreach ($step in $report.steps) { $lines += "| $($step.name) | $($step.status) | $($step.seconds) |" }
    foreach ($entry in $report.hashes.GetEnumerator()) { $lines += "`n$($entry.Key) SHA256: ``$($entry.Value)``" }
    if ($report.Contains('toolchain')) {
        $lines += "`nToolchain: MSVC $($report.toolchain.EfmCompilerVersion), linker $($report.toolchain.EfmLinkerVersion), SDK $($report.toolchain.EfmWindowsSdkVersion)."
    }
    $lines += '`nDCS in-game test: **not run — manual verification is separate**.'.Replace('`n', "`n")
    if ($failure) { $lines += "`nFailure:`n`n" + '```text' + "`n" + $failure.Exception.Message + "`n" + '```' }
    $summary = $lines -join "`n"
    [IO.File]::WriteAllText((Join-Path $ReportDirectory 'summary.md'), $summary, $encoding)
    if ($env:GITHUB_STEP_SUMMARY) { [IO.File]::AppendAllText($env:GITHUB_STEP_SUMMARY, $summary + "`n", $encoding) }
}
if ($failure) { throw $failure }
