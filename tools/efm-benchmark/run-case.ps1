param([string]$Variant, [string]$Target, [string]$RepoRoot, [string]$OutputDirectory)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $RepoRoot 'tools\efm-build-common.ps1')
. (Join-Path $PSScriptRoot ($Variant + '.ps1'))
$watch = [Diagnostics.Stopwatch]::StartNew()
$result = Invoke-EfmNativeBuild -RepoRoot $RepoRoot -Target $Target -LogDirectory $OutputDirectory
$watch.Stop()
$hash = Get-EfmSha256 $result.Output
if ($Target -eq 'Dll') {
    if ($hash -ne (Get-EfmSha256 (Join-Path $RepoRoot 'bin\F-CK-1C_EFM.dll'))) { throw "DLL mismatch: $Variant" }
}
else {
    & $result.Output
    if ($LASTEXITCODE -ne 0) { throw "Native tests failed: $Variant" }
}
$tasks = @{}
foreach ($line in Get-Content -LiteralPath $result.Log) {
    if ($line -match '^\s*(\d+) ms\s+(CL|Link|Exec)\s+\d+ calls') { $tasks[$matches[2]] = [int]$matches[1] }
}
$report = [ordered]@{ variant = $Variant; target = $Target; wallSeconds = [math]::Round($watch.Elapsed.TotalSeconds, 2); buildSeconds = $result.Seconds; sha256 = $hash; tasksMs = $tasks; status = 'passed' }
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'result.json'), ($report | ConvertTo-Json -Depth 5), [Text.UTF8Encoding]::new($false))
Write-Host ('RESULT ' + ($report | ConvertTo-Json -Depth 5 -Compress))
