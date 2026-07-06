# F-CK-1C DLL Build Commands

This file records the commands used to build `F-CK-1C_EFM.dll` for this mod.

## Project paths

- Solution: `src\efm\F-CK-1C_EFM.sln`
- Project: `src\efm\F-CK-1C_EFM\F-CK-1C_EFM.vcxproj`
- Only supported target: `Release | x64`

## Requirement

Use a shell where `MSBuild.exe` is available.

Recommended:

- Visual Studio 2019 Developer Command Prompt

The project file uses `PlatformToolset=v142`, which usually maps to Visual Studio 2019.

## Build DLL

```powershell
Set-Location .\src\efm
MSBuild.exe .\F-CK-1C_EFM.sln /t:Build /p:Configuration=Release /p:Platform=x64
```

## Output DLL

After a successful build, the DLL should be here:

```text
src\efm\x64\Release\F-CK-1C_EFM.dll
```

## Copy DLL into mod bin

```powershell
Copy-Item `
  ".\src\efm\x64\Release\F-CK-1C_EFM.dll" `
  ".\bin\F-CK-1C_EFM.dll" `
  -Force
```

## Build and copy in one go

```powershell
Set-Location .\src\efm
MSBuild.exe .\F-CK-1C_EFM.sln /t:Build /p:Configuration=Release /p:Platform=x64
Copy-Item ".\x64\Release\F-CK-1C_EFM.dll" "..\..\bin\F-CK-1C_EFM.dll" -Force
```

## Clean build

```powershell
Set-Location .\src\efm
MSBuild.exe .\F-CK-1C_EFM.sln /t:Clean,Build /p:Configuration=Release /p:Platform=x64
Copy-Item ".\x64\Release\F-CK-1C_EFM.dll" "..\..\bin\F-CK-1C_EFM.dll" -Force
```

## Notes

- `entry.lua` already references `F-CK-1C_EFM.dll`.
- The current mod already has a DLL at `bin\F-CK-1C_EFM.dll`.
- Do not build Debug, x86, or Win32 variants for runtime use.
- If `MSBuild.exe` is not found, open the Visual Studio Developer Command Prompt first and run the same commands there.

```powershell
.\tools\build_dll.ps1
```
