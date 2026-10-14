Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-EfmSha256 {
    param([string]$Path)

    $stream = [IO.File]::OpenRead($Path)
    $algorithm = [Security.Cryptography.SHA256]::Create()
    try { return [BitConverter]::ToString($algorithm.ComputeHash($stream)).Replace('-', '') }
    finally { $algorithm.Dispose(); $stream.Dispose() }
}

function Get-EfmBuildProfile {
    param([string]$RepoRoot)

    $profilePath = Join-Path $RepoRoot 'src\efm\EfmBuild.props'
    $settings = [Xml.XmlReaderSettings]::new()
    $settings.DtdProcessing = [Xml.DtdProcessing]::Prohibit
    $reader = [Xml.XmlReader]::Create($profilePath, $settings)
    try {
        $document = [Xml.XmlDocument]::new()
        $document.Load($reader)
    }
    finally { $reader.Dispose() }
    $values = @{}
    foreach ($name in @('EfmPlatformToolset', 'EfmVCToolsVersion', 'EfmWindowsSdkVersion',
            'EfmCompilerVersion', 'EfmLinkerVersion', 'EfmCompilerSha256', 'EfmLinkerSha256',
            'EfmFrontendSha256', 'EfmBackendSha256', 'EfmCanonicalSourceRoot')) {
        $nodes = $document.GetElementsByTagName($name)
        if ($nodes.Count -ne 1 -or [string]::IsNullOrWhiteSpace($nodes[0].InnerText)) {
            throw "Invalid build profile property: $name"
        }
        $values[$name] = $nodes[0].InnerText
    }
    foreach ($name in @('EfmVCToolsVersion', 'EfmWindowsSdkVersion', 'EfmCompilerVersion', 'EfmLinkerVersion')) {
        if ($values[$name] -notmatch '^\d+\.\d+\.\d+(\.\d+)?$') { throw "Invalid version: $name" }
    }
    foreach ($name in @('EfmCompilerSha256', 'EfmLinkerSha256', 'EfmFrontendSha256', 'EfmBackendSha256')) {
        if ($values[$name] -notmatch '^[A-Fa-f0-9]{64}$') { throw "Invalid tool hash: $name" }
    }
    return $values
}

