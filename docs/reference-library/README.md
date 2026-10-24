# Aviation Reference Library

This library gives developers and AI agents a shared place to find external
aviation evidence during design, discussion, and programming. It includes
F-CK-1C material, related aircraft research, subsystem documents, and useful
background research. Preserve the actual subject and conditions of every source;
research on another aircraft does not establish facts about the F-CK-1C.

Project configuration, simulation tuning, implementation status, and project
decisions belong in their existing project documents.

## Current capabilities

This library provides structure, record contracts, grading definitions, and
operating instructions. Discover publications through
`sources/<source-id>/source.yaml` and topics through `knowledge/<topic-id>.md`.
The directory READMEs explain how to use and add material; they are not source
or topic catalogs and do not track collection counts or ingestion milestones.

Manual cataloging, reading, and repository text search can use these contracts.
Library-specific registration, validation, PDF extraction, OCR, SQLite search,
and staged-rights checks are not implemented. Do not claim that those operations
have run or use the planned database as an existing search service.

## Start here

- Agents: read [AGENTS.md](AGENTS.md) before using or contributing material.
- Readers: search [topic knowledge](knowledge/README.md), then consult the
  relevant source guide or original content when more context is needed.
- Contributors: follow [source handling](sources/README.md),
  [formats](FORMATS.md), and [rights handling](RIGHTS.md).
- Grades: use the shared definitions in [ratings.yaml](ratings.yaml).
- Design context: see [the design document](../REFERENCE_LIBRARY_DESIGN.md).

## File responsibilities

| Location | Responsibility |
|---|---|
| `AGENTS.md` | Agent retrieval, contribution, citation, and figure-display rules |
| `RIGHTS.md` | Acquisition, reuse, AI processing, and Git-storage procedure |
| `FORMATS.md`, `schemas/` | Record formats and structural contracts |
| `ratings.yaml` | Independent fixed grading scales |
| `sources/<source-id>/source.yaml` | Bibliography, versions, assets, rights, and topic relationships |
| `sources/<source-id>/guide-v*.md` | Substantive condensation of a particular document version |
| `sources/<source-id>/figures/` | Selected source figures with curated provenance |
| `knowledge/<topic-id>.md` | Canonical topic definition and directly reusable, cited knowledge |
| `local/` within a source or `knowledge/` | Local-only content, ignored by Git |
| `generated/` | Rebuildable indexes, automatic extraction, and OCR; ignored by Git |

Optional asset directories are created only when they contain real material.
Source packages are organized by stable source ID, not by publisher or topic.
One source can support multiple independently assessed topics without copying
its PDF into each topic.

Source records, guides, topic files, retained assets, and shared contracts are
authoritative inputs. A future SQLite database will be rebuilt from them.
There is no independently edited database catalog or separate manual topic
list: topic definitions live in their canonical topic files.

## Read and contribute manually

Start with the smallest useful topic entry or guide section. Check its scope,
review, and evidence links before reuse. Search available source text or inspect
original pages when coverage is incomplete, conditions are missing, or a table,
equation, or diagram needs verification. Absence from a guide is not evidence
that the original document lacks relevant content.

When new external material materially informs work and has continuing value,
check for an existing source/version, record its bibliography and both link
types, assess actual rights, and preserve a PDF when permitted. Add a useful
basic guide for content actually read and valuable topic knowledge as needed.
Follow the detailed manual workflow in [AGENTS.md](AGENTS.md); automated checks
remain future work.

Ordinary ingestion changes the affected source packages and topic records, not
these shared instructions or a hand-maintained publication list. Update consuming
links if assets move, and refresh affected derived indexes when implemented
tooling supports them. Change shared documentation only when responsibilities,
contracts, procedures, or actual tool capabilities change, not when the collection
becomes empty or gains another source.

All authored library content uses English. Preserve original publications,
titles, names, images, and quotations in their original form, and identify
translations. Developer conversations may use the developer's language.
