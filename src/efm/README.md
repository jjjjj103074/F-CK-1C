# F-CK-1C EFM

C++17 external flight model for the DCS module.
The supported build is Release/x64 using the toolchain in
[BUILD_DLL.md](../../docs/BUILD_DLL.md).

## Structure

| Path | Responsibility |
|---|---|
| `F-CK-1C_EFM/DcsBridge/` | DCS C ABI, adapters, lifecycle and runtime tools |
| `F-CK-1C_EFM/Core/` | DCS-neutral per-flight simulation and typed contracts |
| `F-CK-1C_EFM/Core/Systems/` | Aircraft equipment and control state |
| `F-CK-1C_EFM/Core/Simulation/Models/` | Aerodynamics, propulsion, ground interaction and mass |
| `F-CK-1C_EFM/DcsIds/` | DCS IDs and generated C++/Lua tables |
| `F-CK-1C_EFM/Common/` | Shared units, configuration loader and utilities |
| `F-CK-1C_EFM_Tests/` | Native tests independent of DCS |
| `F-CK-1C_EFM/include/`, `ThirdParty/` | SDK and vendor-owned sources |

[DcsBridge](F-CK-1C_EFM/DcsBridge/README.md),
[Core](F-CK-1C_EFM/Core/README.md),
[Systems](F-CK-1C_EFM/Core/Systems/README.md) and
[DcsIds](F-CK-1C_EFM/DcsIds/README.md) define extension contracts.
Units are defined in [EFM_UNIT_CONVENTIONS.md](../../docs/EFM_UNIT_CONVENTIONS.md).
Cockpit ownership is described in [COCKPIT.md](../../docs/COCKPIT.md).

DFLCC currently implements configurable three-axis direct mapping.
Feedback, trim application, autopilot and automatic throttle are unimplemented;
see the [DFLCC guide](F-CK-1C_EFM/Core/Systems/DigitalFlightControlComputer/README.md).

## Build and verification

```powershell
npm run build:efm
npm run validate:efm
```

Run from the repository root. Build updates `bin/F-CK-1C_EFM.dll`;
validation preserves that DLL and compares it with a clean rebuild.
To verify an existing commit, run validation directly.

Shared EfmCore rules compile the same Core sources in the DLL and test projects,
check dependencies and generate the System catalog from
`Core/Systems/*/Entry.cpp`. Physical models remain in a fixed execution order.

Native production-entry integration coverage is tracked in
[issue #30](https://github.com/jjjjj103074/F-CK-1C/issues/30);
Core/DCS dependency-check coverage is tracked in
[issue #31](https://github.com/jjjjj103074/F-CK-1C/issues/31).
DCS-dependent loading, callbacks and flight lifecycle require
[in-game verification](../../docs/DCS_TESTING.md).

## Diagnostics

The installed module's `log/` contains `fck1c_efm.log`,
`fck1c_state.csv` and `debug.csv` when channels are declared.
[Runtime diagnostics](F-CK-1C_EFM/DcsBridge/README.md#runtime-diagnostics)
defines their timing, rotation, failure and display behavior.
