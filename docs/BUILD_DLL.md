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

- locates MSBuild and the pinned C++ components through Visual Studio Installer/vswhere
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
- Pinned v142 tools 14.29.30133 (cl 19.29.30159.0, link 14.29.30159.0)
- Windows SDK 10.0.26100.0; compiler/linker/backend fingerprints must match
- C++17 language support

Visual Studio / Build Tools can be newer if they include the pinned components.
The script finds them with vswhere and enters their developer environment.
Run `npm run check:efm-toolchain` to check installation. The common profile in
`src/efm/EfmBuild.props` is read by both projects, Visual Studio and CLI builds.
It uses deterministic timestamps, source-path mapping and a fixed PDB filename;
personal user.props and Directory.Build overrides are excluded. See
[PR validation](PR_VALIDATION.md) for the complete recipe and adoption limits.

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
the repository root; the script sets up the pinned developer environment:

```powershell
.\tools\build_efm.ps1 -Target Tests
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

`npm run check:efm-dll` normally loads the uploaded DLL in an isolated x64
Windows process, resolves the expected exports and unloads it. It has a timeout
and makes no EFM/DCS callback. `npm run validate:efm` performs a clean DLL rebuild,
whole-file hash comparison, these DLL checks, fixtures and native tests without
replacing or staging bin/F-CK-1C_EFM.dll. See [PR validation](PR_VALIDATION.md).

## Commit Hook

Before checking runtime build inputs, the hook runs the read-only staged C++
formatting check. It covers authored EFM sources and tests, and reads source
from the Git index without modifying or staging C++ files. Format working-tree
files before staging. See [C++ formatting](CPP_FORMATTING.md) for setup and
scope. A formatting failure stops the commit before the DLL build.

The pre-commit hook rebuilds and stages `bin/F-CK-1C_EFM.dll` when the staged
change includes a whitelisted runtime EFM build input. The runtime whitelist
contains C/C++ source and header extensions (`.c`, `.cc`, `.cpp`, `.cxx`, `.h`,
`.hh`, `.hpp`, `.hxx`, `.inc`, `.inl`, `.ipp`, and `.tpp`). The exact-path
whitelist contains `F-CK-1C_EFM.vcxproj`, the shared `EfmBuild.props` / `EfmBuild.targets`,
`efm-build-common.ps1`, `check_efm_toolchain.ps1`, `EfmCore.props` and
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
