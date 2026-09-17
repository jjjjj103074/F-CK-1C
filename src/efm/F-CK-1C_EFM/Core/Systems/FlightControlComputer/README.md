# FlightControlComputer architecture

`FlightControlComputer` represents one physical FLCC box and is the only
flight-control `System` registered with `SystemPipeline`. Its software modules
are ordinary composed C++ objects; they are not separately scheduled aircraft
devices.

## Source layout

The root contains only the `Entry` factory, the `FlightControlComputer`
System adapter, the `FlightControlExecutive` orchestration, and this README.
Implementation files are grouped by responsibility without changing the
execution order or introducing independently scheduled Systems:

- `Configuration/`: FLCC configuration and validation.
- `Contracts/`: internal flight-control references and status types.
- `Input/`: observation and pilot-signal management.
- `CommandSystem/`: pilot/AP command production, selection, and coordination.
- `ModeAndGainScheduling/`: stores configuration and gain scheduling.
- `ControlLaws/`: control-law calculations and surface mixing.
- `Output/`: electronic output selection, limits, and actuator-command status.
- `Diagnostics/`: snapshots, cockpit projection, and debug telemetry.
- `Util/`: stateless flight-state and throttle-command calculations.

## Current production chain

> **Longitudinal status:** the failed Nz-to-q cascade has been replaced by a
> tagged Nz/q command contract and a single state-owning longitudinal law.
> Release tests pass; targeted DCS validation is still required before the
> behavior is accepted. The normative design and test procedure are documented
> in [`docs/LONGITUDINAL_FLIGHT_CONTROL_REDESIGN.md`](../../../../../../docs/LONGITUDINAL_FLIGHT_CONTROL_REDESIGN.md).

```text
FlightControlComputer adapter @ 64 Hz
  -> FlightControlExecutive
       -> InputSignalManagement
       -> FlightStateComputation
       -> ModeAndGainScheduling
       -> FlightControlCommandSystem
            -> PilotManeuverCommandLaw
            -> AutomaticFlightControl
            -> authority selection and joint guidance coordination
       -> FlightControlLaws
            -> longitudinal, lateral, and directional laws
            -> aircraft-specific surface command mixer
       -> FlightControlOutputSystem
       -> FlightControlDiagnostics (read-only projection)
  -> FlightControlActuatorCommand (physical radians)
```

The Executive owns deterministic execution order and integer subrates. Fast
measurement handling, pitch shaping, all three axis laws, and output run at
64 Hz. Roll-stick and pedal shaping run at the AFTI/F-16-reference 32 Hz
cadence and hold their last result between updates.
Slow air-data gain scheduling runs at the reference-derived 4 Hz cadence.
These rates are not confirmed F-CK-1 data. The external
`FlightControlActuationSystem` remains a separate physical-device System at the
project-defined 256 Hz integration rate.

The landing-gear handle selects the longitudinal command law for both pilot
and AFCS references: gear-up guidance is converted to a normal-acceleration
command, while gear-down guidance is converted to a pitch-rate command.
`FlightStateComputation` derives flight-path angle from vertical speed and
airspeed so this conversion follows the point-mass relation between flight-path
rate and normal acceleration instead of switching law according to AP mode.

The normative unit and reference-frame contract is
[`docs/EFM_UNIT_CONVENTIONS.md`](../../../../../../docs/EFM_UNIT_CONVENTIONS.md).
Pressure-altitude guidance uses feet, magnetic-heading guidance uses degrees,
and attitude, body-rate, AOA, sideslip, and primary-surface paths use radians.
The DCS world yaw is never an aviation heading.

Debug telemetry keeps the raw boundary `Actual Nz` separate from `Filtered Nz`.
The latter is the input-signal-managed value used to form the published `Nz
Error`, so a DCS trace can reconstruct the control-law calculation.

## Output and future actuator channels

`FlightControlOutputSystem` owns electronic-law selection, bumpless fading on
selection changes, command and feedback validation, physical demand limits,
electronic saturation reporting, actuator-tracking assessment, and the typed
conversion to the three modeled primary channels. It does not hide invalid
commands or feedback: those fail explicitly. The external actuation System
owns servo lag, rate, position, and physical saturation at 256 Hz.
Each actuator surface reports its signed physical position-limit state, whether
the stop has actually been reached, and its rate-limit state explicitly. The
64 Hz FLCC consumes those device facts; it never reconstructs a physical stop
from its electronic mixer limits.

Electronic command saturation and physical actuator saturation are separate
facts. The former means the FLCC demand exceeded configured surface travel
before transmission; the latter is actuator feedback from the 256 Hz physical
device. The AFCS monitor may combine them to assess available control authority,
but diagnostics retain both sources independently.
`ControlAuthorityLimited` is reported separately after an AOA-protection
nose-down command has exhausted longitudinal electronic or symmetric-
stabilator travel authority and alpha has failed to recover for the project-
defined qualification interval. Saturation on another axis cannot raise this
longitudinal state.

Create a separate public `ActuatorSignalManagement` module only when a real
second electronic channel, channel-specific feedback, or persistent channel
selection state exists. At that point it owns surface-to-channel routing,
feedback selection, disagreement, and channel availability. It must not own
hydraulic/servo physics or select a control law. A future integrity module
consumes its status and applies reconfiguration on the following FLCC tick.

## Deferred integrity and lifecycle modules

Do not add empty BIT, voting, redundancy, or failure-manager classes. Materialize
`SystemIntegrity` only when all three exist:

1. a real failure or disagreement source;
2. persistent qualification state;
3. a control-law or channel reconfiguration consumer.

Its future result is an immutable next-tick availability/reconfiguration
assessment. Same-tick input and output validity checks remain in their signal
owners; the Executive must not rerun a completed control-law tick.

Materialize `FlightControlLifecycle` only when cold start, restart, BIT, or
power-state transitions change FLCC operational availability. It will own that
state machine while the Executive only sequences it. Hot-start defaults alone
do not justify a lifecycle class.

## Developer-only behavior

Direct control law, G-limiter override, and experimental auto-throttle are
independent developer features. They are disabled and unavailable in the
production configuration unless an explicit development configuration enables
them. None is evidence that the real F-CK-1 provides that feature. Experimental
auto-throttle remains a throttle side branch and never enters the surface
control laws.

## Dependency rules

- Internal control modules depend only on typed Core contracts and their
  immutable configuration.
- Consumers include only `FlightControlCommandSystem` and `FlightControlLaws`.
  Pilot mapping, reference selection, guidance coordination, numerical helpers,
  and inner-loop mechanics remain under their respective `Internal` folders.
- No internal module reads `AircraftDataView`, DCS parameters, CSV, or Debug
  Indicator state.
- `FlightControlDiagnostics` may observe completed tick results but cannot feed
  any control calculation.
- Persistent algorithm state has one owner. The Executive stores counters and
  held subrate configuration only; it does not duplicate controller state.
- Production has one control path. Do not add an old/new-law switch, a silent
  fallback, or fake-success redundancy.
