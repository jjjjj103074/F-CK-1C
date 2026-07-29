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

Fuel getters use `BridgeContext::query_core_preparation`; flight-preparation
setters use `BridgeContext::perform_core_preparation`.
Each completed `FrameOutput` mass effect is queued by `OutputStore`.
`ed_fm_change_mass` drains that queue in publication order, while ordinary
outputs retain latest-value semantics. Other callbacks publish input and wait
for the next simulation step unless the DCS ABI requires an immediate return.

`EfmExports.cpp` owns boundary orchestration: ABI defaults, validation,
translation, locking, tool coordination, and return values. Every exported
callback initializes any output parameters before work and uses the shared ABI
exception boundary. Unexpected C++ exceptions are written to EventLog when it
is available, otherwise to debugger output, and the callback returns its
neutral result; no exception may cross the DCS C ABI. Structured Core
execution errors retain their System or Simulation Model owner, operation, and
reason in the same EventLog. It must not gain flight calculations, log message
formatting, or CSV row formatting.

## Lifecycle and concurrency

The production bridge has two separate lifetimes:

- `ProcessBridgeContext` owns process-level tools and the single
  `BridgeContext`. It is initialized on the first DCS callback that needs it.
  Its process-lifetime allocation deliberately avoids destructor work under
  the Windows DLL loader lock.
- A flight begins with `ed_fm_cold_start`, `ed_fm_hot_start`, or
  `ed_fm_hot_start_in_air`. Start creates the Core's initial `FrameOutput`,
  publishes CSV sequence `0`, and makes a complete initial result available
  before the first simulation step.
- If another start arrives while a flight is active, DCSBridge writes
  `lifecycle_warning=repeated_start_without_release` at Warning level. Core
  preserves the current fuel preparation and directly replaces the active
  simulation; DCSBridge does not synthesize an `ed_fm_release`.
- `ed_fm_release` releases only the active flight. It clears collected input,
  completed output, and queued mass effects, then emits counted-warning
  summaries. A later start reuses the process tools for a new flight.
- Callbacks that require an active flight report a lifecycle or unavailable
  output error after release. Preparation setters and observation collectors
  are intentionally allowed before the next start: preparation goes directly
  to Core's single `FlightPreparation`, while normalized observations remain
  in `FrameInputCollector` for the first `step()`.

Core execution is serialized by `BridgeContext::execution_mutex()`.
`FrameInputCollector`, `OutputStore`, Param availability history, EventLog,
and CSV publication also protect their own state, so DCS callback threading
does not expose a partly updated `FrameInput` or `FrameOutput`.

Input categories normally use latest-value semantics. A frame snapshot copies
one complete input set; suspension values are retained, but each wheel's
availability is consumed per step so Core can distinguish fresh feedback from
fallback. Completed ordinary outputs also use latest-value semantics. Mass
deltas are the exception: they remain ordered and lossless until DCS consumes
them.

## Runtime diagnostics

Both files are created under `<module-root>/log`, opened with sharing enabled,
and may be read by another application while DCS is running:

- `fck1c_efm.log` contains code events only. Each line includes wall-clock
  time, simulation time, severity, and a traceable message. It is flushed
  immediately.
- `fck1c_state.csv` contains CSV sequence, simulation time, and completed
  `FrameOutput` values. Scalars keep their contract units, vectors expand to
  `x,y,z`, booleans are `True`/`False`, and unavailable data is `-`.

At the next DLL execution, the active file becomes `.old` and the previous
`.old` is removed. Files are not reopened between flights; CSV sequence resets
to `0` for each new flight.

CSV uses a worker thread and a single latest-record mailbox. Publishing never
waits for disk I/O: if the writer falls behind, the pending older row is
replaced by the newest row. Sequence gaps expose this condition without
building an unbounded queue. The writer flushes dirty data at most
approximately 100 ms after publication. `StateCsvWriter` joins its worker when
explicitly destroyed; the production process context intentionally avoids DLL
detach-time teardown and lets process termination reclaim process resources.

Unknown Command and Param IDs write one counted warning per ID on first use.
Repeated uses are counted, and `ed_fm_release` writes the total. Declared
commands that do not belong to the EFM are ignored without warnings. Invalid
runtime data, invalid callback values, structured Core failures, and lifecycle
violations other than the repeated-start warning above are errors rather than
telemetry.

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
  `AircraftSimulation`. DCSBridge reads per-frame simulation results only from
  a completed `FrameOutput`; it must not inspect System or Simulation state
  during a frame. Fuel preparation getters are separate façade queries through
  `query_core_preparation`, not simulation-result reads.
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

The build-time architecture checker enforces this dependency boundary.
DCSBridge may include only `Core/Fck1cEfm.h`, `Core/Contracts/`, and the
specific structured error boundary in `Core/Diagnostics/ExecutionError.h`;
it must not reach into Core Systems, pipelines, or Simulation implementation.

## Adding or changing a DCS callback

1. Keep the DCS-defined signature and record any intentional ABI change in
   `EfmExports.baseline.txt`.
2. Initialize every output parameter to its neutral value before work.
3. Validate and translate DCS values in `EfmExports.cpp` or a focused adapter
   under `Internal/`.
4. Use the shared ABI catch macro. Do not add a second exception policy.
5. Test neutral output, invalid input, lifecycle behavior, and the observable
   Core result.

Do not call a concrete Core System or Simulation Model from a callback.

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

Commands have three explicit outcomes:

- Supported EFM commands map to one semantic Core command.
- Known non-EFM commands are declared in `DcsIds/CommandIds.json` and ignored.
- Undeclared commands remain unknown and generate a counted warning.

## Adding a Param export

1. Define or reuse its semantic ID in `../DcsIds/ParamIds.h`. Generated cockpit
   parameter names belong in `../DcsIds/CommandIds.json`.
2. Add the mapping to the focused lookup in `Internal/ParamExport.h`.
3. Read only the published `FrameOutput`; do not query Core implementation
   state or recalculate a completed simulation result.
4. If DCS queries the value before its runtime input has ever been available,
   define an explicit start-compatibility value and comment why it is safe.
5. Test the value, missing-data behavior, and unknown-ID warning.

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

Non-Core production sources belong in `F-CK-1C_EFM.vcxproj`. A DCSBridge
source needed by native tests must also be referenced by
`../../F-CK-1C_EFM_Tests/F-CK-1C_EFM_Tests.vcxproj`.
Core sources and System Entries instead come from the shared
`EfmCore.props`/`EfmCore.targets` rules.

Follow the
[`DLL build guide`](../../../../docs/BUILD_DLL.md) for the canonical native
tests, architecture checks, DLL build, and export verification. A DCSBridge
change must also test its boundary validation and neutral return behavior.
Use DCS when the result depends on callback order, cockpit Param access, flight
start/release, or the runtime loader.
