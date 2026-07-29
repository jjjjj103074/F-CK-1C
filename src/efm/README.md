# F-CK-1C EFM

This directory contains the C++ external flight model source for the F-CK-1C
DCS module.

The current contributor contracts are the
[`DcsBridge` guide](F-CK-1C_EFM/DcsBridge/README.md),
[`Core` guide](F-CK-1C_EFM/Core/README.md),
[`System` guide](F-CK-1C_EFM/Core/Systems/README.md), and
[`DcsIds` guide](F-CK-1C_EFM/DcsIds/README.md). These guides describe only the
current structure, responsibilities, and extension rules.

## Layout

- `F-CK-1C_EFM.sln` - Visual Studio solution.
- `F-CK-1C_EFM/F-CK-1C_EFM.vcxproj` - C++ dynamic library project.
- `F-CK-1C_EFM/DcsBridge/EfmExports.cpp` - DCS callback composition root.
- `F-CK-1C_EFM/DcsBridge/README.md` - boundary data flow and contributor guide.
- `F-CK-1C_EFM/DcsIds/` - DCS-owned identifiers, generated custom command
  tables, and cockpit parameter names.
- `F-CK-1C_EFM/Core/` - stable façade, shared contracts, and DCS-neutral
  per-flight simulation.
- `F-CK-1C_EFM/Core/Diagnostics/` - structured execution errors shared by Core
  pipelines and the DCS ABI boundary; it does not write logs.
- `F-CK-1C_EFM/Core/Systems/` - aircraft-owned systems and their pipeline
  contracts.
- `F-CK-1C_EFM/Core/Simulation/Models/` - DCS-neutral physical effect models
  for aerodynamics, propulsion, ground interaction, and mass properties.
- `F-CK-1C_EFM/DcsBridge/Internal/` - private DCS adapters, lifecycle, log, and CSV implementation.
- `F-CK-1C_EFM/include/` - DCS cockpit and flight model API headers.
- `F-CK-1C_EFM_Tests/` - native input/output, lifecycle, boundary, and
  regression tests that do not require DCS.

Every concrete System and Simulation Model owns its configuration, production
values, lookup tables, and validation inside its own directory. There is no
global `Data` configuration bag or legacy top-level `Systems` implementation.

## Build

The projects compile as C++17 with the `v142` platform toolset. Use a shell
where `MSBuild.exe` is available, then run from the repository root:

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
from `Core/Systems/*/Entry.cpp` into the intermediate directory. No generated
catalog is stored in the source tree.
Simulation models are deliberately not registered or scanned: the
`SimulationPipeline` executes each one exactly once in the fixed order
Aerodynamics, Propulsion, GroundInteraction, then MassProperties.

## Runtime diagnostics

DCSBridge creates these share-readable files under the installed module's
`log` directory:

- `fck1c_efm.log` - program events, warnings, and errors.
- `fck1c_state.csv` - the latest completed Core `FrameOutput` values for
  debugging calculations.

See the
[`DcsBridge` guide](F-CK-1C_EFM/DcsBridge/README.md) for file rotation,
warning aggregation, CSV publication, and failure behavior.

## Verification

Use the [DLL build guide](../../docs/BUILD_DLL.md) as the single source for
native tests, DLL build, architecture checks, System catalog fixtures, export
verification, and pre-commit hook behavior. Changes that depend on DCS
callback order, cockpit parameters, flight lifecycle, or runtime loading also
require an in-game test.
