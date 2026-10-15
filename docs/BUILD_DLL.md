# EFM DLL build

Windows builds use `Release | x64` and output `bin/F-CK-1C_EFM.dll`.

## Requirements

Install these components through Visual Studio Installer:

| Component | Version |
|---|---|
| Visual Studio / Build Tools | 2026 |
| MSVC v142 x64/x86 | 14.29.30133; cl 19.29.30159.0, link 14.29.30159.0 |
| Windows SDK | 10.0.26100.0 |
| Node.js | 22 or newer LTS |

The shared [build profile](../src/efm/EfmBuild.props) pins the compiler, linker,
frontend, backend and static CRT/UCRT libraries. Check the installation:

```powershell
npm ci
npm run check:efm-toolchain
```

The CLI selects a matching installation and sets its environment. Visual Studio
reads the same profile. Identical component labels can contain different serviced
libraries; the fingerprint check identifies those differences.

## Build

Run from the repository root:

```powershell
npm run build:efm
```

Equivalent: `./tools/build_dll.ps1`.
This rebuilds the DLL, copies it to `bin/` and verifies the copy's hash.
In Visual Studio, build
[F-CK-1C_EFM.vcxproj](../src/efm/F-CK-1C_EFM/F-CK-1C_EFM.vcxproj), then copy its DLL to `bin/`.

## Native tests

Run from the repository root with the tracked `FM/` configuration present:

```powershell
./tools/build_efm.ps1 -Target Tests
./src/efm/x64/Release/F-CK-1C_EFM_Tests.exe
```

Exit code 0 means all checks passed. DCS is used for separate in-game tests.

## Verification

| Command | Check |
|---|---|
| `npm run validate:efm` | Clean rebuild, whole-DLL hash, exports/loading, tool fixtures and native tests |
| `npm run check:efm-dll` | DLL load, required exports and unload in an x64 process |
| `./tools/check_efm_exports.ps1` | Complete export list against `EfmExports.baseline.txt` |
| `./tools/test_efm_architecture_checker.ps1` | Source dependency rules |
| `./tools/test_system_catalog_generator.ps1` | System discovery and generated catalog |
| `./tools/test_dcs_id_generator.ps1` | Generated DCS identifiers |

Validation uses a preserved copy of the submitted DLL. Reports are saved in
`tmp/pr-validation/`. An ABI change updates the source, export baseline and DCS integration together.
CI results and setup: [PR validation](PR_VALIDATION.md).

## Commit hook

The hook checks staged C++ formatting first. Runtime C++ sources, build settings,
build scripts or the DLL trigger a rebuild and automatic DLL staging.
The exact input list is in [.husky/pre-commit](../.husky/pre-commit).
Documentation and test-only edits skip the runtime build.

Builds read the working tree. Stage all related inputs before committing;
the hook reports unstaged, untracked or ignored build inputs.
Run native tests and the relevant verification commands above before opening a PR.
Formatting setup: [C++ formatting](CPP_FORMATTING.md).

## Build profile

The profile uses x64 host tools, deterministic timestamps, source-path mapping
to `D:\efm-repro`, and a PDB filename. Visual Studio may ask for the local source path
when debugging. Personal user.props and Directory.Build overrides are excluded.
Review profile changes and repeat cross-machine hash validation when updating components.

System discovery writes `SystemCatalog.g.cpp` to the intermediate build directory.
To register a System, add `Core/Systems/<Owner>/Entry.cpp`.
