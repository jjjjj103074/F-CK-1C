---
schema_version: 1
record_type: topic
topic_id: flight-simulation-validation
title: Flight Simulation Validation
scope: >-
  Evidence, comparison conditions, timing performance, reproducibility, and known
  limitations used to evaluate aircraft simulations. Agreement for one model or
  test condition is not assumed to establish another aircraft's validity.
related_topics: [control-law-design]
updated_at: '2026-10-24'
---

# Flight Simulation Validation

These entries concern the reported evaluation of Kassan's 1987 SIMTACS F-16A
simulation. They are not certification criteria or F-CK-1C settings. See
[control law design](control-law-design.md) for controller assumptions and
[the source guide](../sources/SRC-000001/guide-v001.md) for chapter context.
Grades remain in the [source record](../sources/SRC-000001/source.yaml).

## KNO-000004: The reported response comparisons have a specific trim condition and limited provenance
```yaml
kind: finding
review:
  state: cross_checked
  scope: Chapter VI's stated comparison method, operating condition, selected findings, plotting warning, and bibliography reference 19; no independent validation of the reference responses.
  method: Read original PDF pages 89-129 and 182; visually checked Table III on PDF page 94. Did not digitize strip-chart curves or inspect the external comparison data.
  checked_by: Codex
  checked_at: '2026-10-24'
publication:
  git_storage:
    status: allowed
    basis: &prose_basis
      - kind: statutory
        ref: Taiwan Copyright Act, Article 10-1
        url: https://law.moj.gov.tw/LawClass/LawAll.aspx?PCode=J0070017
        scope: Test conditions, reported findings, and provenance expressed in newly authored prose, not copied source expression or reproduced charts.
      - kind: license
        ref: Repository LICENSE, MIT documentation terms
        url: null
        scope: Newly authored explanatory prose only; no relicensing of the underlying thesis or figures.
    conditions: [Retain source attribution and the distinction between reported and independently verified results.]
    reason: Publishes a factual synthesis with explicit evidence boundaries, rather than the original prose or chart images.
    assessed_by: Codex
    assessed_at: '2026-10-24'
  ai_processing:
    status: allowed
    basis: *prose_basis
    conditions: [Preserve the reported comparison's actual condition and provenance limitations.]
    reason: Agent reading and processing of this newly authored factual explanation; source assets retain their separate rights assessments.
    assessed_by: Codex
    assessed_at: '2026-10-24'
evidence:
  - source_id: SRC-000001
    version_id: v001
    relation_id: REL-003
    asset_id: PDF-001
    locator: {type: pdf, asset_id: PDF-001, page_start: 89, page_end: 129, section: Simulator Testing and Verification}
  - source_id: SRC-000001
    version_id: v001
    relation_id: REL-005
    asset_id: PDF-001
    locator: {type: pdf, asset_id: PDF-001, page_start: 182, page_end: 182, section: Bibliography reference 19}
```

Chapter VI compares the simulation's closed-loop time responses with reference
responses and checks whether the behavior is physically plausible. The stated
initial operating point is Mach 0.9, 20,000 ft, one-g, wings-level,
non-accelerating flight. Table III specifies a weight of 23,555 lb, angle of
attack and pitch angle of 1.9 degrees, zero sideslip, horizontal-tail deflection
of -2.8 degrees, and zero leading-edge-flap deflection, together with the model's
inertias and CG. This is a defined configuration, not an unspecified generic F-16.

The chapter includes trimmed/no-input, pitch, roll, and rudder responses for
inertially cross-coupled and uncoupled models. Selected reported results include:

- The no-input coupled run reaches the intended trim in approximately two seconds
  (PDF p. 94).
- A commanded 45 deg/s roll settles at about 32 deg/s in the coupled example; the
  author attributes the tracking error to the roll controller's type-zero
  structure (PDF p. 101).
- The uncoupled roll example instead commands 5 deg/s, so those coupled/uncoupled
  roll runs are not an identical-input experiment (PDF p. 128).
- Rudder-pedal-force traces include a reversed-polarity plotting error identified
  by the author (PDF pp. 106, 128-129). The plotted sign must not silently become
  a control-input sign convention.

The comparison source cited as reference 19 is a 25 November 1987 personal
interview with Finley Barfield, identified as an F-16/AFTI-F-16 engineer (PDF
p. 182). The thesis's description of reference or true-aircraft responses does
not, by itself, supply an independently inspectable flight-test dataset. These
results therefore remain the author's reported model validation at the stated
condition, not demonstrated whole-envelope accuracy or F-CK-1C validation.

Original conditions: [Table III, PDF p. 94](../sources/SRC-000001/figures/v001/table-3-p094.png).
Source: SRC-000001 / v001 / PDF-001, PDF pp. 89-129 and 182.

