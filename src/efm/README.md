# F-CK-1C EFM

This directory contains the C++ external flight model source for the F-CK-1C
DCS module.

## Layout

- `F-CK-1C_EFM.sln` - Visual Studio solution.
- `F-CK-1C_EFM/F-CK-1C_EFM.vcxproj` - C++ dynamic library project.
- `F-CK-1C_EFM/F-CK-1C_EFM.cpp` - main EFM implementation.
- `F-CK-1C_EFM/Core/` - DCS-neutral simulation owner and pipeline.
- `F-CK-1C_EFM/Systems/` - flight systems, including the active FBW controller.
- `F-CK-1C_EFM/FM_data.*` - flight model data helpers.
- `F-CK-1C_EFM/include/` - DCS cockpit and flight model API headers.

## Build

Use a shell where `MSBuild.exe` is available, then run from the repository root:

```powershell
.\tools\build_dll.ps1
```

The script builds `src\efm\F-CK-1C_EFM.sln` and copies the resulting
`F-CK-1C_EFM.dll` into the runtime `bin` folder.

Only `Release|x64` is supported for this DLL.

The Lua runtime references this DLL from `entry.lua`.
