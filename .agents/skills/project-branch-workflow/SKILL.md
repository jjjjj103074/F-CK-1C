---
name: project-branch-workflow
description: Choose, create or reuse a work branch in the F-CK-1C repository according to its branch naming and single-objective rules.
---

# Project branch workflow

Read the canonical [branch policy](../../../CONTRIBUTING.md#branch-naming-and-scope)
and [project instructions](../../../AGENTS.md#branch-naming-and-scope) before
choosing, creating or reusing a work branch.

- Identify the single objective: a feature, system change, bug fix or issue.
  Keep directly required code, tests, documentation and assets together;
  separate independent objectives even if they affect the same system.
- Inspect the current branch and working tree before switching or creating a
  branch. Preserve existing work and branch names. Do not rename or delete
  existing branches to apply this policy.
- For a new work branch, select exactly one of `feature/`, `bugfix/`,
  `refactor/`, `docs/`, or `chore/`. Use `<type>/<module>-<task>` with a lowercase
  English, hyphen-separated description; optionally include an issue number
  before the module. Never use `codex/`, `claud/`, `claude/`, or another
  agent-name prefix.
- Follow the existing integration flow: daily work starts from `dev` and its
  PR targets `dev`; release PRs go from `dev` to `main`. Keep `dev` and `main`.
- Reuse an existing work branch only when continuing its unfinished objective.
  For a different objective or a completed branch, use a separate work branch.

This skill defines branch selection and scope; it does not authorize pushing,
merging, deleting branches or discarding changes.
