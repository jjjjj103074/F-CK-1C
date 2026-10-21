# Project instructions

## Branch naming and scope

New work branches must use one of `feature/`, `bugfix/`, `refactor/`, `docs/`,
or `chore/`, followed by a lowercase English, hyphen-separated module and task
description: `<type>/<module>-<task>`. An issue number may precede the module,
for example `bugfix/123-efm-engine-start`.

Do not use `codex/`, `claud/`, `claude/`, or other agent-name prefixes for new
branches. Preserve existing branch names; do not rename or delete them as part
of adopting this convention. Keep the established `dev` and `main` branches.

Each branch represents one coherent unit of work: one feature, one system
change, one bug fix, or one issue. Include directly required code, tests,
documentation and assets together; split independently reviewable objectives
into separate branches. Do not accumulate unrelated changes under a broad
system name or reuse a completed branch for another task.

Follow [CONTRIBUTING.md](CONTRIBUTING.md#branch-naming-and-scope) for the type
definitions and examples. The repository skill
[project-branch-workflow](.agents/skills/project-branch-workflow/SKILL.md)
applies when choosing, creating or reusing a work branch.

## PR review and merge

Follow [project-pr-workflow](.agents/skills/project-pr-workflow/SKILL.md) when
creating, updating, validating or preparing a PR for review.
AI must create new PRs as Draft. It may automatically mark them Ready for
review when the skill's readiness conditions are met; Ready is not human approval.
Every PR requires human review and a human-performed merge. Solo maintainers
may personally review and merge; a second reviewer or formal Approve is not required.
Only humans may close or resolve human review comments and threads.
AI must not submit approval on a human's behalf, merge by any mechanism,
enable auto-merge, or enqueue automatic merging. Do not bypass PR review through
direct target-branch pushes, protection changes or weakened required checks.
Commit frequency and collaboration within a branch remain task-specific;
authorization to commit, push or create a PR comes from the current task.
See [CONTRIBUTING.md](CONTRIBUTING.md#pr-review-and-merge) for the shared policy.

## C++ formatting

When editing C++, read [the formatting guide](docs/CPP_FORMATTING.md).
After editing files in the active `cppFormat.roots` scope in `package.json`,
run `npm run format:cpp` on the working tree before staging, then run
`npm run check:cpp-format`. This scope includes authored EFM sources and tests.
Preserve SDK, third-party, and generated files through their owning upstream
source or generator.

## Semble Code Search

Use `mcp__semble__search` to locate implementations, then read the returned file
and line directly. Set `content` to `docs`, `config`, or `all` for non-code searches.
Use literal repository-wide search only when every occurrence is needed.
CLI fallback: `semble search "query" . --max-snippet-lines 10`; if unavailable,
use `uvx --from "semble[mcp]==0.6.1" semble`.
