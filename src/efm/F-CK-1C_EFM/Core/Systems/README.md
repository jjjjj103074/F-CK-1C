# SystemPipeline contributor guide

`SystemPipeline` owns Systems, setup validation, command/event dispatch,
fixed-rate scheduling and completed `AircraftData` snapshots.
`AircraftSimulation` consumes those snapshots rather than concrete Systems.

## Systems

| System | Rate | Responsibility |
|---|---:|---|
| `PilotControls` | 64 Hz | Pilot axes, trim inputs and independent throttle-lever signals |
| `DigitalFlightControlComputer` | 64 Hz | Configurable three-axis direct mapping |
| `MissionComputer` | 64 Hz | Master-arm/mode management, HOTAS, stores fallback and ordered DCS operation requests |
| `FlightControlActuationSystem` | 256 Hz | Primary-surface dynamics and feedback |
| `SecondaryFlightControls` | 64 Hz | Flaps, slats and airbrake |
| `LandingGear` | 64 Hz | Gear, brakes, NWS, wheels and suspension |
| `Engine` | 64 Hz | Engine state, spool, nozzle and fuel demand |
| `Fuel` | 64 Hz | Storage, supply, transfer and consumed mass |
| `AirframeStructure` | 64 Hz | Component integrity and damage |
| `PropulsionDiagnostics` | 64 Hz | Thrust-cut test intent and telemetry |

The DFLCC's 64 Hz uses the F-16XL reference in
[NASA TP-3547](https://ntrs.nasa.gov/api/citations/20040040334/downloads/20040040334.pdf).
It is not confirmed F-CK-1C timing. Other 64 Hz rates and the 256 Hz actuation
rate are project-defined simulation choices.

## Lifecycle and scheduling

Every System implements:

- `setup()`: declare one fixed rate/period, AircraftData reads/publications and handlers.
- `step()`: advance one device tick using `SystemStepContext` time and `dt_s`.

`SystemStepContext.input_time` is the current host frame's input observation time.
It can be later than `scheduled_time` during catch-up. An acknowledgement newly
observed in that frame must not be treated as having arrived at a historical tick.
Existing Systems continue to use their scheduled time and fixed `dt_s`.

Setup first gathers declarations, then validates providers, types, required
initial values, single writers and handler ownership before committing state.
The first tick occurs after a complete period.

Systems due at the same time read one immutable snapshot and publish through a
Pipeline-owned `SystemResult` buffer. Their results commit together; subsequent
buckets see that completed state. Tick times derive from the common epoch and
invocation count, so a delayed host frame executes every due tick without
shifting device clocks. Use the System's `dt_s`, not the host frame's interval.

## Adding a System

Place `Entry.cpp` directly under `Core/Systems/<Owner>/`.
The directory name must be a C++ identifier. Expose `create_entry()` in the
matching `Core::Systems::Catalog::<Owner>` namespace:

```cpp
namespace Core::Systems::Catalog::MySystem
{
SystemEntry create_entry()
{
	SystemEntry entry;
	entry.id = "my_system";
	entry.factory = [](const FlightSetupContext& setup)
	{
		return std::make_unique<::Core::Systems::MySystem>(setup);
	};
	return entry;
}
}
```

MSBuild generates the catalog from these Entries in the intermediate directory.
Shared build rules discover System sources for both the DLL and native tests;
no manual catalog or project-file entry is needed. Adding a System should not
require edits to Fck1cEfm, AircraftSimulation, SystemPipeline or another System.

`FlightSetupContext` supplies StartMode, initial fuel, initial throttle-lever
signal and the debug sink. Configuration belongs to its owner; typed file
loading uses [Common/Configuration](../../Common/Configuration/README.md).
ControlLawData requires valid settings and aborts initialization on load failure.
Simulation policies such as infinite fuel, invincibility and easy flight belong
in Simulation.

## Data and handlers

Keys are defined in `Core/Contracts/AircraftData.h`.
Each key has one publisher; readers declare whether an initial value is required.
Undeclared reads, type mismatches, missing providers, duplicate publishers and
missing required initial values fail explicitly.

`FrameInput` contains callback-local values such as suspension samples.
`AircraftObservation` retains available flight-state observations between
callbacks. Systems use declared typed inputs rather than DCS callback details.
Field suffixes and conversion owners follow the
[unit contract](../../../../../docs/EFM_UNIT_CONVENTIONS.md).

Commands have one handler per semantic CommandId. Handlers may latch intent;
observation-dependent transitions occur on the next scheduled tick.
Handlers run synchronously. A System that defers edge actions must retain each
event until its tick. Damage areas have one owner;
repair can have multiple subscribers. Unregistered commands/damage return
`DispatchResult::Unhandled`. Handler `SystemActionContext` carries the current
simulation time for timestamped diagnostics.

Creation, setup, step and handler failures carry the catalog ID and operation in
`ExecutionError`. DcsBridge records the error.

## Control and fuel paths

`PilotControls` publishes `PilotControlSignal` and `ThrottleLeverSignal`.
DFLCC maps the three pilot axes to normalized surface-position demands.
Actuation converts demands to physical radians. Engine reads the throttle-lever
signal directly, independently of DFLCC power.

DFLCC is one System with seven internal components and five typed databases.
Its Executive owns internal task order and timing. Feedback control, trim
application, modes, monitoring and automatic throttle remain unimplemented.
Both flight-control status snapshots remain unavailable.
See the [DFLCC guide](DigitalFlightControlComputer/README.md).

MissionComputer follows the DFLCC's assembly pattern with four internal
components and three typed databases. Its 64 Hz is the project fallback;
monitoring, failure handling and mission functions remain unimplemented.
It declares no AircraftData reads, publications or handlers yet.
See the [MC guide](MissionComputer/README.md).

Fuel owns preparation handlers and the sole FuelData publication.
Its consumed mass accumulates across all Fuel ticks in one host frame.
Simulation turns that value into a mass effect. Infinite-fuel policy suppresses
consumption for the callback while preserving the reported flow.

Debug channels are declared through `SystemSetup::declare_debug_channel()`
and published with the owning operation's simulation timestamp.
They do not alter AircraftData or control state.

## Verification

Use the [build guide](../../../../../docs/BUILD_DLL.md).
Cover setup declarations, handlers, scheduled timing, retained data and
observable outputs. Architecture checks and catalog fixtures protect the shared
extension path.
