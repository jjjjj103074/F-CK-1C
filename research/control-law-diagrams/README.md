# F-16 longitudinal control model

`ControlLawDiagrams.F16Block25.LongitudinalControl` transcribes
[ADA189675 Figure 3.1](../references/ADA189675.pdf).
Figure 3.2 supplies gain schedules. The Modelica package is an executable
research model, separate from the C++ EFM.

## Model boundary

The model contains the pitch-rate, normal-load, alpha-filter, P+I,
trailing-edge-flap and leading-edge-flap paths.
`LongitudinalControlVerification` supplies a two-second boundary exercise.

`impactPressurePsf` represents source q_c, impact pressure (p_t - p_s).
It is distinct from dynamic pressure.
The opposed N3/N3A branches share that pressure input and combine at one sum.
The command chain contains one PA sum followed by one feedback sum.

Nine mode inputs use numeric 0/1 semantics. MODE 4 and MODE 7 each feed two
locations with the same state; MODE 7 also enters an arithmetic sum.
N6, MODE 4 standby gain, M9–M12 and the leading-edge standby/override command
are external inputs because Figure 3.1 does not define their producers.

The elevator path includes limited integration, MODE 7 feedback and ±25
breakout feedback into the integrator input. F2 adds static-stability feedback
after P+I. Alpha paths include:

```text
high alpha: alpha + F + N7*q - 13.4 - N6*Y
low alpha:  positive(0.322*(alpha + N4*q - 15.8))
```

## Numerical assumptions

Readable values follow the source. Unreadable labels use named parameters:

- N3A uses the visible zero-rise-fall-zero shape and 0.30 peak;
  its 250/600 psf interior labels remain scan-dependent.
- U-013 sensor pole defaults to 50 rad/s.
- U-034 alpha-command limiter defaults to 0/25 g.
- U-033 pre-P+I limiter defaults to -25/+25.

These parameters expose uncertainty in the transcription.
They are not verified F-CK-1C coefficients or an implemented EFM control law.

## Open and verify

In OMEdit, open `ControlLawDiagrams/package.mo` and select
`F16Block25.LongitudinalControl` in Diagram View.

Run in PowerShell 7 from the repository root:

```powershell
./research/control-law-diagrams/verify-topology.ps1
./research/control-law-diagrams/verify-gains.ps1
./research/control-law-diagrams/verify.ps1
```

Topology verification uses Python. Gain and execution checks use OpenModelica;
their default executable path is
`C:/Program Files/OpenModelica1.27.0-64bit/bin/omc.exe`.
Supply `-PythonPath` or `-OmcPath` for a different installation.

The topology contract checks edges, producers, directions and required paths.
Gain checks cover implemented breakpoints and both N4 Note 1 branches.
The condition selecting those branches remains an external input.
Execution checks exercise modes and surface bounds; they do not establish
closed-loop aircraft performance or DCS behavior.
