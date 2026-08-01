# Flight-control refactor baseline

## Baseline identity

- Source commit: `75b2e4a` (`refactor(efm): establish flight control system boundaries`).
- Captured on: 2026-08-01.
- Canonical plan: [`FLIGHT_CONTROL_REFACTOR_PLAN.md`](FLIGHT_CONTROL_REFACTOR_PLAN.md).
- Native baseline: Release x64 build passed; `2353` checks, `0` failures.
- Architecture checker: passed with `165` source files.

This file records which behavior exists before the FCC/AP refactor. It is not
an assertion that the existing controller math or response is correct.

## Ownership and verification map

| Behavior | Current owner | Existing verification | Refactor policy |
|---|---|---|---|
| AP master engage/reject/disconnect | `AutomaticFlightControl` | engage-boundary, ground/attitude-gate, next-tick, disconnect tests | Preserve command availability; replace normalized output seam |
| Pitch/VS/ALT mode selection and reference adjustment | `AutomaticFlightControl` | vertical-mode, capture, controller-direction tests | Preserve modes; replace controller math and capture behavior where specified |
| Heading Hold/Select and 0/360 wrapping | `AutomaticFlightControl` | heading-controller and wrap tests | Preserve modes and wrap behavior; cap AP bank/rate from Reference |
| Paddle bypass | `AutomaticFlightControl` | bypass freeze/recapture and threshold tests | Preserve momentary all-axis bypass; remove motion threshold and use explicit release rules |
| Navigation Track | `AutomaticFlightControl` | placeholder test | Keep as explicit placeholder; do not invent guidance |
| Auto-throttle compatibility | `AutomaticFlightControl` | engage-gate, Mach-guard, target-limit, controller tests | Move unchanged to experimental assist ownership |
| CAT and developer G-limit override | FCC/FBW control laws | FBW command and limiter tests | Preserve availability and diagnostics; keep out of AP ownership |
| Manual alpha/Nz/q and p/q/r control laws | FCC/FBW control laws | reference-frame, hold, limiter and actuator-feedback tests | Preserve baseline before physical-reference migration |
| Actuator command publication | `FlightControlComputer` | FCC/System and actuator-system tests | Preserve one publisher and one production path |

## Current command set

`AutomaticFlightControl` owns AP master, bypass, Pitch Hold, Vertical-Speed
Hold, Altitude Hold, Heading Hold, Heading Select, Navigation Track, vertical
and lateral reference adjustment, plus the temporary A/T commands.

`FlightControlComputer` owns CAT I/III selection and the developer-only
G-limiter override commands. All observation-dependent effects occur on the
next scheduled 64 Hz FCC tick.

## Known defects that are not compatibility contracts

- ALT Hold has visible overshoot/oscillation and is not acceptably settled.
- Vertical and heading reference adjustments operate but do not feel well
  shaped.
- AP currently emits normalized pitch/roll stick-equivalent commands and the
  FCC overwrites pilot pitch/roll inputs.
- Paddle release ignores changes below an arbitrary one-degree attitude
  threshold.
- AP vertical and lateral controllers are tuned as direct normalized outputs,
  not as a coordinated physical-reference cascade.

Tests may preserve that commands and modes exist, that outputs remain finite,
and that signs are correct. They must not preserve the defective waveform,
arbitrary threshold, or normalized AP-to-FBW seam as desired behavior.

## Baseline verification command

```powershell
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' `
  '.\src\efm\F-CK-1C_EFM_Tests\F-CK-1C_EFM_Tests.vcxproj' `
  /t:Rebuild /p:Configuration=Release /p:Platform=x64 /m /v:m
& '.\src\efm\x64\Release\F-CK-1C_EFM_Tests.exe'
```

The documented toolchain remains the authoritative command source; the full
MSBuild path above records the executable used for this capture.