## KNO-000005: A functioning closed-loop cockpit did not establish real-time man-in-the-loop performance
```yaml
kind: limitation
review:
  state: cross_checked
  scope: Reported execution times, graphics-update limitation, and the author's conclusion about man-in-the-loop evaluation.
  method: Read the original Chapter II discussion on PDF pages 26-36 and compared it with the explicit simulator-performance statement on PDF page 129.
  checked_by: Codex
  checked_at: '2026-10-24'
publication:
  git_storage:
    status: allowed
    basis: &prose_basis
      - kind: statutory
        ref: Taiwan Copyright Act, Article 10-1
        url: https://law.moj.gov.tw/LawClass/LawAll.aspx?PCode=J0070017
        scope: Reported timing facts and evaluation limitations expressed in newly authored prose, not copied source expression.
      - kind: license
        ref: Repository LICENSE, MIT documentation terms
        url: null
        scope: Newly authored explanatory prose only; no relicensing of the underlying thesis.
    conditions: [Retain source attribution and the original hardware context.]
    reason: Publishes a factual explanation of the documented execution limitation, not the original expressive text.
    assessed_by: Codex
    assessed_at: '2026-10-24'
  ai_processing:
    status: allowed
    basis: *prose_basis
    conditions: [Do not reinterpret the thesis's timing target as an F-CK-1C or present-day simulator requirement.]
    reason: Agent reading and processing of this newly authored factual explanation; source assets retain separate rights assessments.
    assessed_by: Codex
    assessed_at: '2026-10-24'
evidence:
  - source_id: SRC-000001
    version_id: v001
    relation_id: REL-004
    asset_id: PDF-001
    locator: {type: pdf, asset_id: PDF-001, page_start: 26, page_end: 36, section: Simulation Specifications}
  - source_id: SRC-000001
    version_id: v001
    relation_id: REL-003
    asset_id: PDF-001
    locator: {type: pdf, asset_id: PDF-001, page_start: 129, page_end: 129, section: Simulator Performance}
```

The intended system is a real-time, fixed-base simulator with pilot input and
cockpit display. In the actual two-SIMSTAR configuration, the author reports an
approximately 15 ms desired cycle through all code, while the aerodynamic
interpolation/look-up work on SIMSTAR #114 alone takes about 125 ms and the
controller code on #115 takes about 20 ms (PDF p. 34). Those are reported
subsystem timings, not a demonstrated end-to-end real-time cycle or a portable
computational requirement.

The planned graphics display is also incomplete: the available execution region
permits updates only about seven times per ten seconds, which the author finds
unacceptable for man-in-the-loop operation (PDF pp. 34-35). The implemented
cockpit instead uses a force stick and an oscilloscope showing pitch and roll.

On PDF p. 129 the author explicitly states that the simulator cannot run in real
time and that this prevents evaluating its man-in-the-loop performance. The
cockpit demonstrates closure of the pilot/simulation loop, but the large time
scaling and lag remain. Accordingly, the completed interface and plausible
aircraft response plots do not establish usable real-time pilot handling.

Source: SRC-000001 / v001 / PDF-001, PDF pp. 26-36 and 129. The timings describe
the thesis hardware, not production aircraft computers or F-CK-1C requirements.

## KNO-000006: The thesis alone is not a self-contained executable or aerodynamic-data baseline
```yaml
kind: limitation
review:
  state: cross_checked
  scope: Appendix E's external source-code and data dependencies and the bibliography's explicit aerodynamic-data validity warning.
  method: Read original PDF pages 178-182 and visually checked the bibliography entries and footnote on PDF page 182; did not obtain or validate the separate code or magnetic-tape data.
  checked_by: Codex
  checked_at: '2026-10-24'
publication:
  git_storage:
    status: allowed
    basis: &prose_basis
      - kind: statutory
        ref: Taiwan Copyright Act, Article 10-1
        url: https://law.moj.gov.tw/LawClass/LawAll.aspx?PCode=J0070017
        scope: Factual dependency names, bibliography provenance, and limitations expressed in newly authored prose, not copied code or source expression.
      - kind: license
        ref: Repository LICENSE, MIT documentation terms
        url: null
        scope: Newly authored explanatory prose only; no relicensing of the thesis, external code, or aerodynamic database.
    conditions: [Retain source attribution and do not treat the cited external works as independently checked.]
    reason: Publishes factual dependency information and an original explanation without reproducing external programs or database contents.
    assessed_by: Codex
    assessed_at: '2026-10-24'
  ai_processing:
    status: allowed
    basis: *prose_basis
    conditions: [Preserve missing-data and verification limitations.]
    reason: Agent reading and processing of this newly authored factual explanation; external programs and data require separate assessment.
    assessed_by: Codex
    assessed_at: '2026-10-24'
evidence:
  - source_id: SRC-000001
    version_id: v001
    relation_id: REL-005
    asset_id: PDF-001
    locator: {type: pdf, asset_id: PDF-001, page_start: 178, page_end: 182, section: Appendix E and Bibliography}
```

Appendix E inventories programs rather than including their listings. It states
that the actual simulator code is in a separate document, bibliography reference
20, *SIMTACS Program Source Code*, an unpublished December 1987 listing.

The aerodynamic look-up programs depend on `FILELIST` and `AC.DAT`; Appendix E
describes `AC.DAT` as the file containing the F-16A aerodynamic values (PDF
pp. 179-180). Bibliography reference 21 identifies the underlying F-16A OHT
"CLEAN" aerodynamic data as obtained on magnetic tape from ASD/ENFTC. The
bibliography explicitly says that the package is representative of the F-16A OHT
and that ASD/ENFTC does not guarantee its validity (PDF p. 182).

Consequently, the retained thesis supplies descriptions, equations, diagrams,
and example responses, but not the separate program listings or the complete
machine-readable aerodynamic database. Their titles in this bibliography are
research leads, not proof that those external assets are present, checked, or
licensed for reuse. Reconstructing missing values from plots would be a new,
uncertain derivation, not recovery of the original data.

Source: SRC-000001 / v001 / PDF-001, PDF pp. 178-182. No source-code execution
or independent aerodynamic-data validation was performed for this entry.
