# EFM unit and coordinate contract

This document is the normative unit contract. A field suffix is part of the
interface: values with different quantity, reference, or suffix must not be
compared, integrated, or assigned without an explicit conversion at the owning
boundary.

## Layered policy

The project intentionally does not force one unit system through every layer.
Real aircraft separate sensor encoding, aircraft-system engineering values,
pilot-selected values, and flight mechanics; the EFM uses the same separation.

| Layer | Unit policy | Owner |
|---|---|---|
| DCS ABI and world | Validate and translate DCS callbacks, coordinates, signs, and parameter encodings | `DcsBridge` |
| Simulation physics | SI, radians, radians/second | `Core::Simulation` |
| Aircraft Systems | Domain-native engineering units named in typed contracts | owning `System` contract |
| Pilot selection/presentation | Exact aviation display units such as whole heading degrees | cockpit/AFCS presentation seam |

`DcsBridge` is the only code that knows DCS ABI conventions. Simulation models
must not consume DCS draw-argument values. Aircraft Systems must not infer an
aviation heading from simulation world yaw.

## Simulation physics units

| Quantity | Unit | Suffix |
|---|---|---|
| time | second | `_s` |
| frequency | hertz | `_hz` |
| length/position/geometric altitude | metre | `_m` |
| speed | metre per second | `_mps` |
| acceleration | metre per second squared | `_mps2` |
| mass | kilogram | `_kg` |
| pressure | pascal | `_pa` |
| temperature | kelvin | `_k` |
| force | newton | `_n` |
| moment | newton metre | `_nm` |
| angle | radian | `_rad` |
| angular rate | radian per second | `_rad_s` |

Mach, aerodynamic coefficients, normalized pilot input, and `[0,1]` ratios are
dimensionless. Their field names still expose semantics and range.

## Flight-control aircraft-system contract

The FLCC receives already translated engineering quantities:

| Quantity | Unit/reference |
|---|---|
| measured pressure altitude | feet, `_ft` |
| measured vertical speed | feet/second, `_ft_s` |
| selected altitude and altitude error | feet, `_ft` |
| selected magnetic heading | exact whole degrees `[0,359]`, `_deg` |
| measured magnetic heading | continuous clockwise magnetic degrees `[0,360)`, `_deg` |
| heading error | wrapped degrees `[-180,180)`, `_deg` |
| roll/pitch attitude | radians, `_rad` |
| angle of attack and sideslip | radians, `_rad` |
| body rates | radians/second, `_rad_s` |
| normal acceleration | g, `_g` |
| primary surface command/state | radians and radians/second |
| pilot control authority | dimensionless `[-1,1]`, `_normalized` |

The AFCS altitude loop keeps selected altitude, measured pressure altitude,
altitude error, vertical-speed reference, and measured vertical speed in feet
and feet/second. Its command seam converts the resulting guidance objective to
`PitchReferenceRad` or normal-acceleration demand.

Pressure altitude is an explicit cockpit/navigation observation. The HMCS Lua
sensor adapter samples DCS `getBarometricAltitude()` and publishes both an
availability flag and the returned metre value. `CockpitBridge` converts that
value to feet exactly once. If the sensor value is absent or invalid, pressure
altitude remains unavailable: geometric `altitude_asl_m` must never be used as
an AFCS fallback.

The AFCS heading loop keeps selected heading, measured magnetic heading, and
wrapped heading error in degrees. `LateralGuidance` converts the completed
heading-loop output into `BankReferenceRad`; degree/radian conversion must not
occur on each Heading Set button press.

## Reference frames and signs

Simulation retains the local-body convention used by the EFM ABI:

- `x`: forward;
- `y`: up;
- `z`: right;
- positive roll: right wing down;
- positive pitch: nose up;
- positive yaw: nose left.

Pilot control inputs use pull/right-roll/left-yaw positive. DCS raw axis signs
are converted once by `DcsCommandRouter`.

Magnetic heading is a different quantity from DCS world yaw. It increases
clockwise toward the right and comes from the cockpit/navigation observation.
`world_yaw_rad` is simulation orientation only and must never substitute for
`magnetic_heading_deg`.

## Conversion ownership

| Boundary | Conversion owner | Rule |
|---|---|---|
| DCS raw command -> pilot command | `DcsCommandRouter` | adapt sign/range once |
| DCS body/world state -> simulation state | focused `DcsBridge` adapters | map axes and preserve SI/radians |
| cockpit barometric-altitude parameter -> pressure-altitude observation | `CockpitBridge` | validate availability and convert metres -> feet once; never fall back to geometric ASL |
| simulation vertical speed -> FLCC observation | flight-control observation adapter | metres/second -> feet/second once |
| cockpit heading parameter -> magnetic heading observation | `CockpitBridge` | radians from DCS parameter -> degrees once |
| heading loop -> bank reference | `LateralGuidance` | degrees -> radians only at the completed guidance seam |
| FLCC surface demand -> actuator/aerodynamics | no conversion | radians remain radians |
| physical surface angle -> DCS draw argument | `DcsBridge::DrawArgs` | normalize with visual travel calibration |
| Core values -> CSV/debug | projection owner | preserve source unit and include unit in channel/header |

## Deliberate normalized values

Normalized values remain valid for pilot axes, throttle, gear, brakes,
secondary-control positions, damage ratios, and DCS visual draw arguments.
Primary flight-control surface demand and state are never normalized in Core.

Private numerical helpers may use generic scalar names only when every operand
in one call has the same unit and the helper value never crosses a Module seam.

## Contributor rules

1. Put quantity, reference, and unit in every new physical field or type.
2. Perform a conversion once at its named boundary owner.
3. Never use a generic `Angle` type or implicit degree/radian conversion.
4. Add a boundary test for each conversion and a sign/wrap test for each new
   rotational path.
5. Debug Watch and CSV are read-only projections; they never feed control math.
