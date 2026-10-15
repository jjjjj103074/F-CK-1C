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
    foreach ($name in @('EfmPlatformToolset', 'EfmHostArchitecture', 'EfmVCToolsVersion', 'EfmWindowsSdkVersion',
            'EfmCompilerVersion', 'EfmLinkerVersion', 'EfmCompilerSha256', 'EfmLinkerSha256',
            'EfmFrontendSha256', 'EfmBackendSha256', 'EfmLibcmtSha256', 'EfmLibcpmtSha256',
            'EfmLibvcruntimeSha256', 'EfmLibucrtSha256', 'EfmCanonicalSourceRoot', 'EfmMsBuildVersion',
            'EfmToolFilesSha256', 'EfmToolHeadersSha256', 'EfmToolLibrariesSha256', 'EfmSdkHeadersSha256',
            'EfmSdkLibrariesSha256', 'EfmSdkToolsSha256', 'EfmMsBuildFilesSha256', 'EfmCppTargetsSha256', 'EfmDevShellSha256', 'EfmVcSetupSha256')) {
        $nodes = $document.GetElementsByTagName($name)
        if ($nodes.Count -ne 1 -or [string]::IsNullOrWhiteSpace($nodes[0].InnerText)) {
            throw "建置設定缺少或重複：$name"
        }
        $values[$name] = $nodes[0].InnerText
    }
    foreach ($name in @('EfmVCToolsVersion', 'EfmWindowsSdkVersion', 'EfmCompilerVersion', 'EfmLinkerVersion', 'EfmMsBuildVersion')) {
        if ($values[$name] -notmatch '^\d+\.\d+\.\d+(\.\d+)?$') { throw "版本格式錯誤：$name" }
    }
    foreach ($name in @($values.Keys | Where-Object { $_.EndsWith('Sha256') })) {
        if ($values[$name] -notmatch '^[A-Fa-f0-9]{64}$') { throw "工具雜湊格式錯誤：$name" }
    }
    if ($values.EfmHostArchitecture -ne 'x64') { throw '建置配方需使用 x64 主機工具。' }
    return $values
}

