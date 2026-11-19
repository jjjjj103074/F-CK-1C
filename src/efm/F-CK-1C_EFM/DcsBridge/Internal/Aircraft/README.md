# DCS aircraft interface

`DcsAircraftInterface` provides observations and actions for the current player's
DCS aircraft. BridgeContext owns one instance and shares it with its native I/O
adapters. The interface returns values and operation results. DCS owns the
underlying cockpit context, human-plane interface, payload and gun objects.

| Operation | Result |
|---|---|
| `read_gun_ammunition()` | Combined native gun rounds, source status, host identity and native exception |
| `read_countermeasures()` | Flare/chaff counts and source status |
| `dispense_countermeasure(kind)` | One-round native call with before/after observations |
| `reset_flight()` | Clears the per-flight library resolution state |

The implementation shares library discovery, symbol resolution, executable-memory
checks, fingerprints and guarded native calls. Gun and countermeasure capabilities
validate their own code profiles. Each operation reacquires aircraft-owned objects;
module identity changes invalidate the cached symbols and profiles. A dispense
operation checks the aircraft and method again before calling the engine.
The `returned` field records native call completion; the after-observation records
its measured effect.

`DcsAircraftBackend` is the internal environment seam. Its Windows adapter performs
process-memory reads and typed calls behind SEH. Native tests substitute the same
interface to exercise version checks, object changes, zero inventory and failures.
The interface mutex serializes its local state and operations. Bridge chooses the
DCS callback/phase for each capability, using its verified execution path.

GunAmmunitionSource currently consumes this interface, retaining its 10 Hz
observations, source diagnostics and per-flight error policy. CountermeasureExecutor
uses the same interface in the native I/O phase of ed_fm_simulate to observe inventory
and execute one round from CountermeasureSystem. Its feedback returns through
FrameInput and SystemPipeline; program scheduling belongs to CountermeasureSystem.

Core Systems consume typed FrameInput/AircraftData and publish typed actions to
FrameOutput through SystemPipeline. Further aircraft capabilities can add their
own focused adapter and contracts while reusing this module's environment access.
