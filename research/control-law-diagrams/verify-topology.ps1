param(
    [string]$PythonPath = 'python'
)

$outputPath = [IO.Path]::GetTempFileName()
$errorPath = [IO.Path]::GetTempFileName()
$process = Start-Process -FilePath $PythonPath `
    -ArgumentList 'validate_topology.py' `
    -WorkingDirectory $PSScriptRoot `
    -RedirectStandardOutput $outputPath `
    -RedirectStandardError $errorPath `
    -PassThru `
    -WindowStyle Hidden

try {
    Wait-Process -Id $process.Id -Timeout 60 -ErrorAction Stop
    $output = Get-Content $outputPath -Raw
    $errors = Get-Content $errorPath -Raw
} catch {
    Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    throw 'Topology verification exceeded the 60 second timeout.'
} finally {
    Remove-Item -LiteralPath $outputPath, $errorPath -Force
}

Write-Output $output
if ($errors) {
    Write-Error $errors
}
if ($process.ExitCode -ne 0) {
    throw 'Topology contract failed. Review the diagnostics above.'
}
