# Record Formats

This is the serialization guide for manual cataloging. The exact structural
contracts are in [schemas/](schemas/); the grading vocabulary is in
[ratings.yaml](ratings.yaml). No library parser or validation command exists yet.
The examples below illustrate syntax only: they are not registered publications,
topics, evidence, permission decisions, or claims that any review occurred.

## Common conventions

Use UTF-8 YAML for `source.yaml` and YAML metadata in Markdown. Use the repository
line-ending rules. Do not use duplicate YAML keys or custom YAML tags. Dates are
quoted `YYYY-MM-DD` strings, not YAML-native date objects. Use `null` for unknown
nullable values and explain significant unknowns in the corresponding note,
scope, or reason. An empty array means no items recorded, not that a search
proved none exist. Never invent dates, authors, URLs, grades, or review evidence.

Paths in asset records are relative to `sources/<source-id>/`, including paths
starting with `local/`. Use forward slashes; never absolute paths, `..`, `.`
segments, backslashes, or credentials. Check resolved containment and symbolic
links before accessing an untrusted path; a schema pattern alone is insufficient.

| Identifier | Scope and format |
|---|---|
| `source_id` | Library-wide `SRC-` plus digits; e.g. `SRC-900001` |
| `version_id` | Within a source: `v` plus digits; e.g. `v001` |
| `asset_id` | Within a source/version: uppercase prefix plus digits; e.g. `PDF-001`, `FIG-001`, `GUIDE-001` |
| `relation_id` | Within a source/version: `REL-` plus digits |
| `section_id` | Within a guide: `SEC-` plus digits |
| Knowledge ID | Library-wide `KNO-` plus digits, in the entry heading only |
| `topic_id` | Library-wide lowercase English hyphen-separated name |

Numeric widths are examples, not collection limits. Check all available tracked
and local records for collisions before assigning IDs. IDs survive path/title
changes and are not reused for a different entity. A revised document adds a
version; an alternate scan can add an asset in the same version. Exact-byte
mirrors add acquisition links rather than duplicated sources.

## Review and permission objects

Every `review` object has `state`, `scope`, `method`, `checked_by`, and
`checked_at`. The scope says what was actually checked, not what the record
happens to contain. `agent_added` allows unknown method/checker/date.
`cross_checked` requires those three fields and means inspection against the
actual evidence, not independent-source consensus. `human_reviewed` additionally
requires `evidence` identifying the actual human review. An agent must not invent
or infer human review. Review is separate from grading and rights.

Illustrative review object (schema: `source#/$defs/review`):

```yaml
state: agent_added
scope: Syntax example; no source has been checked.
method: null
checked_by: null
checked_at: null
```

Each action uses a complete permission object, not a scalar status. Status is
`allowed`, `prohibited`, or `unverified`. `allowed` requires at least one basis,
an assessor, and an assessment date. A basis has `kind`, `ref`, `url`, and `scope`;
kind is `license`, `public_domain`, `permission`, `statutory`, or `terms`.
The scope identifies the relevant material and actual use. Record applicable
conditions even when status is allowed; no generic AI-license category is needed.

Illustrative permission object (schema: `source#/$defs/permission`):

```yaml
status: unverified
basis: []
conditions: []
reason: Syntax example; no action has been assessed.
assessed_by: null
assessed_at: null
```

An asset's `rights` has independent `download`, `git_storage`, and
`ai_processing` permission objects. An authored record's `publication` has
`git_storage` and `ai_processing` permission objects. These assessments concern
that specific content/action, not every use of its underlying source.
`source.yaml` may include `publication` when its metadata itself needs an
explicit assessment. Guides and knowledge entries require it. Follow
[RIGHTS.md](RIGHTS.md); schema validity does not establish legality.

## Source records

Location: `sources/<source-id>/source.yaml`. Validate the parsed object against
the root of `source.schema.json`. Unknown properties are rejected so misspelled
fields do not silently disappear. Every source has at least one version, even
when it is bibliography-only.

This complete structural example has no acquired content (schema: `source`):

```yaml
schema_version: 1
source_id: SRC-900001
title: Illustrative source record
original_title: null
authors: []
publisher: null
official_record_url: null
official_record_status: unverified
official_record_note: Syntax example; this is not a registered publication.
links_checked_at: null
review:
  state: agent_added
  scope: Bibliography not checked.
  method: null
  checked_by: null
  checked_at: null
versions:
  - version_id: v001
    publisher_revision: null
    published_at: null
    download_url: null
    download_status: unverified
    download_note: No download location has been verified.
    links_checked_at: null
    authority:
      grade: U
      reason: Not assessed.
      assessed_by: null
      assessed_at: null
    assets: []
    topic_relations: []
```

