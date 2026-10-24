# F-CK-1C Ching-kuo (經國號)

[繁體中文](README.zh-TW.md)

A DCS World community mod of the AIDC F-CK-1C Ching-kuo.

## Status

The release supports AI aircraft in missions. The player cockpit and EFM are
under development; development builds are available on [dev](https://github.com/jjjjj103074/F-CK-1C/tree/dev).

The EFM currently provides configurable three-axis direct mapping. Feedback
control, trim, autopilot and automatic throttle are unimplemented. See the
[DFLCC guide](src/efm/F-CK-1C_EFM/Core/Systems/DigitalFlightControlComputer/README.md)
for the supported control path and limitations.

## Installation

Download a package from [Releases](https://github.com/jjjjj103074/F-CK-1C/releases/latest), then run
`install.bat` or copy the module folder to
`Saved Games/DCS/Mods/aircraft/F-CK-1C/`.

The installer deletes and replaces the existing module folder. Back up local
changes inside that folder before running it.

For a custom Saved Games location:

```bat
install.bat "D:\Saved Games\DCS"
```

## Development

- [Contributing](CONTRIBUTING.md): setup, workflow and submission rules.
- [EFM](src/efm/README.md): architecture and module guides.
- [Cockpit](docs/COCKPIT.md): Lua devices, input routing and parameters.
- [Build and verification](docs/BUILD_DLL.md): toolchain and local commands.
- [DCS testing](docs/DCS_TESTING.md): scenarios and checks.

Report reproducible problems or feature requests in [Issues](https://github.com/jjjjj103074/F-CK-1C/issues).

## Research

[Aviation reference library](docs/reference-library/README.md) defines the
English-language source, knowledge, grading and rights workflow. Use its source
records and topic files to discover material; the library guide describes the
available retrieval tools.

[Control-law models](research/control-law-diagrams/README.md) contain independent
F-16 Modelica research and verification tools.

## License

Code, tooling, documentation and generated runtime binaries use MIT.
Non-code assets use CC BY 4.0 unless otherwise specified.
See [LICENSE](LICENSE) and the retained [EFM notice](src/efm/LICENSE).
