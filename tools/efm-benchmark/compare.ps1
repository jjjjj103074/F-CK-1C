param([string]$OutputDirectory)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $root 'tmp\efm-comparison' }
[IO.Directory]::CreateDirectory($OutputDirectory) | Out-Null
. (Join-Path $root 'tools\efm-build-common.ps1')
$source = Get-EfmSourceFingerprint $root
$uploaded = Get-EfmSha256 (Join-Path $root 'bin\F-CK-1C_EFM.dll')
$cases = @(
    @('current', 'Dll'), @('simple', 'Dll'), @('reuse', 'Dll'),
    @('environment', 'Dll'), @('stream', 'Dll'), @('no-file-log', 'Dll'),
    @('legacy', 'Dll'), @('simple', 'Tests'), @('current', 'Tests'),
    @('simple', 'Dll'), @('current', 'Dll')
)
$rows = @()
$index = 0
foreach ($case in $cases) {
    $index++
    $name = '{0:d2}-{1}-{2}' -f $index, $case[0], $case[1]
    Write-Host "::group::$name"
    $directory = Join-Path $OutputDirectory $name
    & powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'run-case.ps1') -Variant $case[0] -Target $case[1] -RepoRoot $root -OutputDirectory $directory
    if ($LASTEXITCODE -ne 0) { throw "Comparison failed: $name" }
    $row = Get-Content -Raw -LiteralPath (Join-Path $directory 'result.json') | ConvertFrom-Json
    $rows += $row
    [IO.File]::WriteAllText((Join-Path $OutputDirectory 'results.json'), ($rows | ConvertTo-Json -Depth 6), [Text.UTF8Encoding]::new($false))
    Write-Host '::endgroup::'
}
if ((Get-EfmSourceFingerprint $root) -ne $source -or (Get-EfmSha256 (Join-Path $root 'bin\F-CK-1C_EFM.dll')) -ne $uploaded) { throw 'Comparison modified source or uploaded DLL.' }
$metadata = [ordered]@{ runner = $env:RUNNER_NAME; image = $env:ImageVersion; processors = $env:NUMBER_OF_PROCESSORS; toolchain = (Get-EfmBuildProfile $root); uploadedSha256 = $uploaded; cases = $rows }
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'report.json'), ($metadata | ConvertTo-Json -Depth 7), [Text.UTF8Encoding]::new($false))
$lines = @('## 同機建置比較', '', '| 順序 | 版本 | 目標 | 全程秒 | 建置秒 | CL 毫秒 | Link 毫秒 | 結果 |', '|---|---|---|---:|---:|---:|---:|---|')
$index = 0
foreach ($row in $rows) {
    $index++
    $cl = if ($row.tasksMs.PSObject.Properties['CL']) { $row.tasksMs.CL } else { '' }
    $link = if ($row.tasksMs.PSObject.Properties['Link']) { $row.tasksMs.Link } else { '' }
    $lines += "| $index | $($row.variant) | $($row.target) | $($row.wallSeconds) | $($row.buildSeconds) | $cl | $link | 通過 |"
}
$summary = $lines -join "`n"
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'summary.md'), $summary, [Text.UTF8Encoding]::new($false))
if ($env:GITHUB_STEP_SUMMARY) { [IO.File]::AppendAllText($env:GITHUB_STEP_SUMMARY, $summary + "`n", [Text.UTF8Encoding]::new($false)) }
