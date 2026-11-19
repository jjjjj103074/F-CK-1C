<#
.SYNOPSIS
    Read the F-CK-1C module version from entry.lua.

.DESCRIPTION
    Extracts the single local FCK1C_VERSION value used by DCS module metadata.

.EXAMPLE
    .\tools\get_release_version.ps1
#>

param(
    [string]$EntryPath = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir

if ([string]::IsNullOrWhiteSpace($EntryPath)) {
    $EntryPath = Join-Path $repoRoot 'entry.lua'
}

if (-not (Test-Path -LiteralPath $EntryPath -PathType Leaf)) {
    throw "entry.lua not found: $EntryPath"
}

$content = Get-Content -Raw -Encoding UTF8 -LiteralPath $EntryPath
$pattern = '(?m)^\s*local\s+FCK1C_VERSION\s*=\s*"([^"]+)"\s*$'
$matches = [regex]::Matches($content, $pattern)

if ($matches.Count -ne 1) {
    throw "Expected exactly one FCK1C_VERSION declaration in entry.lua, found $($matches.Count)."
}

$version = $matches[0].Groups[1].Value
if ($version -notmatch '^v\d+\.\d+\.\d+(-[A-Za-z0-9._-]+)?$') {
    throw "Invalid FCK1C_VERSION value: $version"
}

Write-Output $version
