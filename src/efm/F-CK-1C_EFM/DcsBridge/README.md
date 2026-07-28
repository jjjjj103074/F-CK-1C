# DCSBridge contributor guide

DCSBridge is the boundary between the DCS EFM C ABI and the DCS-neutral
simulation in `Core`. It validates and translates DCS values, coordinates the
runtime tools, and converts typed Core results back to the ABI. Physics and
aircraft-system calculations do not belong here.

To trace the main path, read these files in order:

1. [`EfmExports.cpp`](EfmExports.cpp) — all exported callbacks and the
   production composition root.
2. [`../Core/Contracts/FrameContracts.h`](../Core/Contracts/FrameContracts.h) —
   typed frame input and output shared across the boundary.
3. [`../Core/Fck1cEfm.h`](../Core/Fck1cEfm.h) — the small Core command and
   immediate-operation interface.

## Data flow

```text
DCS setter callbacks
  -> EfmExports validation and translation
  -> Internal/FrameInputCollector (latest complete value per input category)
  -> ed_fm_simulate snapshots FrameInput
  -> Core::Fck1cEfm::step
  -> Core::FrameOutput
  -> EfmExports publishes the same completed output to
       -> Internal/OutputStore
            -> DCS force, moment, draw-arg, param, and event callbacks
       -> Internal/StateCsvWriter
```

Fuel getters and flight-preparation setters use
`BridgeContext::perform_core_preparation`.
Each completed `FrameOutput` mass effect is queued by `OutputStore`.
`ed_fm_change_mass` drains that queue in publication order, while ordinary
outputs retain latest-value semantics. Other callbacks publish input and wait
for the next simulation step unless the DCS ABI requires an immediate return.

`EfmExports.cpp` owns boundary orchestration: ABI defaults, validation,
translation, locking, tool coordination, and return values. Every exported
callback uses the shared ABI exception boundary, which logs unexpected C++
exceptions and returns that callback's neutral result. Structured Core
execution errors retain their System or Simulation Model owner, operation, and
reason in the same EventLog. It must not gain flight calculations, log message
formatting, or CSV row formatting.

## Layout and ownership

- `EfmExports.cpp` is the only DCSBridge entry point compiled into the DLL.
- `Internal/` contains all C++ implementation details: callback adapters,
  command and damage mapping, bridges, latest-value stores, logging, CSV, and
  lifecycle coordination.
- `ProcessBridgeContext` constructs one process-lifetime production context
  without running logger or worker-thread destruction under the Windows loader
  lock. `BridgeContext` owns Core, the collector and output store,
  cockpit/carrier bridges, EventLog, StateCsvWriter, and the execution mutex.
- `Core::Fck1cEfm` is the stable façade and owns the current per-flight
  `AircraftSimulation`. DCSBridge reads only `FrameOutput`; it must not read
  Core implementation state.
- `FrameInputCollector` owns the latest typed inputs. `OutputStore` owns the
  latest completed frame plus the lossless per-flight mass-effect queue.
  `StateCsvWriter` owns its worker thread and latest-record mailbox.
- Suspension sample values retain latest-value semantics, while their
  availability flags are consumed by each frame snapshot so Core can select
  DCS feedback or fallback per wheel.
- `../DcsIds/` contains DCS boundary identifiers and generated ID tables. It is
  boundary-only, is not a supported external include, and is not a Core
  dependency.

There is no DCSBridge umbrella or stable public C++ header. Files under
`Internal/` are private and may be included only by DCSBridge implementation or
native tests. Other production modules use `Core/Contracts/` and the explicit
Core interface instead. DCS itself sees only the exported C ABI.

## Adding a command

1. Reuse or add the DCS ID in `../DcsIds/Commands.h`. For a generated custom
   command, update `../DcsIds/CommandIds.json` and run
   `.\tools\generate_dcs_ids.ps1` from the repository root.
2. Add one binding in `Internal/DcsCommandRouter.cpp` with its explicit value
   rule: `PassThrough`, `Constant`, or `PressOnly`.
3. If the semantic command is new, extend `CommandId` in
   `../Core/Contracts/Commands.h`, then register its handler from the owning
   System's `setup()`.
4. Test both the DCS-to-command mapping and the observable `FrameOutput` result.

Do not add callback-specific methods to `BridgeContext`, and do not store
pressed state for `PressOnly` commands.

## Adding a FrameOutput field

1. Add the typed field to the appropriate output structure in
   `../Core/Contracts/FrameContracts.h`, using the value's existing unit.
2. Populate it in
   `../Core/Simulation/AircraftSimulationFrameOutput.cpp` from Core-owned state.
3. Consume it only where required: an ABI callback, an adapter in `Internal/`,
   or CSV output.
4. Add input/output tests. Do not expose a full Core snapshot to make testing
   easier.

## Adding a CSV field

CSV mirrors `FrameOutput` explicitly. In `Internal/StateCsvWriter.cpp`, update
the header and the matching row formatter in the same order, then update the
CSV tests. Scalars retain their existing units, vectors expand as fixed
`x,y,z` columns, booleans use `True`/`False`, and unavailable data uses `-`.
CSV publication must remain non-blocking for `ed_fm_simulate`.

## Project references and verification

Production sources belong in `F-CK-1C_EFM.vcxproj`. A source needed by native
tests must also be referenced by
`../../F-CK-1C_EFM_Tests/F-CK-1C_EFM_Tests.vcxproj`.
After changing the boundary:

1. Build and run the native tests.
2. Run `tools/build_dll.ps1` from the repository root.
3. Run `tools/check_efm_exports.ps1` and confirm the exported DCS ABI names
   match `EfmExports.baseline.txt`, unless the task explicitly changes the ABI.
