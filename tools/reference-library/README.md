# Reference-library tooling boundary

This directory owns automation for the
[reference library](../../docs/reference-library/README.md). There are no
executable library commands, SQLite database, extractor, OCR pipeline, or staging
guard yet; the instructions, grading definitions, and record schemas support
manual cataloging and reading independently of collection size.

Use the documented manual workflow. Do not report that registration, indexing,
full-text search, or rights validation has run through a nonexistent command.

Future tooling belongs here and will operate on the authoritative files under
`docs/reference-library/`. It should provide:

- Registration and duplicate/version detection without inventing bibliography.
- Record/reference validation and an explicit staged-content rights check.
- Page-preserving text extraction, selected figure rendering, and transparent
  handling of extraction errors, partial content, and unavailable local assets.
- A rebuildable relational/full-text index with input provenance and freshness.
- Small structured search results, expandable content, and relevant figure paths.

The runtime, packages, CLI syntax, and any MCP adapter remain future choices.
Choose them when implementing executable behavior and verify against real,
permitted sources within the authorized task. Do not create fabricated source
records or successful search results to demonstrate an unimplemented capability.
