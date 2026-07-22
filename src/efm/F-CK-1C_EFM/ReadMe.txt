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
- Core/: DCS-neutral simulation owner and pipeline.
- Systems/: active aircraft systems, including FBWController.
- Data/: immutable aircraft configuration and aerodynamic/engine tables.
- DcsBridge/Internal/: private DCS adapters, lifecycle, log, and CSV implementation.

Build from the repository root with:

    .\tools\build_dll.ps1

The expected runtime output is:

    bin\F-CK-1C_EFM.dll

Only the Release|x64 build target is supported.
