# DcsBridge contributor guide

DcsBridge translates the DCS EFM C ABI into Core contracts and completed results
back into DCS outputs. It owns validation, lifecycle, logging and runtime tools;
aircraft-system behavior and physics belong to Core.

Entry points:

- [EfmExports.cpp](EfmExports.cpp): exported callbacks and production composition.
- [FrameContracts.h](../Core/Contracts/FrameContracts.h): typed frame input/output.
- [Fck1cEfm.h](../Core/Fck1cEfm.h): Core lifecycle, commands and preparation.

## Data flow

```text
DCS setters -> validation/adapters -> FrameInputCollector
ed_fm_simulate -> FrameInput snapshot -> Fck1cEfm::step -> FrameOutput
FrameOutput -> OutputStore -> force, moment, draw-arg, param and event callbacks
            -> StateCsvWriter

ed_fm_set_command -> DcsCommandRouter -> semantic Core CommandId -> System handler

Core debug producer -> DebugTelemetryHub -> latest Indicator values
                                        -> 64 Hz samples -> DebugCsvWriter
```

CockpitSnapshotExporter writes completed C++ snapshots to generated parameters.
Cockpit-only sensor observations enter separately through CockpitBridge.
HMCS Lua publishes availability plus magnetic heading in radians and barometric
altitude in metres; CockpitBridge converts these once to degrees and feet.
Missing observations remain unavailable. World yaw and geometric ASL altitude
are separate quantities.

AP/A/T control is unimplemented and its snapshots remain unavailable.
The [cockpit guide](../../../../docs/COCKPIT.md) identifies current Lua ownership.

## ABI and units

The [unit contract](../../../../docs/EFM_UNIT_CONVENTIONS.md) defines quantities
and signs. DcsKinematicsAdapter names body angular x/z/y as roll/pitch/yaw while
preserving signs: right roll, nose-up pitch and nose-left yaw are positive.
Force/moment outputs retain local x-forward/y-up/z-right axes and use N, N·m and m.
ParamExport converts atmosphere K to the Celsius engine-temperature parameter.

Every callback initializes output parameters before work, validates inputs and
uses the shared ABI exception boundary. Unexpected exceptions produce an EventLog
error, or debugger output when the log is unavailable, and return neutral values.
Core errors preserve owner, operation and reason. C++ exceptions never cross the ABI.

Fuel getters use `query_core_preparation`; preparation setters use
`perform_core_preparation`. Ordinary outputs retain the latest complete frame.
Mass effects are queued and drained in publication order by `ed_fm_change_mass`.

## Lifecycle and concurrency

ProcessBridgeContext lazily owns one process-lifetime BridgeContext.
Its allocation avoids logger/thread destruction under the Windows DLL loader lock.
BridgeContext owns Core, collectors, outputs, cockpit/carrier adapters and tools.

Cold, hot-ground and hot-air starts create a new Core flight and publish its
complete initial FrameOutput at CSV sequence 0. Repeated start writes
`lifecycle_warning=repeated_start_without_release`, preserves fuel preparation,
releases the previous simulation and clears its output before initialization.
Initialization failure leaves no active simulation or previous completed output.

Release clears active-flight input, outputs and mass effects, flushes debug CSV
and emits counted-warning summaries. Process tools remain for the next flight.
Preparation setters and observation collectors may run before a start; callbacks
requiring an active flight report unavailable output after release.

Core execution uses `BridgeContext::execution_mutex()`.
Collectors, output stores, availability history and tools protect their own state.
A FrameInput snapshot copies complete input categories. Suspension values are
retained, but per-wheel availability is consumed each step.
Ordinary outputs use latest-value semantics; mass effects remain ordered and lossless.

## Runtime diagnostics

Files under `<module-root>/log/` allow concurrent readers:

| File | Content |
|---|---|
| `fck1c_efm.log` | Wall time, simulation time, severity and event/error messages |
| `fck1c_state.csv` | Sequence, simulation time and completed physical FrameOutput |
| `debug.csv` | Sequence, simulation time and declared Core debug channels |

EventLog flushes each line. State CSV excludes control-law internals.
Debug CSV samples latest values at 64 Hz with zero-order hold; unpublished values
use `-`. Its header follows channel declaration order and doubles retain
round-trip precision.

EventLog and state CSV rotate at process-tool initialization; debug CSV rotates
on its first flight-start publication. Each active file becomes `.old`, replacing
the previous `.old`. Files stay open across flights; CSV sequence resets per flight.
Without channels, debug CSV rotates an existing file but creates no empty replacement.

