# EFM Core

`Core` owns one DCS-neutral F-CK-1C flight simulation. Its public entry is
`Fck1cEfm`; DCS-specific IDs, callbacks, coordinates, logging, and cockpit
adapters remain in `DcsBridge`.

## Responsibilities

- `Fck1cEfm.h/.cpp` is the only source-level facade in the Core root.
- `Contracts/` defines semantic commands, events, AircraftData, frame
  input/output, and structured execution errors shared across Core modules.
- `Systems/` owns aircraft equipment state and behavior. `SystemPipeline`
  validates declarations, routes commands/events, schedules groups, and
  publishes one completed AircraftData snapshot.
- `Simulation/` retains DCS-owned observations, schedules physical models,
  aggregates their effects, and projects one completed `FrameOutput`.
- `Simulation/Models/<Owner>/` owns each physical model, its configuration,
  production values, tables, and validation.

Each concrete System likewise owns its configuration, production values, and
validation under `Systems/<Owner>/`. There is no global aircraft configuration
bag. Geometry belongs in `Simulation/Definition/` only when more than one model
actually shares it; no such definition exists yet.

## Dependency boundaries

Allowed dependencies point inward through contracts and pipelines:

```text
DcsBridge -> Fck1cEfm -> AircraftSimulation
AircraftSimulation -> SystemPipeline + SimulationPipeline
Concrete System -> System + Core Contracts + Common
Simulation Model -> Core Contracts + completed AircraftData + Common
```

Concrete Systems do not include one another, `DcsBridge`, or anything under
`Simulation/`. Concrete Simulation Models do not include one another or
anything under `Systems/`; `SimulationPipeline` translates their results into
the next Model's plain input values. `DcsBridge` reaches Core only through
`Fck1cEfm.h` and `Core/Contracts/`; it does not include Core pipeline or
Simulation implementation headers.

Systems and Simulation Models do not write logs. Their pipelines attach the
registered owner and operation to unexpected exceptions as an
`ExecutionError`; DCSBridge remains the single owner that writes those errors
to the runtime EventLog.

The build runs `tools/check_efm_architecture.ps1` before compiling. Run the
checker and its fixtures directly from the repository root with:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
    -File .\tools\check_efm_architecture.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass `
    -File .\tools\test_efm_architecture_checker.ps1
```

See `Systems/README.md` for the System extension contract.
