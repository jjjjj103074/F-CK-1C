<#
.SYNOPSIS
    Validate a PR submission or its simulated merge without replacing bin/F-CK-1C_EFM.dll.
#>
param([ValidateSet('Submission', 'Merge')][string]$Phase = 'Submission',
    [string]$RepoRoot = '', [string]$ReportDirectory = '', [string]$ExpectedHead = '', [string]$SubmittedDllPath = '',
    [string]$Checks = 'all')

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'efm-build-common.ps1')
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent $PSScriptRoot }
$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path
if (-not $ReportDirectory) { $ReportDirectory = Join-Path $RepoRoot ('tmp\pr-validation\' + $Phase.ToLowerInvariant()) }
[IO.Directory]::CreateDirectory($ReportDirectory) | Out-Null
$report = [ordered]@{ phase = $Phase; status = 'running'; dcs = 'not_run'; steps = [Collections.Generic.List[object]]::new(); hashes = @{} }
$failure = $null
$run = @{}
$selected = $Checks -split ','
$names = @('dll', 'native', 'analysis', 'architecture', 'catalog', 'dcs', 'cockpit', 'validation')
if (@($selected | Where-Object { $_ -notin ($names + @('all', 'none')) }).Count) { throw '未知的檢查項目。' }
foreach ($name in $names) { $run[$name] = 'all' -in $selected -or $name -in $selected }
if ($run.validation) { $run.dll = $true }
if ($run.analysis -and -not ($run.dll -or $run.native)) { throw '靜態分析需搭配 DLL 或原生測試建置。' }

function Invoke-ValidationStep {
    param([string]$Name, [scriptblock]$Action, [bool]$Enabled = $true)

    $watch = [Diagnostics.Stopwatch]::StartNew()
    $step = [ordered]@{ name = $Name; status = 'running'; seconds = 0 }
    $report.steps.Add($step)
    if (-not $Enabled) { $step.status = 'skipped'; return }
    try { & $Action | Out-Host; $step.status = 'passed' }
    catch { $step.status = 'failed'; $step['error'] = $_.Exception.Message; throw }
    finally { $watch.Stop(); $step.seconds = [math]::Round($watch.Elapsed.TotalSeconds, 2) }
}

function Assert-ExternalSuccess {
    param([string]$Name)
    if ($LASTEXITCODE -ne 0) { throw "$Name 失敗（結束碼 $LASTEXITCODE）。" }
}

try {
    if ($ExpectedHead) {
        Invoke-ValidationStep '提交版本' {
            if ($ExpectedHead -notmatch '^[a-fA-F0-9]{40}$') { throw 'ExpectedHead 需為完整提交 SHA。' }
            $actual = (& git -C $RepoRoot rev-parse HEAD).Trim()
            Assert-ExternalSuccess '讀取提交版本'
            if ($actual -ne $ExpectedHead) { throw "來源版本不符，請重新取出 PR。`n預期：$ExpectedHead`n目前：$actual" }
            if ($Phase -eq 'Submission') {
                & git -C $RepoRoot diff --exit-code HEAD
                Assert-ExternalSuccess '來源乾淨檢查'
            }
            $untracked = @(& git -C $RepoRoot ls-files --others --exclude-standard)
            Assert-ExternalSuccess '未追蹤檔案檢查'
            if ($untracked.Count) { throw "有未追蹤檔案，請先整理來源。`n$($untracked -join ', ')" }
            $ignoredSources = @(& git -C $RepoRoot ls-files --others --ignored --exclude-standard -- src/efm/F-CK-1C_EFM src/efm/F-CK-1C_EFM_Tests | Where-Object { $_ -match '\.(c|cc|cpp|cxx|h|hh|hpp|hxx|inc|inl|ipp|tpp)$' })
            Assert-ExternalSuccess '忽略來源檢查'
            if ($ignoredSources.Count) { throw "有被 Git 忽略的 C++ 來源，請移除或納入追蹤。`n$($ignoredSources -join ', ')" }
            $report['head'] = $actual
        }
    }
    Invoke-ValidationStep '編譯工具' {
        $report['toolchain'] = Get-EfmBuildProfile $RepoRoot
        & (Join-Path $PSScriptRoot 'check_efm_toolchain.ps1') -RepoRoot $RepoRoot
    } ($run.dll -or $run.native)
    $sourceFingerprint = Get-EfmSourceFingerprint $RepoRoot
    if ($run.dll) {
        $uploadedPath = if ($SubmittedDllPath) { (Resolve-Path -LiteralPath $SubmittedDllPath).Path } else { Join-Path $RepoRoot 'bin\F-CK-1C_EFM.dll' }
        $uploadedCopy = Join-Path $ReportDirectory 'uploaded.dll'
        if (Test-Path -LiteralPath $uploadedCopy) { Remove-Item -LiteralPath $uploadedCopy -Force }
        Copy-Item -LiteralPath $uploadedPath -Destination $uploadedCopy
        $uploadedHash = Get-EfmSha256 $uploadedCopy
        $report.hashes['uploaded'] = $uploadedHash
        $originalHash = Get-EfmSha256 $uploadedPath
    }
    $buildResult = $null
    $dllBuildLabel = if ($run.analysis) { 'DLL 編譯／分析' } else { 'DLL 重建' }
    Invoke-ValidationStep $dllBuildLabel {
        $script:buildResult = Invoke-EfmNativeBuild -RepoRoot $RepoRoot -Target Dll -LogDirectory $ReportDirectory -Analyze:$run.analysis
    } $run.dll
    Invoke-ValidationStep 'DLL 雜湊' {
        $rebuiltHash = Get-EfmSha256 $script:buildResult.Output
        $report.hashes['rebuilt'] = $rebuiltHash
        if ((Get-EfmSha256 $uploadedPath) -ne $originalHash -or (Get-EfmSha256 $uploadedCopy) -ne $uploadedHash) { throw '建置改動了提交 DLL，請檢查建置腳本。' }
        if ($rebuiltHash -ne $uploadedHash) {
            Write-Output "提交 SHA256：$uploadedHash`n重建 SHA256：$rebuiltHash"
            $reason = if ($Phase -eq 'Merge') { '合併後 DLL 不同。請同步目標分支，重新建置並提交 DLL。' } else { 'DLL 雜湊不同。請確認工具鏈，重新建置並提交 DLL。' }
            throw $reason
        }
        Write-Output 'DLL 雜湊一致。'
    } $run.dll
    if ($Phase -eq 'Submission') {
        Invoke-ValidationStep 'DLL 接口' {
            & (Join-Path $PSScriptRoot 'check_efm_exports.ps1') -DllPath $uploadedCopy -BaselinePath (Join-Path $RepoRoot 'src\efm\F-CK-1C_EFM\DcsBridge\EfmExports.baseline.txt') -DumpbinPath (Join-Path $script:buildResult.Toolchain 'bin\Hostx64\x64\dumpbin.exe')
        } $run.dll
        Invoke-ValidationStep 'DLL 載入／卸載' {
            & (Join-Path $PSScriptRoot 'check_efm_dll.ps1') -DllPath $uploadedCopy -BaselinePath (Join-Path $RepoRoot 'src\efm\F-CK-1C_EFM\DcsBridge\EfmExports.baseline.txt') -ExpectedSha256 $uploadedHash
        } $run.dll
        foreach ($fixture in @(
            @('架構測試', 'architecture', 'test_efm_architecture_checker.ps1'),
            @('系統目錄產生器', 'catalog', 'test_system_catalog_generator.ps1'),
            @('DCS ID 產生器', 'dcs', 'test_dcs_id_generator.ps1'),
            @('座艙架構', 'cockpit', 'check_cockpit_architecture.ps1'),
            @('座艙架構測試', 'cockpit', 'test_cockpit_architecture_checker.ps1'))) {
            $scriptName = $fixture[2]
            Invoke-ValidationStep $fixture[0] {
                & powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot $scriptName)
                Assert-ExternalSuccess $scriptName
            } $run[$fixture[1]]
        }
    }
    if ($Phase -eq 'Submission') {
        Invoke-ValidationStep '驗證工具' {
            & powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'test_efm_validation.ps1') -DllPath $uploadedCopy
            Assert-ExternalSuccess '驗證工具測試'
        } $run.validation
        Invoke-ValidationStep '分析規則測試' {
            & powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'test_cpp_analysis.ps1') -RepoRoot $RepoRoot
            Assert-ExternalSuccess '分析規則測試'
        } $run.validation
    }
    $testsBuildLabel = if ($run.analysis) { '測試編譯／分析' } else { '測試編譯' }
    Invoke-ValidationStep $testsBuildLabel {
        $script:testBuild = Invoke-EfmNativeBuild -RepoRoot $RepoRoot -Target Tests -LogDirectory $ReportDirectory -Analyze:$run.analysis
    } $run.native
    Invoke-ValidationStep '原生測試' {
        & $script:testBuild.Output 2>&1 | Tee-Object -FilePath (Join-Path $ReportDirectory 'native-tests.log') | Out-Host
        Assert-ExternalSuccess '原生測試'
    } $run.native
    Invoke-ValidationStep '來源完整性' {
        if ((Get-EfmSourceFingerprint $RepoRoot) -ne $sourceFingerprint) { throw '驗證改動了來源或設定，請檢查建置與測試腳本。' }
        if ($run.dll -and ((Get-EfmSha256 $uploadedPath) -ne $originalHash -or (Get-EfmSha256 $uploadedCopy) -ne $uploadedHash)) { throw '驗證改動了提交 DLL，請檢查驗證腳本。' }
    }
    $report.status = 'passed'
}
catch { $report.status = 'failed'; $report['error'] = $_.Exception.Message; $failure = $_ }
finally {
    $encoding = [Text.UTF8Encoding]::new($false)
    [IO.File]::WriteAllText((Join-Path $ReportDirectory 'report.json'), ($report | ConvertTo-Json -Depth 8), $encoding)
    $phaseLabel = if ($Phase -eq 'Submission') { '提交檢查' } else { '合併檢查' }
    $statusLabels = @{ passed = '通過'; failed = '失敗'; running = '執行中'; skipped = '不適用' }
    $lines = @("### ${phaseLabel}：$($statusLabels[$report.status])", '', '| 檢查 | 結果 | 秒 |', '|---|---|---:|')
    foreach ($step in $report.steps) { $lines += "| $($step.name) | $($statusLabels[$step.status]) | $($step.seconds) |" }
    if ($failure) { $lines += "`n" + ($failure.Exception.Message -split "`r?`n")[0] }
    $diagnostics = @()
    if ($report.Contains('head')) { $diagnostics += "PR：$($report.head)" }
    if ($report.Contains('toolchain')) {
        $diagnostics += "MSVC：$($report.toolchain.EfmCompilerVersion)；link：$($report.toolchain.EfmLinkerVersion)；SDK：$($report.toolchain.EfmWindowsSdkVersion)"
    }
    foreach ($entry in $report.hashes.GetEnumerator()) {
        $label = if ($entry.Key -eq 'uploaded') { '提交 DLL' } else { '重建 DLL' }
        $diagnostics += "$label SHA256：$($entry.Value)"
    }
    if ($failure) { $diagnostics += @($failure.Exception.Message -split "`r?`n" | Select-Object -Skip 1) }
    if ($diagnostics.Count) { $lines += @('', '<details>', '<summary>診斷資料</summary>', '', '```text') + $diagnostics + @('```', '', '</details>') }
    $summary = $lines -join "`n"
    [IO.File]::WriteAllText((Join-Path $ReportDirectory 'summary.md'), $summary, $encoding)
    if ($env:GITHUB_STEP_SUMMARY) { [IO.File]::AppendAllText($env:GITHUB_STEP_SUMMARY, $summary + "`n", $encoding) }
}
if ($failure) { throw $failure }
