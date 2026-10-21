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
