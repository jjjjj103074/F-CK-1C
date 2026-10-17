Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# Git hooks can pass PowerShell 7's module path to Windows PowerShell.
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Utility')

function Get-EfmSha256 {
    param([string]$Path)

    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
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
    foreach ($name in @('EfmPlatformToolset', 'EfmHostArchitecture', 'EfmVCToolsVersion', 'EfmWindowsSdkVersion',
            'EfmCompilerVersion', 'EfmLinkerVersion', 'EfmCanonicalSourceRoot', 'EfmMsBuildVersion')) {
        $nodes = $document.GetElementsByTagName($name)
        if ($nodes.Count -ne 1 -or [string]::IsNullOrWhiteSpace($nodes[0].InnerText)) {
            throw "建置設定缺少或重複：$name"
        }
        $values[$name] = $nodes[0].InnerText
    }
    foreach ($name in @('EfmVCToolsVersion', 'EfmWindowsSdkVersion', 'EfmCompilerVersion', 'EfmLinkerVersion', 'EfmMsBuildVersion')) {
        if ($values[$name] -notmatch '^\d+\.\d+\.\d+(\.\d+)?$') { throw "版本格式錯誤：$name" }
    }
    if ($values.EfmHostArchitecture -ne 'x64') { throw '建置配方需使用 x64 主機工具。' }
    return $values
}