`title` is an English display title; `original_title` retains the original
title when different. Do not replace faithful bibliography with an inferred
uploader identity. Record a source-level official catalog/publisher/original
publication URL and a download URL per version regardless of storage.

Link statuses are `available`, `unavailable`, `not_provided`, `not_found`, and
`unverified`. `available` requires a verified URL; other states require a note.
Keep a known dead or unverified URL. Use `null` only when no URL is known, with
the reason stated. Accessibility does not establish permission. Record actual
check dates, or `null` if not checked. Preserve earlier official links in
`official_record_history[]` and alternative/previous download links in the
version's `acquisition_links[]`; each has `url`, `status`, `note`, `checked_at`.

### Assets

Each version's `assets[]` inventories originals and retained derivatives:

| Required field | Meaning |
|---|---|
| `asset_id`, `kind` | Exact identity; kind is `pdf`, `image`, `guide`, `corrected_text`, `web_snapshot`, `video`, `audio`, or `other` |
| `path`, `availability` | Package-relative path or unknown; availability is `present`, `not_acquired`, or `missing` |
| `sha256`, `acquired_at` | Actual bytes' 64-digit hexadecimal hash and acquisition/creation date when known |
| `rights` | Separate action permission objects |
| `derived_from[]` | Original asset references, each with `source_id`, `version_id`, `asset_id`; empty for an original |

A `present` asset requires a path and hash. Do not replace the recorded original
with OCR or annotations. Register retained guides, figures, and corrected text
as derived assets. Check a hash against the actual bytes, not a filename.
Retained assets outside `local/` require allowed Git storage. Allowed Git storage
does not mean every processing action is allowed.

Optional asset fields are `locator`, `figure_label`, `caption`, `related_entries`
(knowledge IDs), and `note`. A selected figure records its original locator,
label/caption, and derivation. A clone can lack another developer's local file
without invalidating the bibliography: report the physical absence explicitly
and do not invent content or rewrite shared availability based only on that clone.

### Locators

PDF pages are one-based and inclusive, counting covers and unnumbered pages.
`asset_id` identifies the exact scan. Display `PDF p. 37` or `PDF pp. 37-52`,
never unlabeled printed page numbers. A locator object (schema:
`source#/$defs/locator`) looks like this; the numbers are not real evidence:

```yaml
type: pdf
asset_id: PDF-001
page_start: 37
page_end: 52
section: Illustrative section
```

PDF locators optionally include `section`, `figure`, and `table`. Web locators
require `type: web`, `url`, and a nonempty `heading` or `post_id`. Video locators
require `type: video`, `url`, `start_seconds`, and `end_seconds` (nonnegative
seconds). Verify ordered ranges against the actual page count or duration;
JSON Schema does not compare these fields. Do not invent PDF locators for media.

### Independently assessed topic relationships

Store relationships in `versions[].topic_relations[]`, even before knowledge
entries exist. The topic must have a canonical definition. One version can have
multiple relationships for different chapters, claims, conditions, or topics.
Authority belongs to the version; directness and relevance belong here. Resolve
grades from these records instead of copying them into guides or knowledge.

Illustrative relationship (schema: `source#/$defs/topic_relation`):

```yaml
relation_id: REL-001
topic_id: illustrative-topic
scope: Report-level discovery only; precise content not yet examined.
locator: null
applies_to:
  aircraft: null
  configuration: null
  period: null
  conditions: null
  limitations: null
directness:
  grade: EU
  reason: Not assessed.
relevance:
  grade: RU
  reason: Not assessed.
assessed_by: null
assessed_at: null
```

Use actual aircraft/configuration, period, conditions, and limitations after
inspection. `limitations: null` means unknown; `[]` means none recorded, not
unrestricted transfer. A null locator must be explained as a report-level or
unlocated relationship. It is not a precise citation for a technical claim.
Assessed grades require an assessor and date. A narrower claim with different
conditions or grades needs a separately scoped relationship.

## Document guides

Location: `sources/<source-id>/guide-<version-id>.md` or its `local/` equivalent.
YAML front matter is enclosed by `---` lines at the start of the file and is
validated against `source.schema.json#/$defs/guide_metadata`.

| Required field | Meaning |
|---|---|
| `schema_version`, `record_type` | `1` and `guide` |
| `source_id`, `version_id`, `original_asset_id` | Exact original, including the scan whose pages are used |
| `review`, `publication` | Scoped review and action permission objects |
| `coverage` | `outline_complete`, `summary_scope`, `summarized_sections[]`, `reviewed_sections[]` |
| `sections[]` | Nonempty outline; each item has `section_id`, `title`, `locator`, `relation_ids[]` |

