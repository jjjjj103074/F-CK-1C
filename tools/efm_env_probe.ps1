param([string]$OutputDirectory)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $root 'tmp\efm-env-probe' }
[IO.Directory]::CreateDirectory($OutputDirectory) | Out-Null
. (Join-Path $PSScriptRoot 'efm-build-common.ps1')
$tools = Find-EfmBuildTools $root
$keep = @('SystemRoot', 'SystemDrive', 'windir', 'ComSpec', 'ProgramFiles', 'ProgramFiles(x86)', 'ProgramW6432', 'ProgramData', 'ALLUSERSPROFILE', 'USERPROFILE', 'APPDATA', 'LOCALAPPDATA', 'TEMP', 'TMP', 'OS', 'PROCESSOR_ARCHITECTURE', 'NUMBER_OF_PROCESSORS')
$rows = @()
foreach ($mode in @('inherited', 'whitelist', 'whitelist-PSModulePath', 'whitelist-PATH', 'whitelist-PSModulePath', 'whitelist')) {
    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
    $start.Arguments = '-NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' + (Join-Path $PSScriptRoot 'check_efm_toolchain.ps1') + '" -RepoRoot "' + $root + '" -ToolchainRoot "' + $tools.Toolchain + '" -MsBuildPath "' + $tools.MsBuild + '"'
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    if ($mode -ne 'inherited') {
        $start.EnvironmentVariables.Clear()
        foreach ($name in $keep) { $value = [Environment]::GetEnvironmentVariable($name); if ($value) { $start.EnvironmentVariables[$name] = $value } }
        $start.EnvironmentVariables['PATH'] = "$env:SystemRoot\System32;$env:SystemRoot;$env:SystemRoot\System32\WindowsPowerShell\v1.0"
        if ($mode -eq 'whitelist-PSModulePath') { $start.EnvironmentVariables['PSModulePath'] = $env:PSModulePath }
        if ($mode -eq 'whitelist-PATH') { $start.EnvironmentVariables['PATH'] = $env:PATH }
    }
    $start.EnvironmentVariables['VSLANG'] = '1033'
    $watch = [Diagnostics.Stopwatch]::StartNew()
    $process = [Diagnostics.Process]::Start($start)
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    $process.WaitForExit()
    $text = $stdout.GetAwaiter().GetResult() + $stderr.GetAwaiter().GetResult()
    $watch.Stop()
    if ($process.ExitCode -ne 0) { throw "$mode failed: $text" }
    $process.Dispose()
    $row = [pscustomobject]@{ mode = $mode; seconds = [math]::Round($watch.Elapsed.TotalSeconds, 2); status = 'passed' }
    $rows += $row
    Write-Host ($row | ConvertTo-Json -Compress)
}
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'results.json'), ($rows | ConvertTo-Json), [Text.UTF8Encoding]::new($false))
$summary = @('## PowerShell environment timing', '', '| Environment | Seconds | Result |', '|---|---:|---|')
foreach ($row in $rows) { $summary += "| $($row.mode) | $($row.seconds) | passed |" }
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'summary.md'), ($summary -join "`n"), [Text.UTF8Encoding]::new($false))
if ($env:GITHUB_STEP_SUMMARY) { [IO.File]::AppendAllText($env:GITHUB_STEP_SUMMARY, ($summary -join "`n"), [Text.UTF8Encoding]::new($false)) }
