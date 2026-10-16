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
    $LogDirectory = [IO.Path]::GetFullPath($LogDirectory)
    $log = Join-Path $LogDirectory ($Target.ToLowerInvariant() + '-build.log')
    if (Test-Path -LiteralPath $log) { Remove-Item -LiteralPath $log -Force }
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
            ' /m /v:minimal /fl /flp:' + (ConvertTo-EfmBatchArgument ("LogFile=$log;Verbosity=normal;PerformanceSummary;ShowTimestamp;Encoding=UTF-8"))),
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