function Get-EfmSourceFingerprint {
    param([string]$RepoRoot)

    $files = @(
        foreach ($directory in @('src\efm\F-CK-1C_EFM', 'src\efm\F-CK-1C_EFM_Tests', 'tools')) {
            Get-ChildItem -LiteralPath (Join-Path $RepoRoot $directory) -File -Recurse
        }
        Get-ChildItem -LiteralPath (Join-Path $RepoRoot 'src\efm') -File
        foreach ($directory in @('Cockpit', 'Input')) {
            $path = Join-Path $RepoRoot $directory
            if (Test-Path -LiteralPath $path) { Get-ChildItem -LiteralPath $path -File -Recurse -Filter '*.lua' }
        }
    ) | Where-Object { $_.Extension -in @('.cpp', '.c', '.cc', '.cxx', '.h', '.hh', '.hpp', '.hxx', '.inc', '.inl', '.ipp', '.tpp', '.ps1', '.mjs', '.props', '.targets', '.vcxproj', '.json', '.txt', '.lua') } | Sort-Object FullName
    $lines = foreach ($file in $files) {
        $relative = $file.FullName.Substring($RepoRoot.TrimEnd('\').Length + 1).Replace('\', '/')
        $relative + '=' + (Get-EfmSha256 $file.FullName)
    }
    return $lines -join "`n"
}

function Find-EfmBuildTools {
    param([string]$RepoRoot, [string]$MsBuildPath = '')

    $profile = Get-EfmBuildProfile $RepoRoot
    $vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswherePath)) { throw 'Visual Studio Installer/vswhere was not found. Install C++ Build Tools; see docs/BUILD_DLL.md.' }
    $instances = @((& $vswherePath -all -products '*' -requires Microsoft.Component.MSBuild -format json) | ConvertFrom-Json)
    if ($LASTEXITCODE -ne 0) { throw 'vswhere could not locate MSBuild.' }
    $requested = ''
    if ($MsBuildPath -and $MsBuildPath -ne 'MSBuild.exe') {
        $requested = (Get-Command $MsBuildPath -ErrorAction Stop).Source
    }
    foreach ($instance in $instances) {
        $toolchain = Join-Path $instance.installationPath ('VC\Tools\MSVC\' + $profile.EfmVCToolsVersion)
        $build = Join-Path $instance.installationPath 'MSBuild\Current\Bin\MSBuild.exe'
        if ($requested) {
            if (-not $requested.StartsWith($instance.installationPath + '\', [StringComparison]::OrdinalIgnoreCase)) { continue }
            $build = $requested
        }
        $developerShell = Join-Path $instance.installationPath 'Common7\Tools\VsDevCmd.bat'
        if ((Test-Path -LiteralPath $build) -and (Test-Path -LiteralPath $developerShell) -and
            (Test-Path -LiteralPath (Join-Path $toolchain 'bin\Hostx64\x64\cl.exe'))) {
            return [pscustomobject]@{ MsBuild = $build; DeveloperShell = $developerShell; Toolchain = $toolchain; Profile = $profile }
        }
    }
    throw "MSBuild with MSVC $($profile.EfmVCToolsVersion) was not found. Install the pinned v142 component; see docs/BUILD_DLL.md."
}

function ConvertTo-EfmBatchArgument {
    param([string]$Value)

    if ($Value -match '["\r\n\x00]') { throw 'A build argument contains a quote, newline, or NUL.' }
    return '"' + $Value.Replace('%', '%%') + '"'
}

function Invoke-EfmNativeBuild {
    param([string]$RepoRoot, [ValidateSet('Dll', 'Tests')][string]$Target = 'Dll',
        [string]$MsBuildPath = '', [string]$LogDirectory = '')

    if ($env:OS -ne 'Windows_NT') { throw 'EFM compilation requires Windows. Linux is supported for editing only.' }
    $root = (Resolve-Path -LiteralPath $RepoRoot).Path
    $tools = Find-EfmBuildTools -RepoRoot $root -MsBuildPath $MsBuildPath
    $projectName = if ($Target -eq 'Dll') { 'F-CK-1C_EFM' } else { 'F-CK-1C_EFM_Tests' }
    $project = Join-Path $root "src\efm\$projectName\$projectName.vcxproj"
    if (-not $LogDirectory) { $LogDirectory = Join-Path $root 'tmp\efm-build' }
    [IO.Directory]::CreateDirectory($LogDirectory) | Out-Null
    $log = Join-Path $LogDirectory ($Target.ToLowerInvariant() + '-build.log')
    $helper = Join-Path $LogDirectory ('build-' + [Guid]::NewGuid().ToString('N') + '.cmd')
    $command = @(
        '@echo off', 'chcp 65001 >nul', 'setlocal DisableDelayedExpansion',
        ('call ' + (ConvertTo-EfmBatchArgument $tools.DeveloperShell) + ' -no_logo -arch=x64 -host_arch=x64 -vcvars_ver=' + $tools.Profile.EfmVCToolsVersion + ' -winsdk=' + $tools.Profile.EfmWindowsSdkVersion),
        'if errorlevel 1 exit /b %errorlevel%',
        'set "CL="', 'set "_CL_="', 'set "LINK="', 'set "_LINK_="',
        ((ConvertTo-EfmBatchArgument $tools.MsBuild) + ' ' + (ConvertTo-EfmBatchArgument $project) + ' /t:Rebuild /p:Configuration=Release /p:Platform=x64 /m /v:minimal'),
        'exit /b %errorlevel%'
    )
    [IO.File]::WriteAllText($helper, ($command -join "`r`n") + "`r`n", [Text.UTF8Encoding]::new($false))
    $watch = [Diagnostics.Stopwatch]::StartNew()
    try {
        & cmd.exe /d /c $helper 2>&1 | ForEach-Object { "$($_)" } | Tee-Object -FilePath $log | Out-Host
        $buildCode = $LASTEXITCODE
        if ($buildCode -ne 0) { throw "EFM $Target build failed (exit $buildCode). Log: $log" }
    }
    finally {
        $watch.Stop()
        Remove-Item -LiteralPath $helper -ErrorAction SilentlyContinue
    }
    $extension = if ($Target -eq 'Dll') { 'dll' } else { 'exe' }
    $output = Join-Path $root "src\efm\x64\Release\$projectName.$extension"
    if (-not (Test-Path -LiteralPath $output)) { throw "Build succeeded but output is missing: $output" }
    return [pscustomobject]@{ Output = $output; Log = $log; Seconds = [math]::Round($watch.Elapsed.TotalSeconds, 2); Toolchain = $tools.Toolchain }
}
