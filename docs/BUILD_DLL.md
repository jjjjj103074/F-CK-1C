# F-CK-1C EFM DLL Build

This project builds one runtime DLL:

```text
bin\F-CK-1C_EFM.dll
```

Only `Release | x64` is supported for runtime use.

## Recommended Command

Run from the repository root:

```powershell
.\tools\build_dll.ps1
```

The script:

- resolves `MSBuild.exe` from PATH or common Visual Studio install locations
- rebuilds `src\efm\F-CK-1C_EFM.sln`
- copies the output DLL into `bin\F-CK-1C_EFM.dll`
- prints SHA256 hashes for the source and runtime DLL
- exits with an error if MSBuild fails, the DLL is missing, or the copy fails

## Requirements

- Visual Studio Build Tools or Visual Studio with MSBuild
- MSVC C++ toolchain and Windows SDK
- C++ toolset compatible with `PlatformToolset=v142`

Visual Studio 2019 Build Tools works directly. Newer Visual Studio versions can also work if they include the v142 toolset.

## Manual Debug Command

Use this only when debugging the Visual Studio project directly:

```powershell
Set-Location .\src\efm
MSBuild.exe .\F-CK-1C_EFM.sln /t:Rebuild /p:Configuration=Release /p:Platform=x64
Copy-Item ".\x64\Release\F-CK-1C_EFM.dll" "..\..\bin\F-CK-1C_EFM.dll" -Force
```

Do not use Debug, x86, or Win32 builds for the runtime DLL.
