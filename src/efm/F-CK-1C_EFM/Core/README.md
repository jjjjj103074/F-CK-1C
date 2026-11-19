# EFM Core

Core owns one DCS-neutral flight simulation. Its public entry is
[`Fck1cEfm`](Fck1cEfm.h); DCS callbacks, IDs and runtime tools belong to DcsBridge.

## Responsibilities

| Path | Responsibility |
|---|---|
| `Fck1cEfm.h/.cpp` | Flight lifecycle, preparation and simulation entry |
| `Contracts/` | Commands, observations, AircraftData, frame results and debug contracts |
| `Diagnostics/` | Structured execution errors with owner and operation |
| `Systems/` | Equipment state, command handlers and scheduled control behavior |
| `Simulation/` | Retained observations, physical model execution and FrameOutput |
| `Simulation/Models/<Owner>/` | Model implementation, configuration and validation |

Each System and physical model owns its configuration. Shared geometry belongs
in `Simulation/Definition/` only when multiple models require it; no such
definition currently exists. Typed JSONC loading is documented in
[Common/Configuration](../Common/Configuration/README.md).

## Dependencies

```text
DcsBridge -> Fck1cEfm -> AircraftSimulation
AircraftSimulation -> SystemPipeline + SimulationPipeline
Concrete System -> System contracts + Core Contracts + Common
Physical Model -> Core Contracts + completed AircraftData + Common
```

Concrete Systems do not include one another, Simulation or DcsBridge.
Physical models do not include other models or Systems.
DCS identifiers stay outside Core. DcsBridge reaches Core through
`Fck1cEfm.h`, `Contracts/` and `Diagnostics/ExecutionError.h`.

Pipelines attach the registered owner and operation to unexpected exceptions.
DcsBridge writes the resulting `ExecutionError` to EventLog; Systems and models
do not write logs.

## Flight and frame execution

`Fck1cEfm` persists across flights. Preparation values survive `release()`;
each `start()` creates a new `AircraftSimulation` and `release()` destroys it.
Systems have no separate release hook.

```text
FrameInput
  -> retain available observations
  -> SystemPipeline: execute due time buckets
  -> SimulationPipeline: Aerodynamics, Propulsion, GroundInteraction, MassProperties
  -> complete FrameOutput
```

Systems due at the same time read one snapshot and commit as a batch.
Later buckets see earlier committed output. Commands may latch intent between
frames; continuous state advances on the owner's scheduled tick.
Simulation models run once per host frame using the completed System snapshot.

Debug producers use an injected `DebugTelemetrySink`. Channels are declared
during setup and published with simulation timestamps. Debug data is
observational and does not participate in AircraftData commits or feed control
state. Core does not know cockpit parameters, display formatting, files or threads.

See the [System guide](Systems/README.md),
[unit contract](../../../../docs/EFM_UNIT_CONVENTIONS.md) and
[build guide](../../../../docs/BUILD_DLL.md).
