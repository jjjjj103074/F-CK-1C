param([Parameter(Mandatory = $true)][string]$HeadSha,
    [Parameter(Mandatory = $true)][string]$BaseSha,
    [Parameter(Mandatory = $true)][string]$OutputDirectory, [string]$RepoRoot = '')

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent $PSScriptRoot }
if ($HeadSha -notmatch '^[a-fA-F0-9]{40}$' -or $BaseSha -notmatch '^[a-fA-F0-9]{40}$') { throw '合併版本需為完整提交 SHA。' }
$actualHead = (& git -C $RepoRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $actualHead -ne $HeadSha) { throw '來源版本不符，請重新取出 PR。' }
& git -C $RepoRoot diff --exit-code HEAD
if ($LASTEXITCODE -ne 0) { throw '來源有未提交變更，請重新取出 PR。' }
[IO.Directory]::CreateDirectory($OutputDirectory) | Out-Null
$controller = Join-Path $OutputDirectory 'controller'
[IO.Directory]::CreateDirectory($controller) | Out-Null
Copy-Item -Path (Join-Path $RepoRoot 'tools\*.ps1') -Destination $controller
Copy-Item -LiteralPath (Join-Path $RepoRoot 'bin\F-CK-1C_EFM.dll') -Destination (Join-Path $OutputDirectory 'pr-uploaded.dll') -Force
# Preserve first: a one-sided base change can replace the DLL during merging.
& git -C $RepoRoot merge --no-commit --no-ff $BaseSha
if ($LASTEXITCODE -ne 0) { throw '合併衝突，請同步目標分支並解決衝突。' }
Write-Output '模擬合併完成，已保留 PR 原 DLL。'