Each section's Markdown heading uses `## SEC-001: Section title`, matching its
metadata. Lower-level headings belong to that section. An outline can include
unsummarized sections; mark them explicitly rather than adding invented prose.
`summarized_sections` and `reviewed_sections` reference the actual section IDs.
`outline_complete` says only whether the outline is complete, not whether all
content is read or verified. Describe abstract-only or other partial coverage.

The body condenses what the author explains, does, finds, and limits. Include
methods, important conditions, assumptions, units, and substantive findings;
"this section discusses control laws" alone is not a guide. Attribute the
author's conclusions. Add visible citations when a subsection needs a narrower
locator than its outline entry. Preserve relevant figures and their provenance.

Register the guide itself as an asset and keep its hash current when it changes.
If no lawfully readable content is available, omit the guide and record why in
the version's `guide_note`. Do not create a dummy guide. Alternate scans with
different page mapping need distinct guide assets/paths, not overwritten locators.

## Canonical topics and knowledge entries

Location: `knowledge/<topic-id>.md`. The front matter defines the topic; validate
it against the root of `knowledge.schema.json`. Do not maintain a second topic
list or aliases. An illustrative front-matter object (schema: `knowledge`):

```yaml
schema_version: 1
record_type: topic
topic_id: illustrative-topic
title: Illustrative Topic
scope: Syntax example only; not an aviation topic definition.
related_topics: []
updated_at: '2026-10-24'
```

After front matter, use a level-one topic title. Each entry starts with exactly
`## KNO-000001: Entry title`, followed immediately by one fenced `yaml` metadata
block, then a nonempty substantive Markdown body. Lower-level headings stay in
that entry. KNO IDs live in headings, not duplicated in metadata. Keep one
canonical home and use Markdown links from related topics.

An illustrative entry metadata block (schema: `knowledge#/$defs/entry`):

```yaml
kind: method
review:
  state: agent_added
  scope: Syntax example; no evidence checked.
  method: null
  checked_by: null
  checked_at: null
publication:
  git_storage:
    status: unverified
    basis: []
    conditions: []
    reason: No publication basis assessed.
    assessed_by: null
    assessed_at: null
  ai_processing:
    status: unverified
    basis: []
    conditions: []
    reason: No processing basis assessed.
    assessed_by: null
    assessed_at: null
evidence:
  - source_id: SRC-900001
    version_id: v001
    relation_id: REL-001
```

Allowed `kind` values are `finding`, `explanation`, `equation`, `data`, `method`,
`procedure`, `observation`, and `limitation`. Each evidence item references an
existing source/version/relation. Optional `asset_id` and `locator` narrow the
reference within that relationship's actual scope; a PDF locator includes its
own exact asset ID and must agree with any evidence-level asset ID.

Write useful source-grounded content, not only directions to open the PDF.
Preserve conditions, uncertainty, units, limitations, disagreements, and the
distinction between a source conclusion and an additional interpretation. Never
present another aircraft's results as F-CK-1C facts or project settings.
The illustrative entry above is not a real record: its references do not exist,
its rights are unverified, and it has no substantive body.

Local-only entries use `knowledge/local/<topic-id>.md`. Their front matter has
only `schema_version: 1`, `record_type: topic_supplement`, `topic_id`, and
`updated_at`. It references an existing canonical topic rather than redefining
its title or scope. The entry format and library-wide KNO uniqueness still apply.
Source images/PDFs stay with their source, not in the topic directory.

## Manual verification before completion

Until tooling is implemented, check these obligations explicitly:

- Parsed objects match the schema and grading vocabulary; no fake review claims.
- IDs are unique in their scopes; sources, versions, assets, topics, relations,
  guide sections, entry links, and original derivations resolve. Derivation and
  supersession chains are acyclic; related-topic and entry links may be mutual.
- Entry evidence matches its topic, statement, conditions, and locator. Guides'
  headings and coverage match the content actually read and checked.
- Paths resolve inside their source package, actual files/hashes agree, and
  original bytes are preserved. Range bounds match the exact asset.
- Source, derivative, and publication rights have applicable evidence and their
  conditions are fulfilled. Local-only content is not leaked into tracked files.
- Before authorized staging/commit, inspect actual staged bytes and records,
  including force-added/already tracked local files. Ignore rules are not rights
  enforcement and do not remove earlier history.

These checks cannot be replaced by schema validity. See [AGENTS.md](AGENTS.md)
for ingestion and reuse, [RIGHTS.md](RIGHTS.md) for action decisions, and
[tools/reference-library/](../../tools/reference-library/) for future automation.
