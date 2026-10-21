# Development Change Validation

Validate one logical change, whether uncommitted or committed. This workflow
provides evidence about that change; it does not declare the whole branch complete.
The [PR workflow](pr-workflow.md) owns the
branch plan, cumulative PR checks, readiness and human handoff.

This procedure is shared by the Codex and Claude skill entry points. It does
not require a contributor's personal skills, prescribe commit frequency, or
authorize edits, commits, pushes, PR creation or external actions by itself.

## 1. Identify the change

- Use the requested scope, or infer the current logical change from the task
  and working tree when unambiguous. Ask only when ambiguity changes the result.
- For uncommitted work, identify its base and include the relevant staged,
  unstaged and new files. For a commit, compare with its parent; for a range,
  record both endpoints. For a merge commit, state the chosen comparison parent.
- Record the reviewed diff and actual tested revision or working-tree contents.
  A test on another revision is not evidence for the selected change unless its
  applicability is established. Preserve the working tree when inspecting history.
- Treat a feature branch as a coherent objective. Select by task and behavior,
  not by author; do not assume another contributor's work is unrelated.
- Examine affected callers, interfaces and data flows beyond changed lines.
  A small diff can require integration checks in unchanged components.

## 2. Define expected results

- Extract observable outcomes and behavior to preserve from the confirmed
  request, project contracts and relevant reference material.
- Identify expected results independently of the implementation. For numerical
  behavior, use justified values, units and tolerances rather than copying the
  algorithm into the test. State assumptions that affect the conclusion.
- Cover relevant boundaries, invalid inputs, state transitions and regressions;
  do not turn these into an indiscriminate checklist for every change.
- If acceptance cannot be determined, identify the missing requirement and
  continue checks that do not depend on it. Do not invent a specification.

## 3. Select verification methods

Use the existing [validation policy](../PR_VALIDATION.md),
[check selector](../../tools/pr-checks.mjs), [build guide](../BUILD_DLL.md)
and [package scripts](../../package.json) as the source of existing checks.
Apply their relevant rules to the selected change; the PR workflow still
evaluates the complete cumulative diff. Do not duplicate or weaken CI rules here.

Choose additional evidence by the intended behavior and affected dependencies:

| Concern | Methods to consider | Evidence boundary |
|---|---|---|
| Computation, conditions, state | Unit, boundary and regression tests | Proves covered behavior, not DCS integration |
| Known defect | Reproduction and regression test | The test must detect the original problem |
| Cross-module commands, IDs or parameters | Integration tests, generator and architecture checks | Structural checks alone do not prove runtime behavior |
| Initialization, reset, re-entry or release | Lifecycle and sequence tests | Check actual ordering where the host controls it |
| EFM runtime, DLL or build inputs | Applicable build, analysis, DLL and native checks | A build alone does not prove flight or cockpit behavior |
| DCS operation, animation, appearance or audio | Relevant [DCS scenarios](../DCS_TESTING.md) and logs | Requires the deployed version and real observation |
| Performance, memory or heavy IO | Reproducible measurements under comparable conditions | Report the workload and limits of comparison |
| Documents, skills or workflows | Requirement comparison, links/examples, scenario evaluation and artifact inspection | Format checks alone do not establish correct guidance |

- For each selected method, identify the outcome or failure it can detect.
  Reuse sufficient existing tests; add tests when they close a meaningful gap.
  Do not add low-value tests merely to mirror the implementation or increase counts.
- Choose change review where it adds evidence or is requested. Read the shared
  [Standards/Spec procedure](change-review.md); do not depend on a personal
  `code-review` installation. Review does not replace execution evidence.
- Verification needs do not depend on whether TDD was used. This workflow
  evaluates results; it does not choose the developer's implementation sequence.

## 4. Execute and assess evidence

- Run applicable checks against the identified contents. Record the commands or
  observed procedure, environment when relevant, actual outcome and evidence.
  Use the required hard 60-second timeout for backend unit-test runs; this is
  not a timeout for an entire build or static-analysis workflow.
- Check that test expectations follow the requirement, assertions detect the
  relevant failure, and the tested version matches the claimed result.
- For a regression, use available reproduction or pre-fix evidence to establish
  that the test detects the defect. Do not modify or discard current work merely
  to reconstruct history. If that comparison cannot be made, disclose the gap.
- If TDD was used, inspect available red/green/refactor evidence: the original
  failure must be relevant, the corrected version must pass, and subsequent
  refactoring must preserve the expected behavior. Missing history limits the
  TDD-process claim; it does not by itself invalidate the final result.
- Inspect the actual deliverable where applicable: generated output, installed
  DLL, rendered content or remote PR description/state. Access to external
  systems still follows task authorization. Distinguish a scenario evaluation
  from a live execution, especially for prohibited merge or approval actions.
- Diagnose failures and revise only within task authorization. Do not remove
  tests, hide errors, relax expectations or replace real execution with fake
  success. Report unavailable tools or environments explicitly.
- After changes, rerun affected checks when their evidence is invalidated.
  Broaden checks for newly identified effects, not simply to repeat a completed run.

## 5. Report the change result

Keep the report proportional to the change. Include:

- Scope: baseline, target or identified working-tree contents, and relevant files.
- Expected outcomes and their evidence: method, result and reproducible reference.
- Review findings under separate Standards and Spec headings when review ran;
  disclose missing specification or reviewer-independence limitations.
- Distinguish passed, failed, running, not run, not applicable and inconclusive.
  Missing infrastructure means not run, not not applicable. Never present a
  skipped CI step or successful wrapper as a test that actually ran.
- Known defects, evidence gaps and work remaining for this change; write `None`
  where none are known. Qualify conclusions that cannot yet be established.

Do not turn a passing change result into branch completion, PR approval or
permission to merge. Pass the evidence to the PR workflow, which retains all
confirmed branch plans, including work not started or still being designed.
