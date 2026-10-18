# EFM units and coordinates

Field suffixes are part of the interface. Convert quantity, reference and unit
explicitly at the owning boundary.

## Physical values

| Quantity | Unit / suffix |
|---|---|
| Time | second, `_s` |
| Length, position, altitude | metre, `_m` |
| Velocity, acceleration | `_mps`, `_mps2` |
| Mass, inertia | `_kg`, `_kg_m2` or the explicit field declaration |
| Pressure, density, temperature | Pa, kg/m³, K |
| Force, moment | N, N·m |
| Physical angle, angular rate | `_rad`, `_rad_s` |

Mach, coefficients, ratios and normalized controls are dimensionless.
DCS draw arguments are presentation values rather than physical surface angles.

## System contracts

| Contract | Units |
|---|---|
| `AircraftObservation` | SI; `alpha_deg` and `beta_deg` are degrees; attitude/rates are rad and rad/s |
| `FlightControlObservation` | Pressure altitude ft, vertical speed ft/s, heading degrees, AOA/sideslip/attitude rad, body rates rad/s; other fields retain named SI units |
| `PilotControlSignal` | Axes and trim inputs normalized to [-1, 1] |
| `ThrottleLeverSignal` | Left/right lever positions normalized to [0, 1] |
| `FlightControlActuatorCommand` | Surface-position demands in [-1, 1] |
| `FlightControlActuatorState` | Surface positions rad and rates rad/s |
| `ForceMomentOutput` | DCS local-body force N, moment N·m and application position m |

`make_flight_control_observation()` converts retained vertical speed to ft/s
and retained alpha/beta to radians. The current direct control law uses pilot
axes; feedback, trim application and AFCS guidance are unimplemented.

## Axes and signs

Local-body axes follow the EFM ABI: x forward, y up, z right.
Positive roll is right wing down; positive pitch is nose up; positive yaw is
nose left. The DCS body angular x/z/y components correspond to roll/pitch/yaw.

Pilot inputs and surface demands use pull/right-roll/left-yaw positive.
`DcsCommandRouter` adapts raw command signs once.
Magnetic heading increases clockwise and is independent of `world_yaw_rad`.

## Conversion owners

| Boundary | Owner and rule |
|---|---|
| DCS command -> semantic command | `DcsCommandRouter`: adapt sign/range |
| DCS body/world callbacks -> Core | DcsBridge adapters: name axes and preserve declared units |
| Cockpit barometric altitude -> observation | `CockpitBridge`: metres to feet |
| Cockpit magnetic heading -> observation | `CockpitBridge`: radians to degrees |
| Retained state -> flight-control observation | `make_flight_control_observation()`: m/s to ft/s, alpha/beta degrees to radians |
| Surface demand -> actuator target | Actuation: normalized demand times nominal travel to radians |
| Actuator state -> aerodynamics | Preserve physical radians; convert only for tables explicitly defined in degrees |
| Surface angle -> DCS draw argument | `DrawArgs`: normalize using visual travel calibration |
| Engine temperature -> DCS parameter | `ParamExport`: kelvin to Celsius, then the project-defined temperature rise |
| Core value -> telemetry | Preserve source unit in the channel/header |

Heading and pressure altitude carry availability flags. Missing barometric data
stays unavailable; geometric ASL altitude is a separate quantity.
Raw DCS radar/seeker units follow the declarations in
[`CommandIds.json`](../src/efm/F-CK-1C_EFM/DcsIds/CommandIds.json).

## Contributor rules

Name the quantity, reference and unit in each physical field.
Add conversion, sign and wrapping tests where relevant.
Use one owner for each conversion. Debug values are read-only observations.
