<#
.SYNOPSIS
    Create editable GitHub Release notes from the project template.

.DESCRIPTION
    Copies the user-facing part of .github/RELEASE_NOTES_TEMPLATE.md, replaces
    the placeholder version heading, and omits the authoring guidelines.

.EXAMPLE
    .\tools\prepare_release_notes.ps1 -Version v0.2.1 -OutputPath dist\release-notes.md
#>

param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[A-Za-z0-9._-]+$')]
    [string]$Version,

    [Parameter(Mandatory = $true)]
    [string]$OutputPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir
$templatePath = Join-Path $repoRoot '.github\RELEASE_NOTES_TEMPLATE.md'

if (-not (Test-Path -LiteralPath $templatePath -PathType Leaf)) {
    throw "Release notes template not found: $templatePath"
}

$template = Get-Content -Raw -Encoding UTF8 -LiteralPath $templatePath
$body = ($template -split '(?m)^---\s*$')[0].TrimEnd()
$body = $body -replace '^# F-CK-1C vX\.Y\.Z', "# F-CK-1C $Version"

$outputDir = Split-Path -Parent $OutputPath
if (-not [string]::IsNullOrWhiteSpace($outputDir)) {
    New-Item -Path $outputDir -ItemType Directory -Force | Out-Null
}

Set-Content -Path $OutputPath -Value $body -Encoding UTF8
