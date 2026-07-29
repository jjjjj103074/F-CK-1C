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

The equivalent npm entry point is:

```powershell
npm run build:efm
```

The script:

- resolves `MSBuild.exe` from PATH or common Visual Studio install locations
- rebuilds only `src\efm\F-CK-1C_EFM\F-CK-1C_EFM.vcxproj`
- copies the output DLL into `bin\F-CK-1C_EFM.dll`
- verifies that the source and runtime DLL SHA256 hashes match
- exits with an error if MSBuild fails, the DLL is missing, the copy fails, or the hashes differ

The DLL build does not modify tracked C++ or Lua source files. Shared MSBuild
rules generate `SystemCatalog.g.cpp` only inside the project intermediate
directory; it is a disposable build artifact and must not be committed.

## Requirements

- Visual Studio Build Tools or Visual Studio with MSBuild
- MSVC C++ toolchain and Windows SDK
- C++ toolset compatible with `PlatformToolset=v142`
- C++17 language support

Visual Studio 2019 Build Tools works directly. Newer Visual Studio versions can also work if they include the v142 toolset.

## Manual Debug Command

Use this only when debugging the Visual Studio project directly:

```powershell
Set-Location .\src\efm
MSBuild.exe .\F-CK-1C_EFM\F-CK-1C_EFM.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform=x64
Copy-Item ".\x64\Release\F-CK-1C_EFM.dll" "..\..\bin\F-CK-1C_EFM.dll" -Force
```

Do not use Debug, x86, or Win32 builds for the runtime DLL.

## Native Logic Tests

The native test executable does not link the DCS runtime. Build and run it from
the repository root using a Visual Studio Developer PowerShell:

```powershell
MSBuild.exe .\src\efm\F-CK-1C_EFM_Tests\F-CK-1C_EFM_Tests.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform=x64
.\src\efm\x64\Release\F-CK-1C_EFM_Tests.exe
```

The process returns exit code `0` only when every check passes.

## Boundary Verification

The DLL build runs the source dependency checker through the shared MSBuild
rules. The checker and its rejection fixtures can also be run directly:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
    -File .\tools\check_efm_architecture.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass `
    -File .\tools\test_efm_architecture_checker.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass `
    -File .\tools\test_system_catalog_generator.ps1
```

The last command tests build-time System discovery independently. Adding
`Core/Systems/<Owner>/Entry.cpp` must not require edits to either `.vcxproj`,
`Fck1cEfm`, `AircraftSimulation`, or `SystemPipeline`.

After building, verify that the DLL still exports the recorded DCS C ABI:

```powershell
.\tools\check_efm_exports.ps1
```

This command compares `bin\F-CK-1C_EFM.dll` with
`src\efm\F-CK-1C_EFM\DcsBridge\EfmExports.baseline.txt`.
An intentional ABI change must update the code, baseline, and DCS integration
together.

## Commit Hook

The pre-commit hook rebuilds and stages `bin/F-CK-1C_EFM.dll` when the staged
change includes a whitelisted runtime EFM build input. The runtime whitelist
contains C/C++ source and header extensions (`.c`, `.cc`, `.cpp`, `.cxx`, `.h`,
`.hh`, `.hpp`, `.hxx`, `.inc`, `.inl`, `.ipp`, and `.tpp`). The exact-path
whitelist contains `F-CK-1C_EFM.vcxproj`, the shared `EfmCore.props` and
`EfmCore.targets` rules, and the build, architecture-check, and System-catalog
scripts used by that project. Added, copied, modified, renamed, and deleted
inputs are covered. Staging the runtime DLL itself also triggers a clean
rebuild.

Files outside those whitelists, including Markdown, text, JSON, images, CSV,
and changes limited to `F-CK-1C_EFM_Tests/`, do not rebuild the runtime DLL.
`DcsIds/CommandIds.json` follows the separate explicit generator workflow
documented in `DcsIds/README.md`; its generated C++ headers are runtime build
inputs and trigger the hook normally.

MSBuild reads the working tree rather than the Git index. To prevent the DLL
from containing code that is absent from the commit, the hook applies the same
whitelists to staged and dirty paths. A triggered build stops with an explicit
error when a whitelisted input is tracked but unstaged, ordinarily untracked,
or ignored. Ignored C/C++ inputs are checked because MSBuild wildcards and
C++ includes can still consume them. Stage, stash, move, or remove those inputs
before committing. Rename detection is disabled for this comparison so the
old and new paths are checked independently.

The hook does not run the native test executable, architecture fixtures,
catalog fixtures, or export verification; contributors must run the relevant
checks above.

All documentation changes must pass `git diff --check` and a local relative
Markdown-link check.
