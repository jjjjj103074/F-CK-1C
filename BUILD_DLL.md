# F-CK-1C DLL Build Commands

This file records the commands used to build `BasicEFM_template.dll` for this mod.

## Project paths

- Solution: `DCS-Basic-EFM-Template-main\BasicEFM.sln`
- Project: `DCS-Basic-EFM-Template-main\Basic_EFM_Template\ED_FM_Template.vcxproj`
- Recommended target: `Release | x64`

## Requirement

Use a shell where `MSBuild.exe` is available.

Recommended:

- Visual Studio 2019 Developer Command Prompt

The project file uses `PlatformToolset=v142`, which usually maps to Visual Studio 2019.

## Build DLL

```powershell
cd "C:\Users\Ragdoll\Saved Games\DCS\Mods\aircraft\F-CK-1C\DCS-Basic-EFM-Template-main"
MSBuild.exe .\BasicEFM.sln /t:Build /p:Configuration=Release /p:Platform=x64
```

## Output DLL

After a successful build, the DLL should be here:

```text
C:\Users\Ragdoll\Saved Games\DCS\Mods\aircraft\F-CK-1C\DCS-Basic-EFM-Template-main\x64\Release\BasicEFM_template.dll
```

## Copy DLL into mod bin

```powershell
Copy-Item `
  "C:\Users\Ragdoll\Saved Games\DCS\Mods\aircraft\F-CK-1C\DCS-Basic-EFM-Template-main\x64\Release\BasicEFM_template.dll" `
  "C:\Users\Ragdoll\Saved Games\DCS\Mods\aircraft\F-CK-1C\bin\BasicEFM_template.dll" `
  -Force
```

## Build and copy in one go

```powershell
cd "C:\Users\Ragdoll\Saved Games\DCS\Mods\aircraft\F-CK-1C\DCS-Basic-EFM-Template-main"
MSBuild.exe .\BasicEFM.sln /t:Build /p:Configuration=Release /p:Platform=x64
Copy-Item ".\x64\Release\BasicEFM_template.dll" "..\bin\BasicEFM_template.dll" -Force
```

## Clean build

```powershell
cd "C:\Users\Ragdoll\Saved Games\DCS\Mods\aircraft\F-CK-1C\DCS-Basic-EFM-Template-main"
MSBuild.exe .\BasicEFM.sln /t:Clean,Build /p:Configuration=Release /p:Platform=x64
Copy-Item ".\x64\Release\BasicEFM_template.dll" "..\bin\BasicEFM_template.dll" -Force
```

## Notes

- `entry.lua` already references `BasicEFM_template.dll`.
- The current mod already has a DLL at `bin\BasicEFM_template.dll`.
- If `MSBuild.exe` is not found, open the Visual Studio Developer Command Prompt first and run the same commands there.
