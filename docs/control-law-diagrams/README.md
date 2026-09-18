# Figure 3.1 DFCS longitudinal control

This directory has one control-law target only:

`ControlLawDiagrams.F16Block25.LongitudinalControl` transcribes ADA189675
Figure 3.1, `DFCS Longitudinal Control Block Diagram`.

No simplified substitute or invented pass condition is included. The source
authority is [ADA189675.pdf](../references/ADA189675.pdf).

The source gain schedules use `q_c`, the standard air-data symbol for impact
pressure (`p_t - p_s`). It is intentionally named `impactPressurePsf` in the
executable transcription. It must not be renamed dynamic pressure: the two are
approximately equal only in the low-speed incompressible limit.

## Current status

The complete visible Figure 3.1 topology is executable. Figure 3.2 supplies the
pitch-command gradient, F1 and F2 functions, N3/N3A/N4/N7/N8 schedules, N14
pitch-command lag, roll-rate contribution to the alpha limiter, and the
configuration-dependent structure filter. The pitch-rate, normal-load,
alpha-filter, P+I, trailing-edge-flap, and leading-edge-flap paths all have
equations and are exercised by an executable boundary harness.

The transcription distinguishes three kinds of information:

- source-readable values are copied directly;
- source-visible functions whose exact labels were lost in the public scan are
  represented by named, documented parameters;
- schedules produced outside Figure 3.1 enter through explicit inputs instead
  of being replaced by hidden zeroes.

The remaining scan-dependent numerical hypotheses are deliberately visible:

- N3A retains the source-visible zero-rise-fall-zero shape and 0.30 peak. Its
  250 and 600 psf interior pressure labels are marked with `*` in the diagram;
- the U-013 sensor pole defaults to 50 rad/s;
- the U-034 alpha-command limiter defaults to 0/25 g;
- the U-033 pre-P+I limiter defaults to -25/+25.

These are executable transcription parameters, not claims that the unreadable
scan proves those exact numbers. Replacing them does not change topology.

The page-overlap reconstruction now preserves the source's opposed N3/N3A path.
The repeated `q_c` labels are two drawings of the same impact-pressure net: the
left occurrence passes through N3, while the right occurrence passes right-to-left
through N3A and the Z multiplier. Their signed difference leaves the summing node
through its bottom connector. The main command chain contains one PA sum
followed by one feedback sum; the earlier duplicate feedback node was not present
in Figure 3.1 and has been removed.

The nine distinct Mode states are inputs to this Figure 3.1 module. That only
means the switching decision is made outside the Figure 3.1 module; its producer
may still be another module inside the same FLCC. MODE 4 and MODE 7 each feed two locations,
so their duplicate labels share one state. The diagram also feeds MODE 7 into
an arithmetic summing path, therefore Mode states use explicit numeric 0/1
semantics rather than an incompatible Boolean connector.

Figure 3.1 does not define every producer. N6, MODE 4 standby gain, M9, M10, M11,
M12, and the leading-edge standby/override command therefore remain explicit
module inputs. This preserves the Figure 3.1 boundary and makes missing data
visible to a caller. `LongitudinalControlVerification` supplies named values to
those inputs only for the verification scenario.

The elevator P+I path includes limited integration, the MODE 7 feedback branch,
and the source-visible ±25 inner-loop breakout feedback. The breakout output is
fed back to the integrator input; it is not added as a second actuator command.
The F2 static-stability path is added after the P+I path. The high-alpha path is
`alpha + F + N7*q - 13.4 - N6*Y`, and the low-alpha path is
`positive(0.322*(alpha + N4*q - 15.8))`.

## Open in OMEdit

1. Open `docs/control-law-diagrams/ControlLawDiagrams/package.mo`.
2. Expand `ControlLawDiagrams > F16Block25`.
3. Open `LongitudinalControl` in Diagram View.

## Verification

Run the completed Figure 3.2 gain checks with:

```powershell
.\verify-gains.ps1
```

This verifies every implemented breakpoint, including both N4 Note 1 branches.
The Note 1 decision itself remains a Figure 3.1 boundary input because the scan
defines the two values but does not place the condition logic inside this diagram.

Run the executable check with:

```powershell
.\verify.ps1
```

This command checks Figure 3.1 and simulates
`LongitudinalControlVerification` for two seconds. The harness drives every
boundary, switches every Figure 3.1 mode input, and fails if a surface output
diverges or the leading-edge output escapes its 0/25-degree limiter.

For a structure and flattened-equation dump:

```powershell
& 'C:\Program Files\OpenModelica1.27.0-64bit\bin\omc.exe' check.mos
```

Run the directed topology contract with:

```powershell
.\verify-topology.ps1
```

This rejects duplicate edges, multiple producers connected to one input, lines
with conflicting arrow directions, missing required N3/N3A/N8/F2/P+I paths,
obsolete reversed paths, non-executable placeholder blocks, and incorrect port
counts on critical nodes. Outputs may fan out because that is a normal signal
branch; every receiving input must still have exactly one source.

The Modelica material is documentation and analysis code. It is not linked to
the EFM and cannot change DCS behavior.
