param(
    [string]$OmcPath = 'C:\Program Files\OpenModelica1.27.0-64bit\bin\omc.exe'
)

function Remove-VerificationArtifacts {
    Get-ChildItem -LiteralPath $PSScriptRoot -File |
        Where-Object { $_.Name -match '^figure31_verify(_|\.|$)' } |
        ForEach-Object { Remove-Item -LiteralPath $_.FullName -Force }
}

$outputPath = [IO.Path]::GetTempFileName()
$errorPath = [IO.Path]::GetTempFileName()
$process = Start-Process -FilePath $OmcPath `
    -ArgumentList 'verify_execution.mos' `
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
    throw 'Figure 3.1 verification exceeded the 60 second timeout.'
} finally {
    Remove-Item -LiteralPath $outputPath, $errorPath -Force
    Remove-VerificationArtifacts
}

Write-Output $output
if ($errors) {
    Write-Error $errors
}

if ($process.ExitCode -ne 0 -or
    $output -match 'resultFile = ""' -or
    $output -match 'LOG_ASSERT') {
    throw 'Figure 3.1 is not executable. Review the diagnostics above.'
}
