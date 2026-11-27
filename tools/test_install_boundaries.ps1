Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$fixture = Join-Path ([IO.Path]::GetTempPath()) ('fck-install-' + [guid]::NewGuid().ToString('N'))
function Get-Manifest([string]$Root, [string]$Allowed = '') {
    @(Get-ChildItem -LiteralPath $Root -Recurse -Force | Where-Object {
        -not $Allowed -or ($_.FullName -ne $Allowed -and -not $_.FullName.StartsWith($Allowed + '\', [StringComparison]::OrdinalIgnoreCase))
    } | ForEach-Object {
        $value = if ($_.PSIsContainer) { 'directory' } else { (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
        $_.FullName.Substring($Root.Length) + ':' + $value
    } | Sort-Object) -join "`n"
}
function Run-Installer([string]$Source, [string]$Target) {
    & $env:ComSpec /d /c ('call "' + (Join-Path $Source 'install.bat') + '" "' + $Target + '" < nul') | Out-Host
    return $LASTEXITCODE
}
try {
    $source = Join-Path $fixture 'source package'
    $saved = Join-Path $fixture 'Saved Games with spaces'
    $module = Join-Path $saved 'Mods\aircraft\F-CK-1C'
    New-Item -ItemType Directory -Path $source -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot '../install.bat') -Destination $source
    foreach ($name in @('entry.lua', 'F-CK-1C.lua', 'comm.lua', 'Views.lua')) {
        Set-Content -LiteralPath (Join-Path $source $name) -Value ('runtime ' + $name)
    }
    foreach ($name in @('bin', 'Cockpit', 'FM', 'Input', 'Liveries', 'Shapes', 'Sounds', 'Textures', 'FlightTest')) {
        $folder = Join-Path $source $name
        New-Item -ItemType Directory -Path $folder -Force | Out-Null
        Set-Content -LiteralPath (Join-Path $folder 'fixture.txt') -Value ('runtime ' + $name)
    }
    foreach ($name in @('Scripts\Export.lua', 'Scripts\Hooks\other.lua', 'Config\options.lua', 'Mods\aircraft\Other\entry.lua', 'Mods\aircraft\F-CK-1C\old.txt')) {
        $file = Join-Path $saved $name
        New-Item -ItemType Directory -Path (Split-Path -Parent $file) -Force | Out-Null
        Set-Content -LiteralPath $file -Value ('preserved ' + $name)
    }
    $before = Get-Manifest $saved $module
    if ((Run-Installer $source $saved) -ne 0) { throw 'Installer failed.' }
    if ((Get-Manifest $saved $module) -ne $before) { throw 'Installer changed a path outside F-CK-1C.' }
    foreach ($name in @('entry.lua', 'F-CK-1C.lua', 'bin\fixture.txt', 'FlightTest\fixture.txt')) {
        if (-not (Test-Path -LiteralPath (Join-Path $module $name))) { throw "Missing installed runtime: $name" }
    }
    Copy-Item -LiteralPath (Join-Path $source 'install.bat') -Destination $module
    $beforeSelf = Get-Manifest $module
    if ((Run-Installer $module ($saved + '\.')) -ne 0) { throw 'Already installed module was not recognized.' }
    if ((Get-Manifest $module) -ne $beforeSelf) { throw 'Running from the installed module changed its files.' }
    $game = Join-Path $fixture 'DCS game directory'
    New-Item -ItemType Directory -Path (Join-Path $game 'bin') -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $game 'bin\DCS.exe') -Value 'game sentinel'
    $beforeGame = Get-Manifest $game
    if ((Run-Installer $source $game) -eq 0) { throw 'Game directory was accepted as a Saved Games target.' }
    if ((Get-Manifest $game) -ne $beforeGame) { throw 'Game directory was modified.' }
    Remove-Item -LiteralPath (Join-Path $source 'F-CK-1C.lua')
    $beforeModule = Get-Manifest $module
    if ((Run-Installer $source $saved) -eq 0) { throw 'Incomplete source package was installed.' }
    if ((Get-Manifest $module) -ne $beforeModule) { throw 'Incomplete package removed an existing installation.' }
    Write-Output 'Installer boundary fixtures passed: module-only writes, game directory preserved, incomplete source preserved.'
}
finally {
    if (Test-Path -LiteralPath $fixture) { Remove-Item -LiteralPath $fixture -Recurse -Force }
}
