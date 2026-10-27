---
schema_version: 1
record_type: guide
source_id: SRC-000001
version_id: v001
original_asset_id: PDF-001
review:
  state: cross_checked
  scope: >-
    Chapter and appendix prose condensed against all 186 PDF pages' extracted
    text; selected original page renders checked for the controller assumptions,
    simplified diagrams, test conditions, bibliography caveat, and Appendix A
    coefficient example. This does not verify every equation, diagram connection,
    plotted value, or numerical table, reproduce the simulation, or independently
    validate the thesis author's aircraft-model claims.
  method: >-
    Read the complete page-indexed text extraction; compare selected page renders
    at PDF pp. 40, 50-53, 94, 162, and 182. Use actual PDF indices rather than a
    constant offset from printed labels; preserve source limitations explicitly.
  checked_by: Codex
  checked_at: '2026-10-24'
publication:
  git_storage:
    status: allowed
    basis:
      - kind: license
        ref: Project LICENSE - MIT documentation terms
        url: null
        scope: Newly authored guide prose only; no relicensing of the original report or figures.
      - kind: statutory
        ref: Taiwan Copyright Act, Article 10-1
        url: https://law.moj.gov.tw/LawClass/LawAll.aspx?PCode=J0070017
        scope: Independently worded explanation of attributed facts, methods, and principles; not a reproduced chapter or diagram.
    conditions:
      - Preserve attribution, exact locators, and source-specific limitations.
      - Original report and figures retain the separate assessments in source.yaml.
    reason: This guide is newly authored factual condensation, not a copy or full translation of the thesis.
    assessed_by: Codex
    assessed_at: '2026-10-24'
  ai_processing:
    status: allowed
    basis:
      - kind: license
        ref: Project LICENSE - MIT documentation terms
        url: null
        scope: Agent reading and processing of this newly authored guide prose, not blanket permission for source assets.
      - kind: statutory
        ref: Taiwan Copyright Act, Article 10-1
        url: https://law.moj.gov.tw/LawClass/LawAll.aspx?PCode=J0070017
        scope: Independently worded explanation of attributed facts, methods, and principles.
    conditions:
      - Preserve the guide's scope and provenance when reusing its statements.
      - Apply source.yaml assessments before supplying original images or report excerpts.
    reason: Agent processing of this guide's independently authored factual explanation is permitted; original assets are assessed separately.
    assessed_by: Codex
    assessed_at: '2026-10-24'
coverage:
  outline_complete: true
  summary_scope: >-
    Basic substantive condensation of all seven chapters and five appendices,
    plus front/back-matter identification. Textual findings are attributed to
    the author; equations, diagram connections, and chart traces are not fully
    transcribed or independently verified. No full aerodynamic database or
    complete simulator source listing is claimed to be present in this PDF.
  summarized_sections:
    [SEC-001, SEC-002, SEC-003, SEC-004, SEC-005, SEC-006, SEC-007, SEC-008,
     SEC-009, SEC-010, SEC-011, SEC-012, SEC-013, SEC-014, SEC-015, SEC-016]
  reviewed_sections:
    [SEC-001, SEC-002, SEC-003, SEC-004, SEC-005, SEC-006, SEC-007, SEC-008,
     SEC-009, SEC-010, SEC-011, SEC-012, SEC-013, SEC-014, SEC-015, SEC-016]
