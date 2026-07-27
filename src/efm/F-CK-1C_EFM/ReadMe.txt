F-CK-1C_EFM C++ project
=======================

This project builds the F-CK-1C external flight model DLL used by the DCS
module runtime.

Important files:

- F-CK-1C_EFM.vcxproj: Visual Studio C++ dynamic library project.
- DcsBridge/EfmExports.cpp: DCS callback composition root.
- DcsBridge/README.md: boundary data flow and contributor guide.
- F-CK-1C_EFM.h: exported DCS EFM function declarations.
- F-CK-1C_EFM_API.h: DLL export/import macro.
- Core/: DCS-neutral facade, aircraft Systems, Simulation, and contracts.
- Core/Systems/: aircraft-owned state and behavior.
- Core/Simulation/Models/: physical effects and model-owned configuration.
- DcsBridge/Internal/: private DCS adapters, lifecycle, log, and CSV implementation.

The build checks Core dependency boundaries before compiling and generates the
System catalog from Core/Systems/*/Entry.cpp.

Build from the repository root with:

    .\tools\build_dll.ps1

The expected runtime output is:

    bin\F-CK-1C_EFM.dll

Only the Release|x64 build target is supported.
