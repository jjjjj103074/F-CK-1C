# F-CK-1C EFM

This directory contains the C++ external flight model source for the F-CK-1C
DCS module.

The target Core responsibilities and dependency rules are recorded in
[`../../docs/EFM_CORE_ARCHITECTURE_PLAN.md`](../../docs/EFM_CORE_ARCHITECTURE_PLAN.md).
The staged refactor and verification gates are recorded in
[`../../docs/EFM_CORE_REFACTOR_IMPLEMENTATION_PLAN.md`](../../docs/EFM_CORE_REFACTOR_IMPLEMENTATION_PLAN.md).

## Layout

- `F-CK-1C_EFM.sln` - Visual Studio solution.
- `F-CK-1C_EFM/F-CK-1C_EFM.vcxproj` - C++ dynamic library project.
- `F-CK-1C_EFM/DcsBridge/EfmExports.cpp` - DCS callback composition root.
- `F-CK-1C_EFM/DcsBridge/README.md` - boundary data flow and contributor guide.
- `F-CK-1C_EFM/Core/` - stable façade, shared contracts, and DCS-neutral
  per-flight simulation.
- `F-CK-1C_EFM/Core/Systems/` - aircraft-owned systems and their pipeline
  contracts.
- `F-CK-1C_EFM/Core/Simulation/Models/` - DCS-neutral physical effect models
  for aerodynamics, propulsion, ground interaction, and mass properties.
- `F-CK-1C_EFM/DcsBridge/Internal/` - private DCS adapters, lifecycle, log, and CSV implementation.
- `F-CK-1C_EFM/include/` - DCS cockpit and flight model API headers.

Every concrete System and Simulation Model owns its configuration, production
values, lookup tables, and validation inside its own directory. There is no
global `Data` configuration bag or legacy top-level `Systems` implementation.

## Build

Use a shell where `MSBuild.exe` is available, then run from the repository root:

```powershell
.\tools\build_dll.ps1
```

The script builds the `Release|x64` EFM DLL project and copies the resulting
`F-CK-1C_EFM.dll` into the runtime `bin` folder.

Only `Release|x64` is supported for this DLL.

The Lua runtime references this DLL from `entry.lua`.

Both the DLL and native test projects import `EfmCore.props` and
`EfmCore.targets`. These shared rules compile the same Core sources and
check architecture boundaries, then generate the build-time System catalog
from `Core/Systems/*/Entry.cpp`.
Simulation models are deliberately not registered or scanned: the
`SimulationPipeline` executes each one exactly once in the fixed order
Aerodynamics, Propulsion, GroundInteraction, then MassProperties.

To verify the catalog generator independently:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
    -File .\tools\test_system_catalog_generator.ps1
```

To verify the dependency checker and its rejection fixtures:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
    -File .\tools\check_efm_architecture.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass `
    -File .\tools\test_efm_architecture_checker.ps1
```
