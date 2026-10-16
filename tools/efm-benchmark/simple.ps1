function Invoke-EfmNativeBuild {
    param([string]$RepoRoot, [ValidateSet('Dll', 'Tests')][string]$Target = 'Dll',
        [string]$MsBuildPath = '', [string]$LogDirectory = '')

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