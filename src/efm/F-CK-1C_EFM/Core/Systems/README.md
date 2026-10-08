# SystemPipeline contributor guide

`SystemPipeline` is the deep module that owns aircraft Systems, validates their
setup declarations, routes commands and events, and commits shared
`AircraftData`.

The Pipeline is the sole production scheduler and commit point for these
aircraft owners. `AircraftSimulation` consumes only the completed aircraft
snapshot and does not call or own concrete Systems.

| System | Rate | Timing evidence | Responsibility |
|---|---:|---|---|
| `PilotControls` | 64 Hz | Project-defined fallback | DCS command integration and normalized pilot-control signals |
| `FlightControlComputer` | 64 Hz | F-16XL DFLCS reference; not confirmed F-CK-1C data | Configurable three-axis direct mapping; feedback, trim, modes and monitoring remain unimplemented |
| `FlightControlActuationSystem` | 256 Hz | Project-defined numerical integration rate | Elevator, aileron, and rudder actuator dynamics and feedback |
| `SecondaryFlightControls` | 64 Hz | Project-defined fallback | Flaps, slats, and airbrake |
| `LandingGear` | 64 Hz | Project-defined fallback | Gear, brakes, NWS, wheels, and suspension state |
| `Engine` | 64 Hz | Project-defined fallback | Engine device state, spool, nozzle, and fuel demand |
| `Fuel` | 64 Hz | Project-defined fallback | Fuel storage, supply, transfer, and consumed mass |
| `AirframeStructure` | 64 Hz | Project-defined fallback | Component integrity and damage ownership |
| `PropulsionDiagnostics` | 64 Hz | Project-defined fallback | Flight-test thrust-cut intent |

