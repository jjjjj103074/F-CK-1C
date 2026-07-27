param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$checker = Join-Path $PSScriptRoot 'check_efm_architecture.ps1'
$temporaryRoot = [IO.Path]::GetFullPath(
    (Join-Path ([IO.Path]::GetTempPath()) (
        'fck1c_architecture_' + [Guid]::NewGuid())))
$moduleRoot = Join-Path $temporaryRoot 'F-CK-1C_EFM'
$utf8 = [Text.UTF8Encoding]::new($false)

function Write-Fixture {
    param(
        [string]$RelativePath,
        [string]$Content
    )

    $path = Join-Path $moduleRoot $RelativePath
    [IO.Directory]::CreateDirectory((Split-Path -Parent $path)) | Out-Null
    [IO.File]::WriteAllText($path, $Content, $utf8)
}

function Remove-Fixture {
    param([string]$RelativePath)

    Remove-Item -LiteralPath (Join-Path $moduleRoot $RelativePath) -Force
}

function Invoke-Checker {
    & $checker -ModuleRoot $moduleRoot | Out-Null
}

function Assert-Rejected {
    param(
        [scriptblock]$Arrange,
        [string]$ExpectedMessage,
        [string]$FixturePath
    )

    & $Arrange
    try {
        Invoke-Checker
        throw "Checker accepted invalid fixture: $FixturePath"
    }
    catch {
        if ($_.Exception.Message -notlike "*$ExpectedMessage*") {
            throw
        }
    }
    finally {
        Remove-Fixture $FixturePath
    }
}

function Assert-RejectedReplacement {
    param(
        [string]$FixturePath,
        [string]$InvalidContent,
        [string]$ExpectedMessage
    )

    $path = Join-Path $moduleRoot $FixturePath
    $originalContent = [IO.File]::ReadAllText($path)
    Write-Fixture $FixturePath $InvalidContent
    try {
        Invoke-Checker
        throw "Checker accepted invalid fixture: $FixturePath"
    }
    catch {
        if ($_.Exception.Message -notlike "*$ExpectedMessage*") {
            throw
        }
    }
    finally {
        Write-Fixture $FixturePath $originalContent
    }
}

try {
    Write-Fixture 'Core\Fck1cEfm.h' "#pragma once`r`n"
    Write-Fixture 'Core\Fck1cEfm.cpp' "#include `"Fck1cEfm.h`"`r`n"
    Write-Fixture 'Core\Systems\Alpha\Alpha.h' "#pragma once`r`n"
    Write-Fixture 'Core\Systems\Bravo\Bravo.h' "#pragma once`r`n"
    Write-Fixture 'Core\Simulation\Models\Lift\Lift.h' "#pragma once`r`n"
    Write-Fixture 'DcsBridge\Bridge.cpp' "#include `"../Core/Fck1cEfm.h`"`r`n"
    Invoke-Checker

    Assert-Rejected {
        Write-Fixture 'DcsBridge\Invalid.cpp' `
            "#include `"../Core/Systems/Alpha/Alpha.h`"`r`n"
    } 'DcsBridge boundary violation' 'DcsBridge\Invalid.cpp'

    Assert-Rejected {
        Write-Fixture 'DcsBridge\Invalid.cpp' `
            "#include `"../Core/Simulation/Models/Lift/Lift.h`"`r`n"
    } 'DcsBridge boundary violation' 'DcsBridge\Invalid.cpp'

    Assert-Rejected {
        Write-Fixture 'Core\Systems\Alpha\Invalid.cpp' `
            "#include `"../Bravo/Bravo.h`"`r`n"
    } 'Cross-System dependency' 'Core\Systems\Alpha\Invalid.cpp'

    Assert-Rejected {
        Write-Fixture 'Core\Systems\Alpha\Invalid.cpp' `
            "#include `"../../../DcsBridge/Bridge.cpp`"`r`n"
    } 'System boundary violation' 'Core\Systems\Alpha\Invalid.cpp'

    Assert-Rejected {
        Write-Fixture 'Core\Systems\Alpha\Invalid.cpp' `
            "#include `"../../Simulation/Models/Lift/Lift.h`"`r`n"
    } 'System boundary violation' 'Core\Systems\Alpha\Invalid.cpp'

    Assert-Rejected {
        Write-Fixture 'Core\Simulation\Models\Lift\Invalid.cpp' `
            "#include `"../../../Systems/Alpha/Alpha.h`"`r`n"
    } 'Simulation Model boundary violation' `
        'Core\Simulation\Models\Lift\Invalid.cpp'

    Assert-Rejected {
        Write-Fixture 'Common\Invalid.cpp' `
            "#include `"../Core/Systems/Alpha/Alpha.h`"`r`n"
    } 'Common boundary violation' 'Common\Invalid.cpp'

    Assert-Rejected {
        Write-Fixture 'Core\Extra.h' "#pragma once`r`n"
    } 'Core root source is not an entry facade' 'Core\Extra.h'

    Assert-Rejected {
        Write-Fixture 'Core\Extra.hpp' "#pragma once`r`n"
    } 'Core root source is not an entry facade' 'Core\Extra.hpp'

    Assert-Rejected {
        Write-Fixture 'Core\Systems\Alpha\Invalid.hpp' `
            "#include `"../Bravo/Bravo.h`"`r`n"
    } 'Cross-System dependency' 'Core\Systems\Alpha\Invalid.hpp'

    Assert-Rejected {
        Write-Fixture 'Data\OldConfig.h' "#pragma once`r`n"
    } 'Legacy production path' 'Data\OldConfig.h'

    Assert-RejectedReplacement 'Core\Fck1cEfm.cpp' `
        "#include `"Systems/Alpha/Alpha.h`"`r`n" `
        'Core facade boundary violation'

    Assert-RejectedReplacement 'Core\Fck1cEfm.cpp' `
        "#include `"Simulation/Models/Lift/Lift.h`"`r`n" `
        'Core facade boundary violation'

    Write-Output 'EFM architecture checker fixtures passed.'
}
finally {
    $systemTemp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    if (-not $temporaryRoot.StartsWith(
        $systemTemp,
        [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Refusing to remove a fixture outside the system temp directory.'
    }
    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}
