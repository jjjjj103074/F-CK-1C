param([Parameter(Mandatory = $true)][string]$HeadSha,
    [Parameter(Mandatory = $true)][string]$BaseSha,
    [Parameter(Mandatory = $true)][string]$OutputDirectory, [string]$RepoRoot = '')

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent $PSScriptRoot }
if ($HeadSha -notmatch '^[a-fA-F0-9]{40}$' -or $BaseSha -notmatch '^[a-fA-F0-9]{40}$') { throw 'Merge inputs must be full commit SHAs.' }
$actualHead = (& git -C $RepoRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $actualHead -ne $HeadSha) { throw 'Merge checkout does not match the submitted PR commit.' }
& git -C $RepoRoot diff --exit-code HEAD
if ($LASTEXITCODE -ne 0) { throw 'Merge preparation requires a clean checkout.' }
[IO.Directory]::CreateDirectory($OutputDirectory) | Out-Null
$controller = Join-Path $OutputDirectory 'controller'
[IO.Directory]::CreateDirectory($controller) | Out-Null
Copy-Item -Path (Join-Path $RepoRoot 'tools\*.ps1') -Destination $controller
Copy-Item -LiteralPath (Join-Path $RepoRoot 'bin\F-CK-1C_EFM.dll') -Destination (Join-Path $OutputDirectory 'pr-uploaded.dll') -Force
# Preserve first: a one-sided base change can replace the DLL during merging.
& git -C $RepoRoot merge --no-commit --no-ff $BaseSha
if ($LASTEXITCODE -ne 0) { throw 'Merge conflict: synchronize the PR branch and resolve it locally.' }
Write-Output "Simulated merge of $BaseSha into $HeadSha; original PR DLL preserved."