sections:
  - section_id: SEC-001
    title: Front matter and abstract
    locator: {type: pdf, asset_id: PDF-001, page_start: 1, page_end: 19}
    relation_ids: []
  - section_id: SEC-002
    title: I. Introduction
    locator: {type: pdf, asset_id: PDF-001, page_start: 20, page_end: 25}
    relation_ids: []
  - section_id: SEC-003
    title: II. Simulation Specifications
    locator: {type: pdf, asset_id: PDF-001, page_start: 26, page_end: 36}
    relation_ids: [REL-004]
  - section_id: SEC-004
    title: III. Aircraft and Flight Control System Description
    locator: {type: pdf, asset_id: PDF-001, page_start: 37, page_end: 55}
    relation_ids: [REL-001, REL-002]
  - section_id: SEC-005
    title: IV. Simulation Modelling
    locator: {type: pdf, asset_id: PDF-001, page_start: 56, page_end: 79}
    relation_ids: []
  - section_id: SEC-006
    title: V. Simulation Design
    locator: {type: pdf, asset_id: PDF-001, page_start: 80, page_end: 88}
    relation_ids: []
  - section_id: SEC-007
    title: VI. Simulator Testing and Verification
    locator: {type: pdf, asset_id: PDF-001, page_start: 89, page_end: 129}
    relation_ids: [REL-003]
  - section_id: SEC-008
    title: VII. Conclusions and Recommendations
    locator: {type: pdf, asset_id: PDF-001, page_start: 130, page_end: 132}
    relation_ids: []
  - section_id: SEC-009
    title: Appendix A. SIMTACS User's Guide
    locator: {type: pdf, asset_id: PDF-001, page_start: 133, page_end: 165}
    relation_ids: []
  - section_id: SEC-010
    title: Appendix B. Intermachine Cable Hookups
    locator: {type: pdf, asset_id: PDF-001, page_start: 166, page_end: 169}
    relation_ids: []
  - section_id: SEC-011
    title: Appendix C. Pilot Control Schematics
    locator: {type: pdf, asset_id: PDF-001, page_start: 170, page_end: 172}
    relation_ids: []
  - section_id: SEC-012
    title: Appendix D. Cockpit Graphics Display
    locator: {type: pdf, asset_id: PDF-001, page_start: 173, page_end: 177}
    relation_ids: []
  - section_id: SEC-013
    title: Appendix E. Simulator Source Code
    locator: {type: pdf, asset_id: PDF-001, page_start: 178, page_end: 180}
    relation_ids: [REL-005]
  - section_id: SEC-014
    title: Bibliography
    locator: {type: pdf, asset_id: PDF-001, page_start: 181, page_end: 182}
    relation_ids: [REL-005]
  - section_id: SEC-015
    title: Vita
    locator: {type: pdf, asset_id: PDF-001, page_start: 183, page_end: 183}
    relation_ids: []
  - section_id: SEC-016
    title: Report documentation and terminal blank page
    locator: {type: pdf, asset_id: PDF-001, page_start: 184, page_end: 186}
    relation_ids: []
---

# SIMTACS: F-16 simulation and simplified flight-control guide

This guide condenses Mark W. Kassan's December 1987 AFIT thesis,
*F-16 Simulator for Man-in-the-Loop Testing of Aircraft Control Systems
(SIMTACS)*. All locators mean **one-based PDF pages in SRC-000001/v001/PDF-001**,
including covers. The scan has 186 PDF pages; split foldouts and the second
microfiche cover at PDF pp. 99-100 prevent a constant printed-page offset.

The subject is a research simulator using **F-16A original-horizontal-tail
aerodynamics with wingtip AIM-9 missiles**, combined with a **simplified Block 25
digital flight controller**. It is neither a complete operational F-16 control
specification nor evidence of F-CK-1C control laws. Its wide aerodynamic table
coverage must not be confused with its single-condition validation or its
controller's fixed design point (PDF pp. 37-39, 50, 89, 132).

## SEC-001: Front matter and abstract

The title and degree pages identify Kassan, the Air Force Institute of
Technology, report AFIT/GE/ENG/87D-30, and December 1987. The document bears an
unlimited-public-release statement. Contents, figure/table lists, and notation
precede the abstract (PDF pp. 3-19). Hosting provenance and action-specific rights
are recorded separately in `source.yaml`; a host is not necessarily the author.

The abstract describes nonlinear aerodynamic tables, inertially cross-coupled
motion, a simplified F-16 controller, and an initial pilot cockpit on two EAI
SIMSTAR hybrid computers. It also states that processor speed limits simulator
response (PDF p. 19). The later performance section explicitly says real-time
operation and man-in-the-loop performance evaluation were not achieved
(PDF p. 129); the abstract's development goal is not a completed capability.

## SEC-002: I. Introduction

The author proposes an educational engineering-design tool rather than a pilot
training simulator: a student should be able to insert a control-law design and
assess its interaction with aircraft dynamics and eventually a human pilot.
Linear computer-aided-design models and approximate pilot transfer functions
motivate, but do not replace, this intended pilot evaluation (PDF pp. 20-22).

The literature survey links output-feedback/PI design, quantitative feedback
theory, stochastic control, pilot handling-quality evaluation, and simulation.
These are background citations, not methods all implemented or compared by this
thesis. Cooper-Harper ratings are discussed as a motivation; no completed
Cooper-Harper pilot-rating study is reported here (PDF pp. 22-25, 129).

## SEC-003: II. Simulation Specifications

