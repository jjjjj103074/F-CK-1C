<# Record Microsoft Defender scan evidence for a release download. #>
param(
    [Parameter(Mandatory = $true)][string]$Path,
    [Parameter(Mandatory = $true)][string]$ReportDirectory
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Path $ReportDirectory -Force | Out-Null
$report = [ordered]@{ status = 'not_run'; scanner = 'Microsoft Defender'; file = (Split-Path -Leaf $Path) }
$failure = $null
try {
    $report.sha256 = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
    $service = Get-Service WinDefend
    if ($service.Status -ne 'Running') {
        Set-Service WinDefend -StartupType Manual
        Start-Service WinDefend
    }
    Set-MpPreference -DisableArchiveScanning $false -DisableScriptScanning $false -ScanAvgCPULoadFactor 25
    Update-MpSignature
    $state = Get-MpComputerStatus
    $report.engine_version = $state.AMEngineVersion
    $report.signature_version = $state.AntivirusSignatureVersion
    $report.signature_updated = $state.AntivirusSignatureLastUpdated.ToString('o')
    $platform = Join-Path $env:ProgramData 'Microsoft\Windows Defender\Platform'
    $commands = @(Get-ChildItem -Path (Join-Path $platform '*\MpCmdRun.exe') -ErrorAction SilentlyContinue | Sort-Object FullName -Descending)
    $command = if ($commands.Count) { $commands[0].FullName } else { Join-Path $env:ProgramFiles 'Windows Defender\MpCmdRun.exe' }
    & $command -Scan -ScanType 3 -File (Resolve-Path -LiteralPath $Path).Path -DisableRemediation 2>&1 |
        Tee-Object -FilePath (Join-Path $ReportDirectory 'defender.log') | Out-Host
    $report.exit_code = $LASTEXITCODE
    $report.status = if ($LASTEXITCODE -eq 0) { 'no_detection' } else { 'scan_failed_or_detection' }
    if ($LASTEXITCODE -ne 0) { throw "Defender returned $LASTEXITCODE; inspect defender.log." }
}
catch {
    $report.error = $_.Exception.Message
    $failure = $_
}
finally {
    $report | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $ReportDirectory 'scan.json') -Encoding UTF8
}
if ($failure) { throw $failure }
