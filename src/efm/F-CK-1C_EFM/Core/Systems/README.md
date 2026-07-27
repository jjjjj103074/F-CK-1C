# SystemPipeline contributor guide

`SystemPipeline` is the deep module that owns aircraft Systems, validates their
setup declarations, routes commands and events, and commits shared
`AircraftData`.

The Pipeline is the sole production scheduler and commit point for these
aircraft owners. `AircraftSimulation` consumes only the completed aircraft
snapshot and does not call or own concrete Systems.

- `FlightControlComputer`
- `PrimaryFlightControls`
- `SecondaryFlightControls`
- `LandingGear`
- `Engine`
- `Fuel`
- `AirframeStructure`

## System contract

Every System is a class derived from `System` and has two lifecycle methods:

- `setup()` declares AircraftData reads/publications and registers handlers.
- `step()` advances one complete frame. Each System reads one immutable group
  snapshot and writes only to its reusable `SystemResult`; the Pipeline
  returns the completed immutable AircraftData snapshot.

Setup completes in two stages. The Pipeline first collects every System's
declarations, then validates providers, types, initial values, single writers,
and handler ownership before committing any initial AircraftData.

Control Systems run first and commit as one batch. Equipment Systems then read
that completed Control snapshot and commit as a second batch. Systems in the
same group never see each other's pending results. The completed frame becomes
externally visible only after both groups finish successfully.

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

`FlightSetupContext` contains the immutable aircraft configuration, `StartMode`,
and initial fuel load needed to construct one flight. Simulation policies,
including infinite fuel, invincibility, and easy flight, do not belong in a
System.

## AircraftData and handlers

Use only keys declared in `Core/Contracts/AircraftData.h`. A key has one
publisher; readers may require an initial value or explicitly accept an
uninitialized value. A missing provider, wrong type, duplicate publisher, or
missing required initial value fails setup.

`FrameInput` carries frame-local values such as `dt`, autopilot commands, and
suspension samples. `AircraftObservation` carries retained, normalized flight
state. The Pipeline updates only observations whose availability flag is set;
missing samples keep their previous committed values.

Commands have one registered handler per semantic `CommandId`. Damage areas
have one semantic owner. Repair may have multiple subscribers. An unregistered
command or damage event returns `DispatchResult::Unhandled`; handlers are not
broadcast.

Fuel registers the preparation and mass-delta handlers required by the
simulation façade. The Pipeline validates that the handler set is complete and
belongs to the sole `FuelData` publisher. Infinite-fuel policy remains in
Simulation; it can skip Fuel advancement without making Fuel aware of that
policy.
