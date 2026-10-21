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
- Drafts may be opened before completion; explicitly identify unfinished work.
- Follow the project title convention. Describe the problem and expected
  outcome, actual changes, validation results, and known issues or unfinished work.
- Update the PR when later changes affect its description or validation results.
  Never claim that tests passed when they were not run.

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