The FCC rate is based on NASA's description of F-16XL DFLCS control laws
running at 64 cycles per second:
[NASA TP-3547](https://ntrs.nasa.gov/api/citations/20040040334/downloads/20040040334.pdf).
No reliable device-specific F-16 or F-CK-1C rate was identified for the other
current Systems. Their 64 Hz values are explicitly project-defined fallbacks,
not aircraft facts. The 256 Hz actuation rate is a project-defined numerical
integration choice, not a claimed real-aircraft sampling or servo rate.

## Flight-data units through the pipeline

The project-wide normative contract is
[`docs/EFM_UNIT_CONVENTIONS.md`](../../../../../docs/EFM_UNIT_CONVENTIONS.md).
This section summarizes how Systems apply it; it does not define AP-only rules.

Units are part of each typed contract rather than an assumption local to a
controller. The required chain is:

| Stage | Contract | Unit rule |
|---|---|---|
| DCS atmosphere input | `AtmosphereInput` | altitude m, temperature K, speed/wind m/s, density kg/m^3, pressure Pa; field suffixes are authoritative |
| DCS surface input | `SurfaceInput` | heights m, world normal dimensionless unit vector |
| DCS mass input | `MassStateInput` | mass kg, body position m, body moment of inertia kg*m^2 |
| DCS world state | `WorldKinematicsInput` | world position m, velocity m/s, acceleration m/s^2, angular rate rad/s, angular acceleration rad/s^2 |
| DCS body adapter | `BodyKinematicsInput` | body velocity m/s, acceleration m/s^2, attitude/aerodynamic angles rad, rates rad/s; DCS handedness retained |
| retained System input | `AircraftObservation` | explicit `_m`, `_mps`, `_pa`, `_g`, `_deg`, `_rad`, and `_rad_s` field suffixes |
| pilot input | `PilotControlSignal` | normalized [-1, 1]; pull, right roll, and left yaw positive |
| FCC observation | `FlightControlObservation` | pressure-altitude ft, vertical speed ft/s, speed m/s, pressure Pa, load factor g, aerodynamic/attitude angles rad, rates rad/s, magnetic heading deg |
| AFCS selected target | `AutopilotModeLogicState` | pitch/roll rad, altitude ft, Heading Set exact integer degrees 0..359 |
| AFCS guidance | `AutomaticFlightGuidanceReference` | attitude/bank rad, vertical speed ft/s; experimental throttle remains normalized |
| shared control reference | `CoordinatedManeuverReference` | normal acceleration g, pitch/roll/yaw rates rad/s, sideslip rad |
| FCC output | `FlightControlActuatorCommand` | named primary-surface position demands normalized to [-1, 1]; actuation owns conversion to physical rad |
| actuator state | `FlightControlActuatorState` | physical surface position rad and rate rad/s |
| aerodynamic model input | private `AerodynamicsFrameInput` | body position m, aerodynamic/attitude angles rad, rates rad/s, primary surfaces rad; secondary devices remain normalized; legacy coefficient schedules receive deg explicitly |
| EFM force output | `ForceMomentOutput` | DCS body force N, body moment N*m, center-of-mass position m |
| diagnostic output | `FlightOutput` / `ControlOutput` | physical suffixes preserved; every control field is explicitly `_normalized` |

The sign convention also applies outside AFCS. `PilotControlSignal`, manual
FBW references, coordinated-turn feed-forward, measured body rates, actuator
commands, the aerodynamic model, frame output, Debug Watch, and CSV all use
the same Core local-body convention. Right yaw is therefore negative in those
contracts. The only raw control conversion is at the DCS input adapter: a DCS
pedal-axis value is converted before `PilotControlSignal` is published.

Core force and moment vectors retain the DCS local body axes because they are
returned directly to the EFM ABI: x forward, y up, z right; force is N, moment
is N*m, and application position is m. No System may reinterpret a component
as magnetic heading or silently invert it.

Degrees are deliberately retained in the magnetic-heading loop, pilot-selected
whole-degree Heading Set, and legacy aerodynamic schedules defined in degrees.
Attitude, AOA, sideslip, body-rate, and actual primary-surface positions use
radians. Primary-surface position demands are normalized and converted once
by FlightControlActuationSystem. Conversion occurs at the owning boundary, never incrementally on each
Heading Set press.

## System contract

Every System is a class derived from `System` and has two lifecycle methods:

- `setup()` declares exactly one fixed update rate or period, declares
  AircraftData reads/publications, and registers handlers.
- `step()` advances one scheduled device tick. `SystemStepContext` supplies
  that tick's scheduled simulation time and the System's own fixed `dt`.
  Each System reads one immutable time-bucket snapshot and publishes only
  through the Pipeline-owned reusable
  `SystemResult` buffer; the Pipeline returns the completed immutable
  AircraftData snapshot.

Setup completes in two stages. The Pipeline first collects every System's
declarations, then validates providers, types, initial values, single writers,
and handler ownership before committing any initial AircraftData.

`SystemPipeline` stores ordered scheduled-time buckets. All Systems due at the
same time read one shared immutable snapshot and commit as one batch. A later
bucket sees the preceding bucket's committed output. The next tick is derived
from the common epoch and invocation count, so host lateness does not shift the
device clock; a large host interval executes every due tick.

The first dynamic tick occurs after one complete period. Catalog entries carry
only the system ID and the factory; scheduling follows setup declarations.

## Adding an Entry

Place one `Entry.cpp` directly under the System directory:

```text
Core/Systems/MySystem/Entry.cpp
```

The directory name must be a valid C++ identifier. The Entry exposes one
factory in the matching catalog namespace:

```cpp
namespace Core
{
namespace Systems
{
namespace Catalog
{
namespace MySystem
{
SystemEntry create_entry()
{
	SystemEntry entry;
	entry.id = "my_system";  // 供 Pipeline 識別系統及回報錯誤。
	entry.factory = [](const FlightSetupContext& setup)
	{
		return std::make_unique<MySystem>(setup);
	};
	return entry;
}
}
}
}
}
```

MSBuild scans `Core/Systems/*/Entry.cpp` before compilation and generates the
catalog in the project intermediate directory. Do not edit a generated catalog
or add the Entry manually to either `.vcxproj`.

Adding a System must not require changes to `Fck1cEfm`,
`AircraftSimulation`, `SystemPipeline`, or another concrete System. Add its
directory, implementation, tests, and `Entry.cpp`; the shared MSBuild rules
discover the Entry for both production and native tests.

`FlightSetupContext` contains `StartMode`, the initial fuel load, the
composition-root-derived initial throttle-lever signal, the injected
DCS-neutral debug-telemetry sink needed to construct one flight. A System may
declare typed channels through `SystemSetup::declare_debug_channel()` and push
them with its scheduled simulation time. Debug publication is observational;
it neither reads nor writes AircraftData and does not wait for bucket commit.
Each System owns its production configuration at the appropriate construction
boundary. Databases may request a typed configuration through
Common/Configuration's `load<T>()` function. DcsBridge initializes the shared
FM root and diagnostic callback. The database determines its file path and load
timing, while its owner defines required fields and validation. The loader reads
UTF-8 JSONC, executes those rules, and returns a `LoadResult<T>` containing either
the complete validated structure or a failure diagnostic.
Missing required fields and extra object fields are errors. Duplicate names
follow the JSON parser's behavior without an additional diagnostic. Load errors are logged and returned;
the database decides whether initialization can continue. ControlLawData requires
valid settings and aborts its own initialization if loading fails.
A test may provide an explicitly merged configuration to a test
factory without changing its production Entry. Simulation
policies, including infinite fuel, invincibility, and easy flight, do not
belong in a System.

## AircraftData and handlers

Use only keys declared in `Core/Contracts/AircraftData.h`. A key has one
publisher; readers may require an initial value or explicitly accept an
uninitialized value. A missing provider, wrong type, duplicate publisher, or
missing required initial value fails setup. During `step()`, both `read()` and
`has()` reject keys that the current System did not declare during `setup()`.

`FrameInput` carries DCS callback-local values such as suspension samples and
the host `dt`. Systems may read callback-local samples when declared, but must
not use `FrameInput::dt_s` as their integration step; scheduler-driven
integration uses `SystemStepContext::dt_s`.
Semantic pilot commands enter through registered command handlers; AP and A/T
are not smuggled through per-frame parameter fields. `AircraftObservation`
carries retained, normalized flight state. `AircraftSimulation` updates only
observations whose availability flag is set, retains missing samples, and gives
the completed observation to the Pipeline. The Pipeline publishes both inputs
through the same typed snapshot; Systems do not know which DCS callback
produced them.

`PilotControls` separately publishes `PilotControlSignal` and the physical
`ThrottleLeverSignal`. While legacy A/T placement remains deliberately out of
scope, the FCC publishes its composed value as `EngineThrottleCommand`; this
transitional command must not be mistaken for physical lever position.

Commands have one registered handler per semantic `CommandId`. A handler may
latch intent immediately, but an observation-dependent transition or reference
capture occurs on the owner's next scheduled tick. Edge actions are queued so
multiple press/toggle/increase events cannot overwrite one another. Damage areas
have one semantic owner. Repair may have multiple subscribers. An unregistered
command or damage event returns `DispatchResult::Unhandled`; handlers are not
broadcast. Command, damage, and repair handlers receive a
`SystemActionContext` containing the Pipeline's current simulation time. This
lets an immediate diagnostic push carry the real operation time without using
wall-clock time or waiting for the next System tick.

Unexpected exceptions from System creation, setup, step, or a registered
handler receive that System's catalog ID and operation in a structured
`ExecutionError`. Systems do not write DCS logs; the error crosses the Core
boundary and DCSBridge records it once.

## Flight-control ownership

`FlightControlComputer` remains one physical-box System. It directly owns seven
internal components under `Components/` and five independent typed databases
under `Database/`. The internal `Executive` centrally defines the task table
with exact rational Hz and fixed row order. Each `ExecutiveTables` row stores
an immutable task definition beside its current runtime state. These components
are not separate Pipeline Systems. FLCC owns assembly and the Pipeline boundary,
while Executive owns internal timing and invocation order.

Setup declares five typed inputs and four typed outputs. Each step copies the
inputs into its database, advances Executive to the scheduled simulation time,
and publishes the database outputs. ControlLaws maps the three pilot axes through
configured piecewise linear curves and gains into normalized surface position
demands. Other component algorithms remain unimplemented; throttle demand retains
its initial value and both status snapshots remain unavailable. No FLCC commands are registered.
The exact boundary and scheduling contract are documented in the
[FLCC guide](FlightControlComputer/README.md).

Fuel registers only the preparation handlers used by the
simulation façade. The Pipeline validates that the handler set is complete and
belongs to the sole `FuelData` publisher. Fuel publishes the current frame's
accumulated consumed mass in `FuelData`; this is necessary when zero, one, or
multiple Fuel ticks occur during one DCS callback. Simulation converts that
value into one mass effect. Infinite-fuel policy remains in Simulation: it
marks the entire callback's Fuel interval as consumption-suppressed. Fuel still
computes and publishes the true total flow, publishes zero consumed mass for
that callback, and does not know why consumption was suppressed.

## Verification

Follow the
[`DLL build guide`](../../../../../docs/BUILD_DLL.md). A System change must
pass the native tests, architecture check, and System catalog fixtures. Add
focused tests for its setup declarations, handlers, scheduled timing, retained
data, and published frame output as applicable.
