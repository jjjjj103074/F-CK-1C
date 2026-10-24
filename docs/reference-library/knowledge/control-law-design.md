---
schema_version: 1
record_type: topic
topic_id: control-law-design
title: Control Law Design
scope: >-
  Feedback structures, command paths, scheduling, actuator representations, and
  explicit assumptions in aircraft flight-control laws. Source aircraft and
  operating conditions remain distinct from F-CK-1C implementation decisions.
related_topics: [flight-simulation-validation]
updated_at: '2026-10-24'
---

# Control Law Design

These entries concern Kassan's 1987 SIMTACS thesis, not an F-CK-1C control law.
See [the source guide](../sources/SRC-000001/guide-v001.md) for document context
and [simulation validation](flight-simulation-validation.md) for the reported
test conditions and unresolved limitations. Grades are resolved from the
[source record](../sources/SRC-000001/source.yaml), not repeated here.

## KNO-000001: Distinguish the reported Block 25 reference from the thesis controller
```yaml
kind: explanation
review:
  state: cross_checked
  scope: Aircraft identity, reference-controller provenance, and feedback structures in the cited chapter prose; no complete diagram transcription.
  method: Read extracted original PDF pages 37-39 and visually located the paired Figure 3.1-3.5 scan pages 40-49; did not transcribe or verify all diagram values.
  checked_by: Codex
  checked_at: '2026-10-24'
publication:
  git_storage:
    status: allowed
    basis: &prose_basis
      - kind: statutory
        ref: Taiwan Copyright Act, Article 10-1
        url: https://law.moj.gov.tw/LawClass/LawAll.aspx?PCode=J0070017
        scope: Factual provenance and engineering ideas expressed in newly authored prose, not copied source expression or reproduced diagrams.
      - kind: license
        ref: Repository LICENSE, MIT documentation terms
        url: null
        scope: Newly authored explanatory prose only; no relicensing of the underlying thesis or figures.
    conditions: [Retain source attribution and distinguish third-party material.]
    reason: Publishes an original factual explanation with citations, not the source's expressive text or images.
    assessed_by: Codex
    assessed_at: '2026-10-24'
  ai_processing:
    status: allowed
    basis: *prose_basis
    conditions: [Preserve scope and provenance when reusing this explanation.]
    reason: Agent reading and processing of this newly authored factual explanation; source assets retain their separate rights assessments.
    assessed_by: Codex
    assessed_at: '2026-10-24'
evidence:
  - source_id: SRC-000001
    version_id: v001
    relation_id: REL-001
    asset_id: PDF-001
    locator: {type: pdf, asset_id: PDF-001, page_start: 37, page_end: 49, section: Aircraft and Flight Control System Description}
```

The simulated airframe is an F-16A with the original horizontal tail and wingtip
AIM-9 missiles. The controller reference is a different identification: Block 25
F-16 digital-flight-control functional diagrams obtained through the F-16 System
Program Office. The thesis implements a simplified controller derived from that
reference. Those labels must not be collapsed into a claim that the thesis models
an unmodified production Block 25 aircraft/control-system combination.

The chapter describes longitudinal feedback of pitch rate and normal
acceleration, a proportional-plus-integral forward controller, and angle-of-attack
feedback for static stability. The lateral-directional description uses roll-rate
feedback to the flaperon and differential horizontal-tail paths. These are
source-reported structural descriptions, not a complete executable control law.

The fuller reference diagrams and gain schedules are separate from the thesis
diagrams. For example, Figure 3.1 spans [PDF p. 40](../sources/SRC-000001/figures/v001/figure-3-1-p040.png)
and [PDF p. 41](../sources/SRC-000001/figures/v001/figure-3-1-p041.png); Figure 3.2
spans [PDF p. 42](../sources/SRC-000001/figures/v001/figure-3-2-p042.png) and
[PDF p. 43](../sources/SRC-000001/figures/v001/figure-3-2-p043.png).
The split scan pages are complementary, not duplicate figures. Their small
labels must be inspected before transcribing a gain, connection, or limiter.

The fuller lateral-directional reference is also saved: Figure 3.3 uses
[PDF p. 44](../sources/SRC-000001/figures/v001/figure-3-3-p044.png) and
[PDF p. 45](../sources/SRC-000001/figures/v001/figure-3-3-p045.png); its gain
functions in Figure 3.4 use
[PDF p. 46](../sources/SRC-000001/figures/v001/figure-3-4-p046.png) and
[PDF p. 47](../sources/SRC-000001/figures/v001/figure-3-4-p047.png). Figure 3.5's
control-surface mixer uses
[PDF p. 48](../sources/SRC-000001/figures/v001/figure-3-5-p048.png) and
[PDF p. 49](../sources/SRC-000001/figures/v001/figure-3-5-p049.png).
Read both pages of each pair; these are the fuller reference, not the simplified
research-controller diagrams or independently verified F-CK-1C control laws.

Source: SRC-000001 / v001 / PDF-001, PDF pp. 37-49. No F-CK-1C architecture,
gain, or performance is established by this source.