The intended simulator combines nonlinear aircraft data and nonlinear motion,
modifiable controller code, a force-stick cockpit, and sufficiently short input
to display delay for pilot interaction. The author separates software routines
so later users can change controllers without redesigning the whole simulator
(PDF pp. 26-28).

SIMSTAR #114 is primarily assigned digital aerodynamic lookup/interpolation;
SIMSTAR #115 runs analog dynamics and the digital controller. Analog port links
exchange state variables and dimensionalized forces/moments. A planned Evans and
Sutherland PS340 display is replaced by a simpler oscilloscope pitch/roll display
(PDF pp. 28-33). The author's descriptions of effectively instantaneous analog
computation are hardware-model assumptions, not a measured zero-delay guarantee.

The target complete cycle is about 15 ms, but the author reports approximately
125 ms for #114's interpolation code alone and 20 ms for #115's controller code.
FORTRAN output restrictions also limit PS340 updates to approximately seven per
ten seconds in the attempted arrangement (PDF pp. 34-35). These are historical
hardware results, not recommended sample periods for this project.

Model limitations include constant mass, a symmetric aircraft, an inertial
earth, and air at rest relative to earth. Ground effects, speedbrake and landing
gear effects, and engine gyroscopic effects are omitted. Some aeroelastic
effects are retained. The available F-16A OHT clean database bounds the model;
the crude cockpit provides only force-stick pitch/roll inputs and pitch/roll
display (PDF pp. 35-36).

## SEC-004: III. Aircraft and Flight Control System Description

### Configuration and controller identity

The aerodynamic subject is the F-16A OHT with wingtip AIM-9 missiles. Its surfaces
include symmetric/differential horizontal tails, flaperons, leading-edge flaps,
and rudder. The text compares it with AFTI/F-16 and notes that canard-based
designs are outside this current model (PDF pp. 37-38).

The control-law reference is a Block 25 F-16 DFCS functional-block-diagram
package obtained through the F-16 System Program Office. Figures 3.1-3.5 show
the fuller reference; **Figures 3.6-3.7 show the thesis's simplified controller**.
Do not treat the two sets as equivalent implementations (PDF pp. 38-53).

### Feedback structure and simplifications

The longitudinal description combines pitch-rate and normal-acceleration
feedback with a proportional-plus-integral forward controller; angle-of-attack
feedback supplies static stabilization. Lateral-directional control uses roll
rate feedback to flaperon/differential-tail control, lateral acceleration and
yaw-rate feedback to rudder, and an aileron-rudder interconnect (PDF pp. 39, 50).

The author fixes gain values at Mach 0.9 and altitude 20,000 ft; Table I lists
nine gain constants. Mechanical/physical and anti-windup limiters are removed,
with an explicit assumption that high rates will not overwork the aircraft
(PDF p. 50). Other simplifications remove analog demodulators, gun compensation,
selected structural/roll filters, and automatic angle-of-attack limiter loops;
use a first-order actuator lag with a 20 rad/s pole; fix leading-edge-flap
deflection at zero; and assume no gear/flap extension or refueling mode
(PDF p. 53). These omissions must accompany any reuse of the diagrams.

Figures 3.8-3.10 give stick/pedal force-to-command gradients for normal
acceleration, roll rate, and rudder command (PDF pp. 54-55). Their plotted
breakpoints and signs require visual reading; they are not fully transcribed
in this guide. The author's assessment that simplification causes little
degradation is scoped to the chosen condition and surrounding regime, not
independently established across the operational flight envelope (PDF p. 55).

### Diagram navigation and readability

- Figure 3.1, fuller longitudinal diagram:
  [PDF p. 40](figures/v001/figure-3-1-p040.png) and
  [PDF p. 41](figures/v001/figure-3-1-p041.png), read together.
- Figure 3.2, fuller longitudinal gain functions:
  [PDF p. 42](figures/v001/figure-3-2-p042.png) and
  [PDF p. 43](figures/v001/figure-3-2-p043.png), read together.
- Figure 3.3, fuller lateral-directional diagram:
  [PDF p. 44](figures/v001/figure-3-3-p044.png) and
  [PDF p. 45](figures/v001/figure-3-3-p045.png), read together.
- Figure 3.4, fuller lateral-directional gain functions:
  [PDF p. 46](figures/v001/figure-3-4-p046.png) and
  [PDF p. 47](figures/v001/figure-3-4-p047.png), read together.
- Figure 3.5, fuller surface mixer:
  [PDF p. 48](figures/v001/figure-3-5-p048.png) and
  [PDF p. 49](figures/v001/figure-3-5-p049.png), read together.