function Get-EfmSourceFingerprint {
    param([string]$RepoRoot)

    $files = @(
        foreach ($directory in @('src\efm\F-CK-1C_EFM', 'src\efm\F-CK-1C_EFM_Tests', 'tools')) {
            Get-ChildItem -LiteralPath (Join-Path $RepoRoot $directory) -File -Recurse
        }
        Get-ChildItem -LiteralPath (Join-Path $RepoRoot 'src\efm') -File
        foreach ($directory in @('Cockpit', 'Input', 'FM')) {
            $path = Join-Path $RepoRoot $directory
            if (Test-Path -LiteralPath $path) { Get-ChildItem -LiteralPath $path -File -Recurse | Where-Object { $_.Extension -in @('.lua', '.jsonc') } }
        }
    ) | Where-Object { $_.Extension -in @('.cpp', '.c', '.cc', '.cxx', '.h', '.hh', '.hpp', '.hxx', '.inc', '.inl', '.ipp', '.tpp', '.ps1', '.mjs', '.props', '.targets', '.vcxproj', '.json', '.jsonc', '.txt', '.lua') } | Sort-Object FullName
    $lines = foreach ($file in $files) {
        $relative = $file.FullName.Substring($RepoRoot.TrimEnd('\').Length + 1).Replace('\', '/')
        $relative + '=' + (Get-EfmSha256 $file.FullName)
    }
    return $lines -join "`n"
}

function Assert-EfmToolchain {
    param([hashtable]$Profile, [string]$ToolchainRoot, [string]$SdkRoot = '', [string]$MsBuildPath = '')

    $toolchain = [IO.Path]::GetFullPath($ToolchainRoot).TrimEnd('\')
    $instance = [IO.Directory]::GetParent($toolchain).Parent.Parent.Parent.FullName
    if (-not $MsBuildPath) { $MsBuildPath = Join-Path $instance 'MSBuild\Current\Bin\MSBuild.exe' }
    if (-not $SdkRoot) { $SdkRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10' }
    $allowedBuilds = @((Join-Path $instance 'MSBuild\Current\Bin\MSBuild.exe'), (Join-Path $instance 'MSBuild\Current\Bin\amd64\MSBuild.exe'))
    if ([IO.Path]::GetFullPath($MsBuildPath) -notin $allowedBuilds) { throw '請使用同一套 Visual Studio 的 MSBuild。' }
    foreach ($tool in @(
        @{ Path = $MsBuildPath; Version = $Profile.EfmMsBuildVersion },
        @{ Path = (Join-Path $toolchain 'bin\Hostx64\x64\cl.exe'); Version = $Profile.EfmCompilerVersion },
        @{ Path = (Join-Path $toolchain 'bin\Hostx64\x64\link.exe'); Version = $Profile.EfmLinkerVersion })) {
        if (-not (Test-Path -LiteralPath $tool.Path -PathType Leaf)) { throw "缺少編譯工具：$($tool.Path)" }
        $info = (Get-Item -LiteralPath $tool.Path).VersionInfo
        $actual = "$($info.FileMajorPart).$($info.FileMinorPart).$($info.FileBuildPart).$($info.FilePrivatePart)"
        if ($actual -ne $tool.Version) {
            throw "工具版本不符：$([IO.Path]::GetFileName($tool.Path))。預期：$($tool.Version)；目前：$actual。請依 docs/BUILD_DLL.md 安裝。"
        }
    }
    foreach ($directory in @('Include', 'Lib')) {
        $path = Join-Path $SdkRoot "$directory\$($Profile.EfmWindowsSdkVersion)"
        if (-not (Test-Path -LiteralPath $path -PathType Container)) { throw "缺少 Windows SDK $($Profile.EfmWindowsSdkVersion)：$path" }
    }
}

function Select-EfmBuildTools {
    param([string]$RepoRoot, [object[]]$Instances, [string]$MsBuildPath = '', [string]$SdkRoot = '')

    $profile = Get-EfmBuildProfile $RepoRoot
    $requested = ''
    if ($MsBuildPath -and $MsBuildPath -ne 'MSBuild.exe') {
        $requested = (Get-Command $MsBuildPath -ErrorAction Stop).Source
    }
    $mismatches = @()
    foreach ($instance in $Instances) {
        $toolchain = Join-Path $instance.installationPath ('VC\Tools\MSVC\' + $profile.EfmVCToolsVersion)
        $build = Join-Path $instance.installationPath 'MSBuild\Current\Bin\MSBuild.exe'
        $developerShell = Join-Path $instance.installationPath 'Common7\Tools\VsDevCmd.bat'
        if ($requested) {
            if (-not $requested.StartsWith($instance.installationPath + '\', [StringComparison]::OrdinalIgnoreCase)) { continue }
            $build = $requested
        }
        if ((Test-Path -LiteralPath $build) -and
            (Test-Path -LiteralPath (Join-Path $toolchain 'bin\Hostx64\x64\cl.exe'))) {
            try { Assert-EfmToolchain -Profile $profile -ToolchainRoot $toolchain -SdkRoot $SdkRoot -MsBuildPath $build }
            catch {
                if ($requested) { throw }
                $mismatches += $_.Exception.Message
                continue
            }
            return [pscustomobject]@{ MsBuild = $build; DeveloperShell = $developerShell; Toolchain = $toolchain; Profile = $profile }
        }
    }
    throw "找不到符合配方的 MSBuild，請依 docs/BUILD_DLL.md 安裝。`n$($mismatches -join "`n")"
}

function Find-EfmBuildTools {
    param([string]$RepoRoot, [string]$MsBuildPath = '', [string]$SdkRoot = '')

    $vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswherePath)) { throw '缺少 Visual Studio Installer，請依 docs/BUILD_DLL.md 安裝。' }
    $instances = @((& $vswherePath -all -products '*' -requires Microsoft.Component.MSBuild -format json) | ConvertFrom-Json)
    if ($LASTEXITCODE -ne 0) { throw '無法讀取 Visual Studio 安裝清單。' }
    return Select-EfmBuildTools -RepoRoot $RepoRoot -Instances $instances -MsBuildPath $MsBuildPath -SdkRoot $SdkRoot
}

function ConvertTo-EfmBatchArgument {
    param([string]$Value)

    if ($Value -match '["\r\n\x00]') { throw '建置參數含無效字元。' }
    return '"' + $Value.Replace('%', '%%') + '"'
}

function Invoke-EfmNativeBuild {
    param([string]$RepoRoot, [ValidateSet('Dll', 'Tests')][string]$Target = 'Dll',
        [string]$MsBuildPath = '', [string]$LogDirectory = '', [switch]$Analyze)

    if ($env:OS -ne 'Windows_NT') { throw '請在 Windows 建置 EFM。' }
    $root = (Resolve-Path -LiteralPath $RepoRoot).Path
    $tools = Find-EfmBuildTools -RepoRoot $root -MsBuildPath $MsBuildPath
    if (-not (Test-Path -LiteralPath $tools.DeveloperShell)) { throw "找不到 Visual Studio 初始化腳本：$($tools.DeveloperShell)" }
    $projectName = if ($Target -eq 'Dll') { 'F-CK-1C_EFM' } else { 'F-CK-1C_EFM_Tests' }
    $project = Join-Path $root "src\efm\$projectName\$projectName.vcxproj"
    if (-not $LogDirectory) { $LogDirectory = Join-Path $root 'tmp\efm-build' }
    [IO.Directory]::CreateDirectory($LogDirectory) | Out-Null
    $LogDirectory = [IO.Path]::GetFullPath($LogDirectory)
    $log = Join-Path $LogDirectory ($Target.ToLowerInvariant() + '-build.log')
    if (Test-Path -LiteralPath $log) { Remove-Item -LiteralPath $log -Force }
    $helper = Join-Path $LogDirectory ('build-' + [Guid]::NewGuid().ToString('N') + '.cmd')
    $command = @(
        '@echo off', 'chcp 65001 >nul', 'setlocal DisableDelayedExpansion',
        ('call ' + (ConvertTo-EfmBatchArgument $tools.DeveloperShell) + ' -no_logo -arch=x64 -host_arch=x64 -vcvars_ver=' + $tools.Profile.EfmVCToolsVersion + ' -winsdk=' + $tools.Profile.EfmWindowsSdkVersion),
        'if errorlevel 1 exit /b %errorlevel%',
        'set "CL="', 'set "_CL_="', 'set "LINK="', 'set "_LINK_="',
        'set "VSLANG=1033"',
        ((ConvertTo-EfmBatchArgument $tools.MsBuild) + ' ' + (ConvertTo-EfmBatchArgument $project) +
            ' /noAutoResponse /t:Rebuild /p:Configuration=Release /p:Platform=x64 /p:UseEnv=false' +
            $(if ($Analyze) { ' /p:RunCodeAnalysis=true' } else { '' }) +
            ' /m /v:minimal /fl /flp:' + (ConvertTo-EfmBatchArgument ("LogFile=$log;Verbosity=normal;PerformanceSummary;ShowTimestamp;Encoding=UTF-8"))),
        'exit /b %errorlevel%'
    )
    [IO.File]::WriteAllText($helper, ($command -join "`r`n") + "`r`n", [Text.UTF8Encoding]::new($false))
    $watch = [Diagnostics.Stopwatch]::StartNew()
    try {
        Push-Location -LiteralPath $root
        try {
            & $env:ComSpec /d /c $helper 2>&1 | ForEach-Object { "$_" } | Out-Host
            $buildCode = $LASTEXITCODE
        }
        finally { Pop-Location }
        if ($buildCode -ne 0) { throw "EFM $Target 建置失敗（結束碼 $buildCode）。`n紀錄：$log" }
    }
    finally {
        $watch.Stop()
        Remove-Item -LiteralPath $helper -ErrorAction SilentlyContinue
    }
    $extension = if ($Target -eq 'Dll') { 'dll' } else { 'exe' }
    $output = Join-Path $root "src\efm\x64\Release\$projectName.$extension"
    if (-not (Test-Path -LiteralPath $output)) { throw "找不到建置產物：$output" }
    return [pscustomobject]@{ Output = $output; Log = $log; Seconds = [math]::Round($watch.Elapsed.TotalSeconds, 2); Toolchain = $tools.Toolchain }
}