## KNO-000002: The simplified controller deliberately omits protections and modes
```yaml
kind: limitation
review:
  state: cross_checked
  scope: The ten listed simplifications and their operating assumptions; no verification of every block-diagram coefficient.
  method: Read original PDF pages 50-55 and visually checked the enumerated assumptions on PDF pages 50 and 53.
  checked_by: Codex
  checked_at: '2026-10-24'
publication:
  git_storage:
    status: allowed
    basis: &prose_basis
      - kind: statutory
        ref: Taiwan Copyright Act, Article 10-1
        url: https://law.moj.gov.tw/LawClass/LawAll.aspx?PCode=J0070017
        scope: Factual model assumptions expressed in newly authored prose, not copied source expression or reproduced diagrams.
      - kind: license
        ref: Repository LICENSE, MIT documentation terms
        url: null
        scope: Newly authored explanatory prose only; no relicensing of the underlying thesis or figures.
    conditions: [Retain source attribution and distinguish third-party material.]
    reason: Publishes a factual synthesis of the author's declared model choices, not the original page images or prose.
    assessed_by: Codex
    assessed_at: '2026-10-24'
  ai_processing:
    status: allowed
    basis: *prose_basis
    conditions: [Preserve the declared assumptions rather than converting them into aircraft specifications.]
    reason: Agent reading and processing of this newly authored factual explanation; source assets retain separate rights assessments.
    assessed_by: Codex
    assessed_at: '2026-10-24'
evidence:
  - source_id: SRC-000001
    version_id: v001
    relation_id: REL-002
    asset_id: PDF-001
    locator: {type: pdf, asset_id: PDF-001, page_start: 50, page_end: 55, section: Simplified digital flight control system}
```

The author's simplified DFCS is centered on straight-and-level flight at Mach
0.9 and 20,000 ft. Its ten declared simplifications/assumptions are:

1. Gains are selected for that single flight condition rather than retaining the
   full gain schedules.
2. Mechanical/physical limiters are removed and anti-windup limiting is omitted;
   the author assumes aircraft rates will not become excessive.
3. Analog demodulator dynamics are omitted under a low-signal-frequency assumption.
4. Landing gear and flaps are not extended, and aerial-refueling mode is excluded.
5. Servoactuators become first-order lags with a 20 rad/s pole, representing the
   lowest-frequency actuator filter rather than the full actuator dynamics.
6. Lateral-directional gun compensation is omitted.
7. Yaw structural and roll filters are omitted because the corresponding
   high-frequency vibration is assumed insignificant.
8. Leading-edge flap deflection is held at zero throughout the simulation.
9. Automatic angle-of-attack limiting loops are removed.
10. The author's selected pitch, roll, and rudder command gradients are those in
    Figures 3.8-3.10, PDF pp. 54-55.

The actual simplified diagrams are [Figure 3.6, PDF p. 51](../sources/SRC-000001/figures/v001/figure-3-6-p051.png)
and [Figure 3.7, PDF p. 52](../sources/SRC-000001/figures/v001/figure-3-7-p052.png).
These omissions describe this research controller, not proof that the operational
F-16 lacks those protections. The author's favorable assessment near the chosen
flight condition on PDF p. 55 is not demonstrated full-envelope validity and is
not evidence about an F-CK-1C controller.

Source: SRC-000001 / v001 / PDF-001, PDF pp. 50-55.

## KNO-000003: Table I contains fixed gains for the thesis's single-point controller
```yaml
kind: data
review:
  state: cross_checked
  scope: Nine gain labels and numeric values in Table I, with the stated Mach and altitude; not their complete units or signal-path interpretation.
  method: Compared extracted PDF page 50 with its rendered original table and checked all nine label/value pairs visually.
  checked_by: Codex
  checked_at: '2026-10-24'
publication:
  git_storage:
    status: allowed
    basis: &prose_basis
      - kind: statutory
        ref: Taiwan Copyright Act, Article 10-1
        url: https://law.moj.gov.tw/LawClass/LawAll.aspx?PCode=J0070017
        scope: Factual numerical label/value pairs and independently written explanatory prose; the source table image is a separately assessed asset.
      - kind: license
        ref: Repository LICENSE, MIT documentation terms
        url: null
        scope: Newly authored explanatory prose only; no relicensing of the source's expressive material.
    conditions: [Retain source and operating-condition attribution.]
    reason: Records factual numeric values in a new layout with explicit provenance and limitations.
    assessed_by: Codex
    assessed_at: '2026-10-24'
  ai_processing:
    status: allowed
    basis: *prose_basis
    conditions: [Preserve the distinction between table transcription and implementation-ready gain definitions.]
    reason: Agent reading and processing of this factual transcription and original explanation; source images retain separate rights assessments.
    assessed_by: Codex
    assessed_at: '2026-10-24'
evidence:
  - source_id: SRC-000001
    version_id: v001
    relation_id: REL-002
    asset_id: PDF-001
    locator: {type: pdf, asset_id: PDF-001, page_start: 50, page_end: 50, table: I}
```

For the simplified DFCS at Mach 0.9 and 20,000 ft, Table I reports:

| Gain label | Value |
|---|---:|
| N2 | 0.38 |
| N3 | 0.70 |
| N5 | 10.0 |
| N8 | 14.4 |
| N14 | 7.2 |
| N23 | 0.5 |
| N24 | 0.67 |
| N25 | 2.5 |
| N30 | 20.0 |

The table does not specify each gain's complete input/output unit convention.
The labels require their corresponding signal paths in the diagrams; these
numbers alone are not an implementation-ready controller, a full gain schedule,
or transferable F-CK-1C parameters. The model omissions in KNO-000002 apply.

Original: [Table I, PDF p. 50](../sources/SRC-000001/figures/v001/table-1-p050.png).
Source: SRC-000001 / v001 / PDF-001, PDF p. 50.
