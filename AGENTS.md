# Project instructions

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