CSV writers use separate worker threads and bounded latest-record/latest-batch
mailboxes. Publication does not wait for disk I/O; newer pending data may replace
older data, visible as sequence gaps. Dirty data flushes within approximately
100 ms; release waits for pending debug CSV data. Process termination reclaims the
process tools.

The EFM Debug Indicator starts hidden each flight. Bind
`[Developer] EFM Debug Indicator - Toggle` under Developer Tools.
Six text blocks display up to 60 channels in declaration order; extra channels
are available in debug.csv. Display doubles use six significant digits.
Debug CSV I/O failures and invalid debug publications appear in the status row
and EventLog. State CSV I/O failures are reported through EventLog.

Production isolates publication errors from aircraft control. The first diagnostic
preserves channel, attempted time and reason; a per-flight count records later
failures. Tests can select report-and-rethrow behavior.

Unknown command/parameter IDs write one counted warning per ID; release summarizes
repeated uses. Declared non-EFM commands are ignored. Invalid callback data, Core
execution failures and lifecycle violations are errors.

Legacy debug exports remain present: `ed_fm_enable_debug_info()` returns false;
`ed_fm_debug_watch()` clears the buffer and returns 0.
The dedicated Indicator supplies the display.

## Dependencies

`Internal/` contains private adapters, stores and tools.
Only DcsBridge implementation and native tests may include it.
DCS sees the exported C ABI, not a public DcsBridge C++ header.

DcsBridge may include `Core/Fck1cEfm.h`, `Core/Contracts/` and
`Core/Diagnostics/ExecutionError.h`. Core, Systems and physical models remain
independent of DCS IDs and bridge implementation.
The architecture checker covers the DcsBridge allowlist and selected System/model
rules; [issue #31](https://github.com/jjjjj103074/F-CK-1C/issues/31) tracks
additional Core/DCS enforcement.

## Extending the bridge

### Callback

Keep the DCS signature and update `EfmExports.baseline.txt` for intentional ABI
changes. Initialize outputs, validate/translate in the focused adapter, use the
shared catch boundary, and test neutral output, invalid input and lifecycle behavior.
Call the public Core interface rather than concrete Systems or physical models.

### Command

Declare custom IDs/routes in `DcsIds/CommandIds.json` and run the
[DcsIds generator](../DcsIds/README.md).
DCS built-in IDs used by Core bindings belong in `DcsIds/Commands.h`.
Add the DcsCommandRouter binding with PassThrough, Constant or PressOnly;
new semantic commands also need a Core CommandId and owning System handler.
Command metadata and routing alone do not implement behavior.

Known cockpit/DCS-owned commands are ignored when received by the EFM.
Undeclared IDs produce counted warnings.
Test input mapping and observable output; PressOnly bindings do not store pressed state.

### Output

Define typed fields in FrameContracts, populate them in
`AircraftSimulationFrameOutput.cpp` and consume completed FrameOutput values.
For Params, use ParamIds and ParamExport; define explicit safe initial values
when DCS requests data before its runtime source is available.
For state CSV, update header and row order together.
Scalars preserve units, vectors expand x/y/z, booleans use True/False and missing
values use `-`. Keep publication non-blocking.

### Debug channel

Store a `DebugTelemetryChannel<T>` in the producer, declare it during setup with
a stable name, label, unit and description, and publish at the owning operation's
simulation time. Types are double, int64, bool and UTF-8 string.
The first-flight schema locks names, types and descriptors for the process;
later flights must redeclare every channel with matching descriptors.
Publication timestamps are nonnegative and nondecreasing across all channels
in the flight. Use the owning operation's scheduled time consistently.

The current System producer is PropulsionDiagnostics'
`propulsion_test_thrust_cut_requested` channel.
Additional channels use the same injected sink; Core does not know files,
cockpit parameters or display formatting.
Test descriptors, timestamps, escaping and error behavior.

## Build and verification

Non-Core DLL sources belong in F-CK-1C_EFM.vcxproj.
Bridge sources needed by tests also belong in the native test project.
Shared EfmCore rules discover Core sources and System Entries.

Use the [build guide](../../../../docs/BUILD_DLL.md) for native checks and
[DCS testing](../../../../docs/DCS_TESTING.md) for loading, callback order,
cockpit parameter access and flight lifecycle.
