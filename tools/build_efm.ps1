param([ValidateSet('Dll', 'Tests')][string]$Target = 'Dll', [string]$RepoRoot = '',
    [string]$MsBuildPath = '', [string]$LogDirectory = '', [switch]$Analyze)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'efm-build-common.ps1')
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent $PSScriptRoot }
$result = Invoke-EfmNativeBuild -RepoRoot $RepoRoot -Target $Target -MsBuildPath $MsBuildPath -LogDirectory $LogDirectory -Analyze:$Analyze
Write-Output "建置完成：$($result.Output)（$($result.Seconds) 秒）"