function Get-EfmToolchainFingerprint {
    param([string]$ToolchainRoot, [string]$SdkRoot, [string]$SdkVersion)

    $instance = [IO.Directory]::GetParent($ToolchainRoot.TrimEnd('\')).Parent.Parent.Parent.FullName
    $groups = [ordered]@{
        EfmToolFilesSha256 = @((Join-Path $ToolchainRoot 'bin\Hostx64\x64'))
        EfmToolHeadersSha256 = @((Join-Path $ToolchainRoot 'include'))
        EfmToolLibrariesSha256 = @((Join-Path $ToolchainRoot 'lib\x64'))
        EfmSdkHeadersSha256 = @((Join-Path $SdkRoot "Include\$SdkVersion"))
        EfmSdkLibrariesSha256 = @((Join-Path $SdkRoot "Lib\$SdkVersion\ucrt\x64"), (Join-Path $SdkRoot "Lib\$SdkVersion\um\x64"))
        EfmSdkToolsSha256 = @((Join-Path $SdkRoot "bin\$SdkVersion\x64"))
        EfmMsBuildFilesSha256 = @((Join-Path $instance 'MSBuild\Current\Bin'))
        EfmCppTargetsSha256 = @((Join-Path $instance 'MSBuild\Microsoft\VC\v180'))
        EfmDevShellSha256 = @((Join-Path $instance 'Common7\Tools\VsDevCmd.bat'), (Join-Path $instance 'Common7\Tools\vsdevcmd'))
        EfmVcSetupSha256 = @((Join-Path $instance 'VC\Auxiliary\Build'))
    }
    $result = [ordered]@{}
    foreach ($group in $groups.GetEnumerator()) {
        $records = [Collections.Generic.List[string]]::new()
        for ($index = 0; $index -lt $group.Value.Count; $index++) {
            $root = (Get-Item -LiteralPath $group.Value[$index]).FullName.TrimEnd('\')
            $files = @(Get-ChildItem -LiteralPath $root -File -Recurse)
            if (-not $files.Count) { throw "工具鏈目錄為空：$root" }
            foreach ($file in $files) {
                $relative = if ($file.FullName -eq $root) { $file.Name } else { $file.FullName.Substring($root.Length + 1) }
                $relative = $relative.Replace('\', '/').ToLowerInvariant()
                # Localized diagnostics do not participate in compilation.
                if ($relative -match '(^|/)\d{4}/' -or $relative.EndsWith('.resources.dll')) { continue }
                $records.Add("$index/$relative=" + (Get-EfmSha256 $file.FullName))
            }
        }
        $records.Sort([StringComparer]::Ordinal)
        $algorithm = [Security.Cryptography.SHA256]::Create()
        try { $result[$group.Key] = [BitConverter]::ToString($algorithm.ComputeHash([Text.Encoding]::UTF8.GetBytes(($records -join "`n") + "`n"))).Replace('-', '') }
        finally { $algorithm.Dispose() }
    }
    return $result
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

function Select-EfmBuildTools {
    param([string]$RepoRoot, [object[]]$Instances, [string]$MsBuildPath = '')

    $profile = Get-EfmBuildProfile $RepoRoot
    $requested = ''
    if ($MsBuildPath -and $MsBuildPath -ne 'MSBuild.exe') {
        $requested = (Get-Command $MsBuildPath -ErrorAction Stop).Source
    }
    $mismatches = @()
    foreach ($instance in $Instances) {
        $toolchain = Join-Path $instance.installationPath ('VC\Tools\MSVC\' + $profile.EfmVCToolsVersion)
        $build = Join-Path $instance.installationPath 'MSBuild\Current\Bin\MSBuild.exe'
        if ($requested) {
            if (-not $requested.StartsWith($instance.installationPath + '\', [StringComparison]::OrdinalIgnoreCase)) { continue }
            $build = $requested
        }
        $developerShell = Join-Path $instance.installationPath 'Common7\Tools\VsDevCmd.bat'
        if ((Test-Path -LiteralPath $build) -and (Test-Path -LiteralPath $developerShell) -and
            (Test-Path -LiteralPath (Join-Path $toolchain 'bin\Hostx64\x64\cl.exe'))) {
            try {
                & (Join-Path $PSScriptRoot 'check_efm_toolchain.ps1') -RepoRoot $RepoRoot -ToolchainRoot $toolchain -MsBuildPath $build | Out-Null
            }
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
    param([string]$RepoRoot, [string]$MsBuildPath = '')

    $vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswherePath)) { throw '缺少 Visual Studio Installer，請依 docs/BUILD_DLL.md 安裝。' }
    $instances = @((& $vswherePath -all -products '*' -requires Microsoft.Component.MSBuild -format json) | ConvertFrom-Json)
    if ($LASTEXITCODE -ne 0) { throw '無法讀取 Visual Studio 安裝清單。' }
    return Select-EfmBuildTools -RepoRoot $RepoRoot -Instances $instances -MsBuildPath $MsBuildPath
}

function ConvertTo-EfmBatchArgument {
    param([string]$Value)

    if ($Value -match '["\r\n\x00]') { throw '建置參數含無效字元。' }
    return '"' + $Value.Replace('%', '%%') + '"'
}

function Invoke-EfmNativeBuild {
    param([string]$RepoRoot, [ValidateSet('Dll', 'Tests')][string]$Target = 'Dll',
        [string]$MsBuildPath = '', [string]$LogDirectory = '')

    if ($env:OS -ne 'Windows_NT') { throw '請在 Windows 建置 EFM。' }
    $root = (Resolve-Path -LiteralPath $RepoRoot).Path
    $tools = Find-EfmBuildTools -RepoRoot $root -MsBuildPath $MsBuildPath
    $projectName = if ($Target -eq 'Dll') { 'F-CK-1C_EFM' } else { 'F-CK-1C_EFM_Tests' }
    $project = Join-Path $root "src\efm\$projectName\$projectName.vcxproj"
    if (-not $LogDirectory) { $LogDirectory = Join-Path $root 'tmp\efm-build' }
    [IO.Directory]::CreateDirectory($LogDirectory) | Out-Null
    $log = Join-Path $LogDirectory ($Target.ToLowerInvariant() + '-build.log')
    $helper = Join-Path $LogDirectory ('build-' + [Guid]::NewGuid().ToString('N') + '.cmd')
    $userImports = Join-Path $LogDirectory ('msbuild-user-' + [Guid]::NewGuid().ToString('N'))
    [IO.Directory]::CreateDirectory($userImports) | Out-Null
    $command = @(
        '@echo off', 'chcp 65001 >nul', 'setlocal DisableDelayedExpansion',
        ('call ' + (ConvertTo-EfmBatchArgument $tools.DeveloperShell) + ' -no_logo -arch=x64 -host_arch=x64 -vcvars_ver=' + $tools.Profile.EfmVCToolsVersion + ' -winsdk=' + $tools.Profile.EfmWindowsSdkVersion),
        'if errorlevel 1 exit /b %errorlevel%',
        'set "CL="', 'set "_CL_="', 'set "LINK="', 'set "_LINK_="',
        ((ConvertTo-EfmBatchArgument $tools.MsBuild) + ' ' + (ConvertTo-EfmBatchArgument $project) +
            ' /noAutoResponse /nr:false /t:Rebuild /p:Configuration=Release /p:Platform=x64 /p:UseEnv=false /p:VcpkgEnabled=false' +
            ' /p:MSBuildUserExtensionsPath=' + (ConvertTo-EfmBatchArgument $userImports) +
            ' /m /v:minimal /fl /flp:' + (ConvertTo-EfmBatchArgument ("LogFile=$log;Verbosity=normal;Encoding=UTF-8"))),
        'exit /b %errorlevel%'
    )
    [IO.File]::WriteAllText($helper, ($command -join "`r`n") + "`r`n", [Text.UTF8Encoding]::new($false))
    $watch = [Diagnostics.Stopwatch]::StartNew()
    $process = [Diagnostics.Process]::new()
    try {
        $start = [Diagnostics.ProcessStartInfo]::new()
        $start.FileName = Join-Path $env:SystemRoot 'System32\cmd.exe'
        $start.Arguments = '/d /c "' + $helper + '"'
        $start.WorkingDirectory = $root
        $start.UseShellExecute = $false
        $start.CreateNoWindow = $true
        $start.RedirectStandardOutput = $true
        $start.RedirectStandardError = $true
        $start.EnvironmentVariables.Clear()
        foreach ($name in @('SystemRoot', 'SystemDrive', 'windir', 'ComSpec', 'ProgramFiles', 'ProgramFiles(x86)',
                'ProgramW6432', 'ProgramData', 'ALLUSERSPROFILE', 'USERPROFILE', 'APPDATA', 'LOCALAPPDATA',
                'TEMP', 'TMP', 'OS', 'PROCESSOR_ARCHITECTURE', 'NUMBER_OF_PROCESSORS')) {
            $value = [Environment]::GetEnvironmentVariable($name)
            if ($value) { $start.EnvironmentVariables[$name] = $value }
        }
        $start.EnvironmentVariables['PATH'] = "$env:SystemRoot\System32;$env:SystemRoot;$env:SystemRoot\System32\WindowsPowerShell\v1.0"
        $start.EnvironmentVariables['VSLANG'] = '1033'
        $process.StartInfo = $start
        $process.Start() | Out-Null
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        $process.WaitForExit()
        $outputText = $stdout.GetAwaiter().GetResult() + $stderr.GetAwaiter().GetResult()
        Write-Host $outputText
        if (-not (Test-Path -LiteralPath $log)) { [IO.File]::WriteAllText($log, $outputText, [Text.UTF8Encoding]::new($false)) }
        $buildCode = $process.ExitCode
        if ($buildCode -ne 0) { throw "EFM $Target 建置失敗（結束碼 $buildCode）。`n紀錄：$log" }
    }
    finally {
        $watch.Stop()
        $process.Dispose()
        Remove-Item -LiteralPath $helper -ErrorAction SilentlyContinue
        Remove-Item -LiteralPath $userImports -ErrorAction SilentlyContinue
    }
    $extension = if ($Target -eq 'Dll') { 'dll' } else { 'exe' }
    $output = Join-Path $root "src\efm\x64\Release\$projectName.$extension"
    if (-not (Test-Path -LiteralPath $output)) { throw "找不到建置產物：$output" }
    return [pscustomobject]@{ Output = $output; Log = $log; Seconds = [math]::Round($watch.Elapsed.TotalSeconds, 2); Toolchain = $tools.Toolchain }
}
