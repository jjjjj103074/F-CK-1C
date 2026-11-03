# Native gun interoperability facts

The probe is implemented for this project. The private ABI facts (payload export,
gun-mount/count access layout and two host fingerprints) were researched from
[Ethanw05/DCS-F22 at fec598b](https://github.com/Ethanw05/DCS-F22/tree/fec598b7ad75714ed9aae7ee87be81eaf1c5f8a6).
Its NativeSidewinder.cpp documents the payload path and NativeSidewinderHost.h
records the version fingerprints. This probe owns its reader abstraction,
Windows exception handling, MC integration, tests and diagnostic packaging.

Host matching is a prerequisite for calling the native entry points. It is a
compatibility check, not independent proof of every native lifetime contract.
The package records runtime results for the user's installed DCS build.
