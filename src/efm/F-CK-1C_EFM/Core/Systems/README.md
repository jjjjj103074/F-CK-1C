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
| `FlightControlComputer` | 64 Hz | F-16XL DFLCS reference; not confirmed F-CK-1C data | FBW/AFCS laws and normalized actuator commands |
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

The first dynamic tick occurs after one complete period. `SystemGroup` remains
catalog metadata only and never controls production execution order.

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
	return {
		"my_system",
		SystemGroup::Equipment,
		[](const FlightSetupContext& setup)
		{
			return std::make_unique<MySystem>(setup);
		}
	};
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
composition-root-derived initial throttle-lever signal, and the injected
DCS-neutral debug-telemetry sink needed to construct one flight. A System may
declare typed channels through `SystemSetup::declare_debug_channel()` and push
them with its scheduled simulation time. Debug publication is observational;
it neither reads nor writes AircraftData and does not wait for bucket commit.
A System-specific Entry captures that System's immutable production
configuration and passes it to the concrete factory. Simulation
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

`FlightControlComputer` is one physical-box System. Its `Autopilot/` and
`ControlLaws/` directories are software Modules inside that box, not additional
Systems and not independently scheduled devices. One 64 Hz FCC tick uses one
conditioned observation snapshot and follows this chain:

```text
FlightControlObservation + PilotControlSignal + actuator feedback
-> InputSignalManagement
-> ConfigurationAndMode (CAT schedule + this-tick maneuver envelope)
-> AP ModeLogic and vertical/lateral guidance
-> PilotCommandLaw
-> per-axis ControlReferenceSelection
-> joint GuidanceCoordination
-> common FBW inner loops and protection
-> FlightControlActuatorCommand
-> FlightControlActuationSystem
-> FlightControlActuatorState feedback
```

The AP owns selected targets, capture shaping, mode state, bypass,
stick-steering, and tracking/degradation monitoring. It publishes physical
pitch-attitude, vertical-speed, and bank-angle references; it never overwrites
pilot axes or publishes stick-equivalent pitch/roll commands. Manual and AP
references meet at `ControlReferenceSelection`, then use the same coordination,
CAT schedule, protection, inner-rate loop, and actuator-feedback path.

`GuidanceCoordination` consumes all selected axes together. It owns the single
bank-to-lift compensation and coordinated-turn calculation, returns an explicit
constraint reason when the combined request is infeasible, and stores no
cross-tick state. `AutopilotModeMonitor` observes that result and can change
authority on the following FCC tick; the same tick is never recalculated after
a mode change.

Flight-control commands have one owner: the FCC handler queues AP, provisional
CAT, and developer G-limiter actions for the next FCC tick. `PilotControls`
alone integrates controller/button bindings into `PilotControlSignal` and
`ThrottleLeverSignal`. The FCC alone publishes
`FlightControlActuatorCommand`, `FlightControlComputerSnapshot`, and
`AutomaticFlightControlSnapshot`; Actuation alone publishes
`FlightControlActuatorState`. Snapshots and CSV are read-only projections and
must not feed control calculations.

Evidence labels are part of the contract. The 30-degree AP bank limit,
20-degree-per-second AP roll-reference limit, and 0.5-to-2.0-g AP guidance
envelope are F-16 Reference-derived project decisions, not confirmed F-CK-1C
values. Capture gains, reference steps, monitor thresholds, CAT behavior, and
the 256 Hz actuator integration rate are Project-defined. G-limiter override is
Developer-only and unavailable in the production configuration. Experimental
A/T remains isolated compatibility behavior; its aircraft authenticity is not
claimed by this architecture.

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