- Figures 3.6 and 3.7, simplified longitudinal and lateral-directional diagrams:
  [PDF p. 51](figures/v001/figure-3-6-p051.png) and
  [PDF p. 52](figures/v001/figure-3-7-p052.png) respectively.
- Table I, fixed simplified gains and initial assumptions:
  [PDF p. 50](figures/v001/table-1-p050.png).

The fuller diagrams are split foldouts with small, degraded labels; inspect
both pages of each pair. The simplified diagrams are sideways in the scan but
substantially clearer. OCR does not reliably capture connection signs, limits,
subscripts, or line crossings. A verified controller implementation cannot be
reconstructed from this guide or OCR alone.

Figures 3.1-3.5 are all saved as their complete pairs of original PDF page
renders. No cropping, invented reconnection, or stitched reconstruction is
claimed. The complementary pages retain the scan's overlap and original defects;
completeness here means preserving both available scan pages, not certifying
every label or connection as legible. See the source package's
[Figure 3.3-3.5 review gallery](README.md#figures-33-35-paired-page-review).

## SEC-005: IV. Simulation Modelling

### Frames, dynamics, and atmosphere

The chapter uses body, stability, wind, and earth/navigation frames. Aerodynamic
data are expressed in stability axes; forces and moments are transformed before
integrating six-degree-of-freedom motion. It includes angular-rate coupling,
Euler-angle kinematics, earth-relative position/velocity, and accelerometer
offset effects (PDF pp. 56-64). Figures 4.1-4.3 locate signs, frames, and the
calculation flow. Surface signs and equations must be checked from original
pages before coding; mathematical OCR is not a verified transcription.

The atmosphere supplies density, speed of sound, Mach number, dynamic/static
pressure, and impact pressure. It uses tropospheric/stratospheric assumptions
with a boundary stated as 36,089.24 ft and distinct subsonic/supersonic impact
pressure formulas (PDF pp. 65-68). Table II tabulates altitude and density, but
the model has not been independently reconciled or validated by this guide.

### Aerodynamic data organization and interpolation

The described database has 18,816 values in eight groups (PDF pp. 68-73):

| Group | Content | Described values |
|---|---|---:|
| I | Baseline lift, drag, and pitching moment | 6,300 |
| II | Baseline side force, rolling moment, and yawing moment | 3,276 |
| III | Flaperon/differential-tail roll and yaw derivatives | 1,680 |
| IV | Side-force control effects and flight-test corrections | 560 |
| V | Rudder effects | 1,260 |
| VI | Angular-rate and angle-of-attack-rate derivatives | 1,400 |
| VII | Flexible/rigid ratios | 1,140 |
| VIII | Flexibility increments in lift and pitching moment | 3,200 |

Each group has its own independent variables and breakpoints. For example,
Group I spans angle of attack -4 to +34 deg, seven Mach breakpoints from 0.2
to 2.0, five symmetric-tail settings, and three leading-edge-flap settings
(PDF p. 69). Other groups have different bounds; these are not one universal
joint operating envelope. Lateral data use absolute sideslip with sign restored
later. For rudder-derivative lookup, magnitudes below 10 deg use the 10-deg
table entry; this does not command a 10-deg rudder deflection (PDF pp. 69-71).

The storage order advances the first independent variable fastest. Lookup
finds neighboring breakpoints, then straight-line interpolation is applied
successively in each dimension, extending from two to four variables
(PDF pp. 68, 73-75). The explanatory interpolation assumes an in-range point;
this guide does not infer a clamping or extrapolation policy.

### Coefficient assembly and sampled controller

The six total aerodynamic coefficients combine tabulated baseline values,
control increments, dynamic derivatives, flexibility ratios/increments, and
flight-test corrections. Stability/dynamic derivatives use radians, whereas
control-deflection derivatives use degrees. The thesis uses nominal CG 0.35,
zero additional drag increment, and omits the optional special pitching-moment
increment (PDF pp. 75-78). These are source-specific choices, not project
configuration defaults.

For controller transfer functions, the author substitutes first-backward
differences and keeps prior sample values. A worked first-order example shows
how the current output depends on the current input, prior output, and sample
interval; higher-order cases are referred to another text (PDF pp. 78-79).
This is one chosen discretization method, not proof that arbitrary sample
times preserve a continuous design's behavior.

## SEC-006: V. Simulation Design

The design separates lookup/interpolation, nonlinear dynamics, digital control,
and cockpit display. Figures 5.1-5.2 show their data exchange and placement
across the two machines (PDF pp. 80-82).

At initialization, `SETUP` reads `FILELIST` and `AC.DAT`, stores breakpoints and
the 18,816 data entries in dimension-specific arrays, and reportedly takes
about 100 seconds. At each digital derivative execution, `LOOKUP` evaluates
46 table functions, assembles six total coefficients, and supplies their
dimensionalized values to the analog dynamics machine (PDF pp. 81-83).

Separate inertially coupled and decoupled dynamics programs share one lookup
routine. Consequently, removing inertial coupling does not remove coupling
already represented in the aerodynamic data when both modes run together
(PDF p. 84). Actuator lags form part of the dynamics; limited analog-component
capacity places the atmosphere on #114 rather than #115 (PDF pp. 84-85).

The digital controller uses explicit difference equations, initial history
values, and sample-dependent coefficients prepared in an initialization
routine. Its history assignments occur after current outputs are computed
(PDF pp. 85-86). The oscilloscope display instead uses a common oscillator,
roll-angle sine/cosine scaling, and pitch offset to draw an attitude reference;
the reported 100 Hz oscillator is a display-generation choice (PDF pp. 86-87).

## SEC-007: VI. Simulator Testing and Verification

The author checks closed-loop responses at **Mach 0.9, altitude 20,000 ft,
one-g wings-level trim**, comparing with externally supplied F-16/AFTI-F-16
responses and expected behavior. Table III includes weight 23,555 lb,
CG 0.35, symmetric-tail trim -2.8 deg, zero leading-edge-flap deflection,
angle of attack/pitch 1.9 deg, and zero sideslip (PDF pp. 89, 94).
The [Table III page render](figures/v001/table-3-p094.png) preserves the
original condition table; it is not an F-CK-1C parameter recommendation.
No independent flight-test dataset or numerical error metric is reproduced
here. The comparison source is further identified in the bibliography as a
personal interview (PDF p. 182, reference 19).

Coupled cases comprise trim settling, an incremental one-g pitch command,
a 45 deg/s roll command, and a rudder/sideslip case (Figures 6.1-6.4;
PDF pp. 90-110, excluding microfiche covers at 99-100). The author reports
trim settling in approximately two seconds, a pitch maneuver reaching two g
from one-g trim, and a roll response of about 32 deg/s rather than the commanded
45 deg/s, attributed to type-zero roll-loop steady-state error
(PDF pp. 94, 101). These are reported simulator responses, not operational
F-16 performance guarantees.

The text explains cross-axis responses during roll/rudder maneuvers and notes
a **reversed plotted rudder-pedal-force polarity**. The chart's apparent input
sign must not be copied without the author's correction (PDF pp. 106, 128-129).
Uncoupled cases use similar categories but a **5 deg/s**, not 45 deg/s, roll
command (Figures 6.5-6.8, PDF pp. 112-127). Thus those roll traces are not
matched-input quantitative evidence of coupling effects. The author uses them
for qualitative comparison (PDF pp. 111, 128).

Most importantly, interpolation speed prevents real-time operation. A pilot
can manipulate the crude cockpit, but large time scaling prevents the intended
man-in-the-loop performance evaluation (PDF p. 129). No handling-quality result
or completed real-time validation should be inferred.

## SEC-008: VII. Conclusions and Recommendations

The author concludes that the SIMSTAR architecture can support this type of
research simulator, while treating real-time performance as unfinished. Proposed
improvements include faster lookup hardware/software, faster digital processors,
more reliable systems, an independent graphics updater, pilot trim, and a
throttle instead of the current automatic zero-forward-acceleration thrust
arrangement (PDF pp. 130-131).

The thesis explicitly limits its evaluation to one flight condition and calls
for other-condition validation, better analog scaling, and a more efficient
lookup search. Its optimistic future-capability statements are proposals, not
demonstrated results (PDF p. 132).

## SEC-009: Appendix A. SIMTACS User's Guide

This historical manual explains selecting a digital/analog skeleton model,
coding a controller, running the two machines, debugging, and advanced changes
(PDF pp. 133-138). It presumes SIMSTAR and FORTRAN knowledge. It is not an
instruction to execute legacy commands in this repository.

The coding procedure converts transfer blocks to discrete equations, orders
them by data dependency, defines limiters algebraically, connects feedback and
actuator variables, initializes old values, and updates history only after
current calculations. `PADC`, `DCA`, wrapper arguments, and COMMON data must
agree with the intended analog/digital mapping (PDF pp. 139-148). Example
FORTRAN is illustrative source material; this guide does not certify its code
or OCR transcription as executable or error-free.

Runtime guidance starts lookup before the dynamics model and explains initial
conditions, communication interval `CINT`, digital period `PERIOD`, and simulation
time scaling `BET`. A typical `BET=20` means twenty wall-clock seconds per
simulated second. The worked sample-time arithmetic includes both lookup and
controller delay; nominal settings alone do not establish real-time operation
(PDF pp. 149-153).

Debugging distinguishes scaling overloads, incorrect port transfer, controller
faults, component capacity, and interrupt-rate overruns. Some machine-specific
startup errors are described as temporary, whereas a #115 controller interrupt
overrun invalidates a run. These are the author's legacy hardware observations,
not permission to suppress errors in current software (PDF pp. 154-158).

Analog-controller examples use derivative equations plus integration rather
than the digital difference equations (PDF pp. 159-160). Advanced work shows
how to expose 46 interpolated functions, gives three numerical cases, and
describes adding cockpit inputs/display to other model variants
(PDF pp. 161-165). Those tables have not been independently recalculated.
For example, PDF p. 162 visibly prints a total pitching-moment coefficient of
`-0.3347`; do not silently change it or treat it as a verified regression-test
oracle without reconciling its components and equations.

## SEC-010: Appendix B. Intermachine Cable Hookups

The appendix maps analog output/input blocks, physical port letters, variable
names, and grounds between #114, #115, the strip-chart recorder, pilot controls,
and the oscilloscope. State variables travel to lookup; computed forces/moments
return to dynamics. Separate X/Y and pitch signals drive the scope
(PDF pp. 166-169). These are hardware integration details, not a portable modern
software interface. Original port diagrams and lists must be checked before
any wiring interpretation.

## SEC-011: Appendix C. Pilot Control Schematics

The pilot input device is a Measurement Systems Model 435 DC two-axis force
controller. The text assigns its X axis to aileron input and Y axis to elevator
input and describes signal-generator excitation. Figures C.1-C.2 contain its
electrical/mechanical schematics (PDF pp. 170-172). The small schematic labels
and ratings are not verified or transcribed here; this guide is not an
electrical construction or safe-power-supply specification.

## SEC-012: Appendix D. Cockpit Graphics Display

This appendix records the attempted PS340 graphics path, including PS330
firmware mode, VAX-hosted display files, serial settings, logical-file assignment,
and routed graphics commands. It explains which machine-side output regions
can send data (PDF pp. 173-177).

The author could update graphics from a VAX program but did **not** achieve
SIMSTAR-driven graphics updates; formatting and update speed remained issues
(PDF p. 176). This is an incomplete development record, separate from the
working but crude oscilloscope display in Chapter V.

## SEC-013: Appendix E. Simulator Source Code

Despite its title, this appendix is a **program inventory**, not the complete
simulator source listing. It explicitly places bulky listings in a separate
document, bibliography reference 20. It identifies digital/analog dynamics
variants, lookup programs, macros, `FILELIST`, `AC.DAT`, and graphics files,
and records unresolved behavior in some model variants (PDF pp. 178-180).

The thesis includes selected code examples elsewhere, but it does not supply
the complete 18,816-value aerodynamic database or a complete runnable SIMTACS
package. Do not infer those files exist in this library from their names.

## SEC-014: Bibliography

The 22 references identify underlying textbooks, F-16/AFTI simulation material,
SIMSTAR manuals, a personal interview, and unpublished data/code. Especially
useful provenance leads are Ruley's 1987 simulation guide (14), the interview
used for response comparisons (19), separate Kassan program listings (20),
the F-16A OHT clean-data tape (21), and Block 25 DFCS diagrams (22)
(PDF pp. 181-182). These leads are not separately acquired or verified sources.

The footnote for reference 21 states that the data package is only
representative of the F-16A OHT and that ASD/ENFTC does not guarantee its
validity (PDF p. 182). Preserve this qualification alongside any database or
simulation-accuracy claim.

## SEC-015: Vita

The vita provides the author's educational/service background (PDF p. 183).
It supplies no additional aircraft-system evidence; personal contact details
are not reproduced in this guide.

## SEC-016: Report documentation and terminal blank page

The documentation form repeats the author, report number, December 1987 date,
unclassified status, public-release statement, and abstract (PDF pp. 184-185).
Its stated report page count is not the scan's PDF page count and must not
replace PDF locators. PDF p. 186 is blank.
