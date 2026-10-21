---
name: project-pr-workflow
description: Create, update and validate F-CK-1C pull requests, automatically marking eligible drafts ready for review while reserving human review, resolution of human review threads and merging for humans.
---

# Project PR Workflow

## Scope and authorization

Handle PR creation, updates, automated validation, readiness and review revisions.
Follow the [contribution guide](../../../CONTRIBUTING.md) and
[PR validation policy](../../../docs/PR_VALIDATION.md). Do not impose commit
frequency or a particular collaboration style within a branch.
Authorization to commit, push or create a PR comes from the current task;
loading this skill does not grant that authorization.

## Human responsibilities

- Every PR must receive human review and may only be merged by a human.
- Support solo maintenance: the author may personally review and merge the PR.
  Do not require a second reviewer or a formal Approve review.
- AI may provide review analysis but must not submit formal approval on a
  human's behalf or treat AI review or passing CI as completed human review.
- Only humans may close or resolve human review comments and threads.
  AI must not perform those actions.
- AI must not merge through the UI, CLI, API or any other mechanism, enable
  auto-merge, or enqueue a PR in a queue that merges automatically.
- AI must not bypass the PR by pushing its changes directly to the target
  branch, or bypass review by changing protection settings, disabling required
  checks or weakening acceptance criteria.

## Create and update PRs

- Verify that the source branch, target branch and full diff match the task.
  Daily work targets `dev`; release PRs go from `dev` to `main`.
- Check for an existing PR for the same work before creating one. Update it
  rather than creating a duplicate.
- Always create new PRs as Draft when acting as AI, even when the work is complete.
- Drafts may be opened before completion. Every PR description must include
  a `Remaining work and Draft status` section using the rules below.
- Follow the project title convention. Describe the problem and expected
  outcome, actual changes, validation results, and known issues or unfinished work.
- Update the PR when later changes affect its plan, description or validation results.
  Never claim that tests passed when they were not run.

## Branch plan and remaining work

- Derive the branch plan from confirmed user instructions, accepted plans and
  subsequent scope decisions, as well as the actual diff. Remaining work includes
  planned work that has not started, work still being designed, partial
  implementation and outstanding validation. It is not limited to changes
  already present in files.
- Include confirmed work planned for this branch even when its design is still
  under discussion. Distinguish undecided proposals from accepted deliverables;
  do not silently promote every suggested idea into a commitment.
- Do not remove planned work from scope or move it to another branch merely
  because the current files are complete. Record explicit scope changes and
  any agreed destination or deferral.
- In `Remaining work and Draft status`, list remaining planned work with its
  status, validation gaps, pending human review actions, and the reason for
  remaining Draft, when applicable. Explicitly state `None` for categories with
  no outstanding items. Separate implementation completion from validation
  completion and pending human actions.
- Keep Draft while confirmed work planned for this branch remains incomplete.
  Do not infer branch completion solely from passing CI or the current diff.
- After creating or updating a PR, read back its actual remote description,
  refs and Draft/Ready state. Check them against the confirmed branch plan,
  validation evidence and requested state; correct omissions before reporting
  the PR as prepared.

## Automated validation

- Select checks using the project's existing change-scope rules and the full
  cumulative PR diff.
- Run applicable tests and inspect required CI. Verify that results apply to
  the latest source commit and the target revision required by project policy.
- Distinguish passed, failed, running and not run. Do not report checks that
  were not run or are not applicable as passed tests.
- Diagnose failures, fix them within task authorization and revalidate.
  Rerun affected checks when new changes invalidate previous results.
- If validation cannot be completed, report the reason and available evidence.
  Do not delete tests, hide errors or weaken assertions to obtain passing results.

## Automatically mark ready for review

AI may convert a Draft to Ready for review without additional confirmation
when all of the following hold:

- The work is ready for review and the PR description matches the actual changes.
- Applicable automated validation and required CI have passed, and their results
  apply to the current revisions.
- All confirmed deliverables planned for this branch are complete, including
  work that was initially unstarted or under design; any scope changes are explicit.
- No known blocking defects remain to be fixed, and unfinished work is not hidden.

Otherwise, keep the PR as Draft and explain what remains outstanding.
Ready for review means available for human review; it does not mean human
approval has been given or automated merging is permitted.

## Address human review

Within task authorization, implement requested revisions, push, revalidate and
reply with the changes and supporting evidence.
Leave human comments and threads for humans to close or resolve. Do not claim
that a human has accepted a revision on their behalf.
If new commits follow human approval, report the new diff and validation results.
Do not claim that earlier approval covers the new revision; approval validity
is determined by the actual remote protection rules.

## Handoff

Report the PR link, Draft or Ready status, validated commit, required CI results,
known issues and unresolved human review items. Distinguish preparation complete,
awaiting human review and awaiting human merge.
Do not claim that GitHub enforces human review or merge restrictions unless
its actual remote protection settings have been verified.
