# Aviation Reference Library Design

Status: File-based contracts and manual workflow; runtime tooling deferred.
Updated: 2026-10-24.

This document consolidates the accepted requirements for an English-language
aviation reference library under `docs/reference-library/`. It also specifies
concrete data contracts. The structure and instructions are implemented in
[reference-library/](reference-library/README.md); its guide describes available
capabilities, not a collection inventory. This design does not itself authorize
downloads, publication, Git operations, or changes to Git hooks.
[FORMATS.md](reference-library/FORMATS.md) and the schemas are the exact
serialization contract; this document explains the design and later work.

- **Accepted baseline:** decisions explicitly accepted during the discussion.
- **Foundation contract:** implemented fields, codes, and serialization.
- **Open implementation choice:** a choice identified in section 12, with a
  recommended starting point rather than an implied prior decision.

## 1. Purpose and scope

**Accepted baseline.** Give developers and AI agents a common place to obtain
external aviation information during design, discussion, and programming.
Include direct F-CK-1C material, related aircraft research, subsystem documents,
and useful background research. Analogue research must retain its actual
aircraft, configuration, dates, operating conditions, and limitations.

The library contains source material and source-grounded knowledge. Project
configuration, simulation tuning, implementation status, and project design
decisions remain in their owning project documents. A finding about another
aircraft does not establish a fact about the F-CK-1C.

All authored filenames, fields, rules, topic definitions, guides, and knowledge
content use English. Preserve original documents, images, titles, names, and
quoted terminology in their original form; identify any English translation.
Conversation with a developer can continue in the developer's language.

## 2. File structure and ownership

**Accepted baseline**, including the later `local/` storage agreement:

```text
F-CK-1C/
├─ docs/
│  ├─ REFERENCE_LIBRARY_DESIGN.md          # This reviewable design
│  └─ reference-library/
│     ├─ README.md                        # Purpose, structure, tool entry points
│     ├─ AGENTS.md                        # Agent reading and contribution rules
│     ├─ RIGHTS.md                        # Canonical rights-handling procedure
│     ├─ FORMATS.md                       # Manual serialization and record guide
│     ├─ ratings.yaml                     # Shared grading definitions
│     ├─ schemas/
│     │  ├─ source.schema.json             # Source record contract
│     │  └─ knowledge.schema.json          # Topic and knowledge record contract
│     ├─ sources/
│     │  └─ SRC-900001/
│     │     ├─ source.yaml                 # Bibliography, versions, assets, rights
│     │     ├─ document-v001.pdf            # Only when Git storage is permitted
│     │     ├─ guide-v001.md               # Substantive document condensation
│     │     ├─ figures/v001/               # Permitted, selected original figures
│     │     └─ local/                      # Local-only assets; ignored by Git
│     │        ├─ document-v002.pdf         # Example of a local-only revision
│     │        ├─ guide-v002.md             # If this guide is also local-only
│     │        └─ figures/v002/
│     ├─ knowledge/
│     │  ├─ automatic-flight-control.md    # Canonical topic plus usable knowledge
│     │  ├─ control-law-design.md
│     │  ├─ flight-testing.md
│     │  └─ local/                        # Local-only topic supplements, if needed
│     └─ generated/                       # Rebuildable; ignored by Git
│        ├─ library.sqlite                 # Relational records and search index
│        └─ text/                          # Extracted text and automatic OCR
└─ tools/
   └─ reference-library/                   # Ingestion, validation, indexing, search
```

The source IDs, topics, and versions above illustrate layout, not collected
sources. Create optional directories only when they hold actual content.

| Location | Responsibility | Authoritative input? |
|---|---|---|
| `source.yaml` | Source identity, bibliography, versions, assets, rights, topic relationships | Yes |
| `guide-v*.md` | Condense the actual content of one document version | Yes |
| `knowledge/*.md` | Organize directly usable information by canonical topic | Yes |
| `figures/` | Preserve selected source images and their provenance | Yes |
| `ratings.yaml`, `schemas/`, `RIGHTS.md` | Define shared rules and contracts | Yes |
| `generated/` | Build searchable views of the authoritative inputs | No |

Ordinary ingestion updates the affected source packages and topic records, plus
consuming links if an asset moves. Refresh affected derived indexes when
implemented tooling supports them. Shared READMEs, rules, and this design are
not a hand-maintained catalog: do not change them to record source counts,
first-source milestones, or whether the collection is empty. Update them when
responsibilities, contracts, procedures, or actual capabilities change.

The planned database is rebuildable. Agents currently edit the authoritative
files manually; future tools will register them, not independent database rows.
There is no second manual
topic catalog: each public topic file defines its own ID and scope; tools derive
the global topic list. Root `AGENTS.md` points relevant work to this library.

## 3. Identity, versions, and locators

**Accepted baseline:** distinguish a source, its published revisions, actual
files, and locators. **Foundation contract:** use the following identifiers.

| Entity | Example | Meaning |
|---|---|---|
| Source | `SRC-900001` | One identifiable publication or original contribution |
| Recorded version | `v001` | A recorded edition/state; not a publisher's revision label |
| Asset | `PDF-001`, `FIG-001` | One exact PDF, image, guide, or corrected text asset |
| Topic | `automatic-flight-control` | One defined concept, using a stable English name |
| Evidence relationship | `REL-001` within a source/version | An independently assessed source-to-topic relationship |
| Knowledge entry | `KNO-000001` | One reusable point with its own evidence and review |

IDs remain stable when titles or paths change. The example numeric widths do
not impose a collection-size limit. Manually check uniqueness now; allocation
tools are future work.

- A mirror of an identical file adds an acquisition link, not another source.
- A publisher's revised document adds a version; preserve the earlier version.
- Different scans of the same edition can be separate assets within a version.
  Their PDF pages may differ, so a locator always identifies the actual asset.
- A different paper, commentary, or independently authored contribution gets
  its own source ID, even if it discusses an existing source.
- Preserve original PDFs. Record SHA-256 for acquired files; do not overwrite
  them with OCR, reflowed text, annotations, or English translations.

**Accepted page convention:** every page locator and displayed citation uses
one-based PDF page numbers, including covers and unnumbered pages. Write
`PDF p. 37` or `PDF pp. 37-52`. Never silently substitute printed page labels.
Tools using zero-based offsets convert at their adapter; stored locators remain
one-based. Ranges are inclusive. Identify figures, tables, and sections as well
when useful. Web sources use headings/post identifiers; videos use time ranges.
Do not invent PDF page numbers for non-PDF sources.

## 4. Data formats

**Foundation contract.** Use UTF-8 YAML for structured source records and grading
definitions, Markdown with YAML metadata for authored reading material, and
JSON Schema for validation. Use relative, forward-slash paths in records.
Dates use quoted `YYYY-MM-DD` strings. Unknown information
is explicit, never a guessed date, URL, author, value, or empty success result.

Schemas describe record structure. Manual checks currently cover references, unique
IDs, paths, file hashes, page ranges, topic membership, and rights consistency.
Future tools will automate these checks. Schema-valid data alone does not prove
that a source was read or is accurate.

### 4.1 Source records

`source.yaml` remains Git-tracked when its contents may be shared, regardless
of whether its PDFs are local-only, unavailable, or not downloaded.

| Fields | Contract |
|---|---|
| `schema_version`, `source_id` | Format version and stable identity |
| `title`, `original_title`, `authors`, `publisher` | English display title and faithful bibliography; unknown values remain explicit |
| `official_record_url`, `official_record_status`, `official_record_note` | Mandatory fields for the official catalog, publisher, or original publication page |
| `links_checked_at`, `review` | Provenance of the cataloging work |
| `versions[]` | At least one recorded state, including a link-only source |
| Version bibliography | `version_id`, `publisher_revision`, `published_at`; distinguish internal identity from published labels |
| Version acquisition | `download_url`, `download_status`, `download_note`, `links_checked_at`; mandatory even without a local file |
| Version assessment | Authority grade, rationale, assessment author/date, and scope |
| `assets[]` | Asset ID, kind, relative path, availability, acquisition date, hash, rights, and origin references |
| `topic_relations[]` within a version | Independently assessed links from source content to canonical topics; usable before knowledge entries exist |

Link states are `available`, `unavailable`, `not_provided`,
`not_found`, and `unverified`. Preserve a known URL even when it is unverified,
temporarily inaccessible, withdrawn, or no longer downloadable. Use `null` only
when no URL is known, with a reason. `available` requires a verified URL and
describes accessibility, not permission to download or redistribute. Preserve
earlier known URLs as acquisition history when a preferred link changes.
Use an original post as the publication page where appropriate. Never label a
mirror as an official catalog or fabricate a missing official record.
Record alternate acquisition links when useful, with their actual provenance.
Do not store credentials or private access tokens in tracked URLs.

The following is an illustrative format fragment, not a cataloged document or
a successful acquisition. See FORMATS for the complete implemented contract:

```yaml
schema_version: 1
source_id: SRC-900001
title: Illustrative source record
original_title: null
authors: []
publisher: null
official_record_url: null
official_record_status: unverified
official_record_note: Format example only; this is not a registered publication.
links_checked_at: null
review:
  state: agent_added
  method: null
  checked_by: null
  checked_at: null
  scope: Bibliography not yet checked.
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

An acquired asset adds its exact path and SHA-256. Availability values
are `present`, `not_acquired`, and `missing`. Local absence on another checkout
is not proof that the catalog record is invalid. An operation needing that
file reports its absence explicitly; it must not fabricate its text or images.
Newly created guides, selected figures, and corrected OCR also enter the asset
inventory. Derived assets identify the source assets from which they were made.

Each `topic_relations[]` item has a `relation_id`, `topic_id`, content scope,
asset and locator where available, actual aircraft/conditions, independent
`directness` and `relevance` assessments, and assessor/date. A report can have
separate flight-control and flight-testing relations before either has a KNO
entry. A report-level relation may initially have no precise locator; explicitly
record that limited scope rather than inventing a section or page.
This is the canonical location for relationship assessments. Guides and
knowledge entries refer to those records instead of copying mutable grades.

### 4.2 Document guides

**Accepted baseline.** A guide is a substantive condensation of the document,
organized by its actual sections. It conveys what the author explains, does,
finds, and limits, including important operating or experimental conditions.
It must provide useful content without requiring an immediate PDF read.
Sentences such as "This chapter discusses flight control" are insufficient.

**Foundation contract:** each guide has YAML front matter identifying its source,
version, original asset, review, and coverage. Each condensed section has a
stable section ID and a structured original locator. Markdown holds the
condensed prose; the guide contract is validated under `source.schema.json`.
The file name identifies the version; front matter identifies the exact scan.
If a second scan needs different locators, register a distinct guide asset/path.

A first pass creates a useful basic condensation when source content can be
lawfully read. A bibliography-only record remains valid without a guide;
record why content/guide creation is unavailable. An abstract-only condensation
must declare that limited coverage. Never fill unread sections with invented
content. Later work deepens sections actually used. Preserve methods,
substantive findings, assumptions, units, and
limitations. Attribute the author's conclusions and inferences to the author.
Do not add a new F-CK-1C conclusion as if it came from the source.

Record whether the outline is complete and which sections have actually been
reviewed. Partial coverage is normal and visible. A section not summarized or
tagged may still contain relevant information; full-text search remains usable.

### 4.3 Topic knowledge

**Accepted baseline.** Important reusable information is organized by topic,
not in one notes file per source. A topic can draw on several sources. Entries
may contain findings, explanations, equations, data, procedures, observations,
and limitations that agents can directly use, retaining their conditions.

Each topic has one canonical English name and an explicit scope. Do not create
alternate topic names for spelling variants, abbreviations, or translations.
Preserve terms actually used in a source as source text; do not automatically
declare nearby technical terms equivalent. Related concepts use related-topic
links. Search can use original terms without making them competing categories.

**Foundation contract:** a topic file starts with `schema_version: 1`,
`record_type: topic`, `topic_id`,
`title`, `scope`, `related_topics`, and `updated_at` in YAML front matter.
Each entry begins with a level-two heading containing its stable `KNO-*` ID,
followed by a YAML metadata block and then its substantive Markdown body.
Lower-level headings belong to that entry. This keeps structured metadata and
readable prose in one file. Parsing and cross-reference automation are future
work; the manual workflow checks both now.

Entry metadata includes `kind`, `review`, `publication`, and `evidence[]`.
Each evidence link identifies a source/version and `relation_id`, with an exact
asset/locator for the particular entry when needed. Resolve aircraft/conditions,
directness, relevance, rationales, and assessment provenance from that relation;
resolve authority from the source version. If a narrower claim needs different
grading or conditions, register a separately scoped relation. Do not silently
override or duplicate shared assessments in the knowledge file.

See [FORMATS.md](reference-library/FORMATS.md) for complete illustrative entry
metadata. Each publication action is a full permission object, not a scalar
status. Every PDF locator identifies its exact `asset_id`. Illustrative IDs
and pages are not registered evidence; real entries require actual content,
resolved provenance, and an applicable publication basis.
An entry has one canonical home; other topics link to it rather than duplicate
its full content. Store competing reports with their individual conditions and
provenance; preserve disagreements rather than manufacturing a consensus.

Local-only entries use `knowledge/local/<topic-id>.md`. Its front matter uses
`record_type: topic_supplement` and references the canonical `topic_id`; it does
not create a second topic definition. Keep source assets with their source,
including local-only PDFs and images. Only permitted content enters tracked
topic files, regardless of whether its original source file is tracked.

### 4.4 Figures and corrected text

Preserve selected figures as original extractions or PDF renders, not AI
recreations. Keep relevant labels, legends, axes, units, and captions. Register
each image's source/version/asset, PDF page, figure label, caption, rights, and
related knowledge entries. An unclear original remains explicitly unclear.
AI-transcribed values or connections are interpretations with their own review.

Automatically reproducible renders/text belong in `generated/`. Selected figures
with curated provenance and manually corrected OCR are retained as source assets
in the permitted tracked or `local/` location. No derivative loses its rights
restrictions merely because a tool generated it or changed its format.

## 5. Fixed grading standards

**Accepted baseline:** multiple independent, discrete grades; no percentages,
combined score, or assumption that source prestige proves applicability.
**Foundation contract:** the following scales are defined in `ratings.yaml`.

### 5.1 Source authority

Assess the identifiable publication and responsible author/publisher for the
recorded version, not just the hosting domain or a citation to a famous body.

| Grade | Label | Basis |
|---|---|---|
| A | Authoritative | Verified first-party government/regulator, original manufacturer, or formal issuing institution speaking within its responsibility |
| B | Professional | Accountable scholarly or professional publication, technical book, or identifiable qualified technical author |
| C | Traceable non-official | Identifiable, attributable non-official publication or original contribution with inspectable provenance |
| D | Informal or unverified | Anonymous/informal assertions, weak attribution, or insufficiently established author/publisher authority |
| U | Unassessed | Authority has not been evaluated or cannot yet be determined |

Grade A does not mean infallible, legally unrestricted, or valid for another
aircraft. Peer review and institutional affiliation are evidence for assessment,
not a substitute for reading the actual publication and its limitations.

### 5.2 Evidence directness

Assess the particular evidence used, which may differ between parts of a source.

| Grade | Label | Basis |
|---|---|---|
| E1 | Primary evidence | Original measurements, analysis, tests, directly observable imagery, or first-party design/operating documentation for the stated claim |
| E2 | Traceable technical synthesis | Technical analysis or synthesis with identifiable underlying primary evidence |
| E3 | Interpretation or recollection | Commentary, interpretation, interview recollection, or a secondary account without directly inspectable support for this point |
| E4 | Unsupported assertion | A claim with insufficient supporting provenance or evidence |
| EU | Unassessed | The evidence type or its directness has not been determined |

An unofficial video can directly show a visible feature, even when its author's
authority is low. A first-party specification proves what is specified, not
automatically what was measured in operational service.

### 5.3 Relevance to the research target

Assess each evidence-to-topic relationship against the F-CK-1C research context
and the actual engineering question. Record the reason and meaningful differences.

| Grade | Label | Basis |
|---|---|---|
| R1 | Direct | Addresses the target F-CK-1C configuration and the topic directly |
| R2 | Strongly related | A documented series/subsystem relationship or strong engineering analogue with relevant architecture, conditions, and differences explained |
| R3 | Comparative | A different platform or context addressing a comparable engineering problem, with material transfer limitations |
| R4 | Background | General theory or a more distant context useful for foundational understanding |
| RU | Unassessed | Applicability to the target question has not been evaluated |

Similar dates alone do not establish R2. The same report may have different
grades for different topics. R2 never asserts that the F-CK-1C has the same
control law, parameters, hardware, or performance. Preserve the actual subject.

Every assessment records its scope, rationale, assessor, and date. Unknown is
not a low grade. Grades aid discovery and judgment; they do not automatically
exclude content. Filter only when the query explicitly requests a filter.
Rights and review status remain independent of all three grading axes.

## 6. Review, coverage, and freshness

**Accepted baseline:** agents can add and cross-check records; human review is
distinguishable and cannot be fabricated. **Foundation review states:**

| State | Meaning |
|---|---|
| `agent_added` | Cataloged or authored by an agent; the declared scope has not yet been verified |
| `cross_checked` | Checked against the actual source for the declared scope, with method and provenance recorded |
| `human_reviewed` | A human has explicitly reviewed the declared scope; record that evidence |

`cross_checked` does not inherently mean several independent sources agree.
Agents should perform useful source checks and may advance that state without
mandatory human review. Record partial checks honestly. Review of bibliography
does not establish correctness of all equations, figures, or knowledge entries.
Agents can use entries with their visible status; do not make human review a
universal access gate or pretend that unchecked material was verified.

Coverage and review are separate: record summarized sections, reviewed sections,
and an explicit outline-completeness indicator. A newer source version does not
silently change existing citations or retroactively verify derived content.
Track superseded versions separately from the review state and retain history.

## 7. Copyright, legal use, and Git storage

**Accepted baseline.** Follow applicable law and source-specific terms before
acquisition, storage, reuse, and publication. Rights are not inferred from an
authority grade, public accessibility, or the existence of a download button.
NASA STI itself describes third-party works and source-specific restrictions
in its [terms of use](https://sti.nasa.gov/disclaimers/).

**Foundation rights contract:** each affected asset/action records `allowed`,
`prohibited`, or `unverified`, with its basis, conditions, review author/date,
and relevant terms or permission links. Separate `download`, `git_storage`, and
`ai_processing`; record additional excerpt/image/publication conditions where
needed. A basis can be an applicable license, public-domain determination,
specific permission, or documented statutory basis for the actual use.

| Situation | Required handling |
|---|---|
| Download prohibited or its basis unresolved | Keep permitted links and bibliography; do not download the material |
| Local acquisition allowed; Git storage prohibited or unresolved | Put the acquired asset in the source's `local/`; retain permitted metadata in `source.yaml` |
| Git storage allowed | Keep the asset at its tracked source path and meet attribution and other conditions |
| Figure, guide, OCR, or knowledge publication has different terms | Assess and store that material according to its own permitted use |

Every source retains its official record link and every version its download
link/status whether downloaded, tracked, local-only, or link-only. A missing
official page is explicit. Legal availability and technical availability differ.
Do not bypass access restrictions or preserve unauthorized copies as if they
were legitimate sources. Do not apply the repository's license to third-party
materials; preserve their actual notices and conditions.

Authored explanations of ideas and factual findings require different analysis
from copying expressive text, figures, or translations. Do not assume that
English translation, OCR, condensation, or attribution alone permits reuse.
The [Taiwan Intellectual Property Office explanation](https://www.tipo.gov.tw/tw/copyright/692-14988.html)
distinguishes ideas, adaptation, and quotation; actual use remains contextual.

### 7.1 Local-only placement and enforcement

Keep local-only files in the same library/source location, using the fixed
`local/` directory convention rather than a per-file ignore list or suffix.
The root `.gitignore` rules are:

```gitignore
/docs/reference-library/**/local/
/docs/reference-library/generated/
```

Use the same convention for local-only topic supplements. Source metadata
remains outside `local/` when it may be tracked. Never store two copies of an
asset merely to satisfy both tracked and local layouts.

Ignore rules are not permission checks. Before staging/commit, validate the
actual staged library contents, including newly force-added local files, and
reject prohibited/unverified Git storage or known local-only content. Validate
the staged record and bytes, not just a more permissive unstaged description.
This is a manual obligation now; automated staged-rights enforcement is not
implemented in the foundation.
Check current tracked files as well: `.gitignore` does not untrack them or erase
history, as documented by [Git](https://git-scm.com/docs/gitignore).

Before importing existing repository material, audit its rights and citations,
preserve original bytes and provenance, and update consuming links when moving
permitted assets. Record the assessment in the destination source package. If
restricted material is already in history, report it and propose remediation;
do not silently rewrite history. Workflow checks reduce accidental publication;
they cannot promise to prevent deliberate bypass or determine the law by hash.

### 7.2 AI reading and external processing

An agent reading a file at a developer's request is not automatically copyright
infringement. Local-only Git storage does not by itself prohibit agent reading.
If a model runs remotely, text/images sent to that model are processed by a
service; this is not automatically public publication or model training.
Assess the actual use, applicable terms, and any statutory basis. Taiwan's
[Copyright Act](https://law.moj.gov.tw/LawClass/LawAll.aspx?PCode=J0070017),
including Article 65, requires a contextual fair-use assessment.

Do not impose a universal requirement for an expressly named "AI license."
Absence of AI-specific wording alone does not establish a prohibition. Record
an actual restriction or material unresolved issue, and explain the action it
affects. Do not promise that developer instruction makes every use lawful.
When a source-specific restriction applies, return permitted metadata/locators
and explicitly state why requested content was not supplied; never fake a
successful full-content result or silently omit the restriction.

## 8. Search and relational model

**Accepted baseline.** Use relational records and full-text search to expose
small, relevant parts of a large collection. The accepted layout uses a
rebuildable SQLite file. Graph and semantic/vector search are not prerequisites.

**Planned logical records:** sources, versions, assets, source sections,
topics, knowledge entries, evidence relationships, and per-asset text passages.
Source-to-topic relationships carry assessments independently of knowledge
entries; entry citations reference those relationships with precise locators.
Topic supplements extend an existing topic. Resolve authority from its version record.
Build these views from the files; do not maintain a parallel editable catalog.

Search includes knowledge, substantive guides, and original extracted text.
It must not depend exclusively on topic tags or condensed guides. Indexing
unknown/unmapped sections preserves discovery beyond previous curation.
Mark each result as `knowledge`, `guide`, `source_text`, or `figure` so an agent
cannot mistake an AI-authored condensation for a verbatim primary source.

Return IDs, a useful excerpt, chapter context, exact locators, applicable
aircraft/conditions, separate grades with rationale, review/coverage, related
figures, and access/rights status. Agents request expanded entries, sections,
or specific original pages as needed. Do not send an entire PDF by default.
Verified knowledge may be reused without reopening the PDF on every task.
Read the original where missing context, discrepancies, or uncertainty require it.

Preserve section/paragraph context when extracting text, with exact PDF pages.
Identify OCR and its quality limitations. Tables, equations, and diagram topology
may require a page/image instead of flattened text. Extraction failures, partial
output, missing files, and stale indexes must be explicit. Rebuild index records
when their authoritative input hashes change; do not silently return stale facts.
Indexing and result delivery obey source-specific processing restrictions.
There are no arbitrary collection limits or undocumented grade cutoffs.

## 9. AI agent rules

The library `AGENTS.md` and short root entry point establish the following
behavioral rules. Use manual reading/cataloging now; extraction, library search,
database refresh, and automated validation below require later tooling:

1. Search the library before external research relevant to design or programming.
   Read the smallest useful knowledge/guide section before expanding to original
   text or pages. Use full-text search when curated material may be incomplete.
2. Treat source-grounded knowledge as reusable information with visible scope
   and review, not just a pointer requiring a complete PDF reread.
3. Preserve aircraft/configuration, conditions, uncertainty, units, and provenance.
   Never convert another aircraft's results into F-CK-1C facts or project settings.
4. Cite source, version, exact asset where needed, and clearly labeled PDF pages
   for substantive source-based claims. Retain the knowledge entry ID when useful.
5. Proactively display all saved figures relevant to the answer, within the
   applicable usage conditions, with caption and source locator. The purpose is
   developer understanding; display is not conditional on asking for confirmation.
6. If a diagram or value is unclear, say what is unclear. Preserve the image and
   mark any transcription as unchecked; do not invent values or connections.
7. Before completing work, register new external sources that materially informed
   the work and have continuing value. Preserve PDF first when lawful; otherwise
   retain the permitted link/bibliography and explain the unavailable operation.
8. Check for an existing publication/version/file before registration. Preserve
   both official record and download links/status, even for local-only sources.
9. For legally readable content, perform a basic substantive guide on ingestion,
   then deepen actually used sections and add valuable topic knowledge
   incrementally. Link-only records do not require a fabricated guide. Do not
   imply that uncurated sections contain no useful content.
10. Add forums and unofficial media only when the task established specific value,
    such as unique imagery, a useful lead, or an attributable observation.
11. Use canonical English topic definitions. Propose a new topic only after
    checking existing scopes; never invent aliases or collapse distinct concepts.
12. Record separate grades and honest scoped review. An agent may cross-check;
    it must not impersonate a human reviewer or hide contradictory evidence.
13. Update authoritative files through the library workflow, validate references
    and rights, and refresh affected indexes. Report failed actions explicitly.
    Do not collect every search hit or generate summaries of unread documents.

## 10. Acceptance scenarios for implementation

These are planned acceptance cases, not tests already executed:

| Scenario | Expected result |
|---|---|
| A permitted technical PDF covers several topics | One source package; separate evidence relationships and locators; no PDF duplication by topic |
| A source cannot be downloaded | Searchable permitted bibliography and both link fields/status; no invented original text |
| A known URL becomes unavailable | Retain the URL with its new status and check date; do not erase acquisition history |
| A report is mapped before KNO entries exist | Independently graded chapter-to-topic relationships remain searchable |
| A lawful local-only file is registered | File stays beside its source under `local/`; metadata tracked when permitted; commit checks reject prohibited staging |
| A clone lacks local-only assets | Catalog remains readable; content requests explain the missing local asset |
| A valuable unofficial image exists | Grades reflect authority and directness separately; relevant display is proactive when permitted |
| A user queries an uncurated section | Full-text retrieval can find it without a topic tag or complete guide |
| A revised PDF or alternate scan exists | Old references remain exact; new version/asset is distinguished; derived content does not silently change |
| Two sources disagree | Each statement retains its evidence and conditions; no fabricated resolution |
| A developer reuses a checked finding | Useful content and provenance are returned without an obligatory full-document reread |
| A rights restriction affects images/OCR | The derivative follows its actual restrictions; no leakage through guides or generated search output |

## 11. Implementation boundaries

The file-based foundation defines library and directory guides, agent
instructions, rights handling, grading definitions, concrete record contracts
and examples, root discovery links, ignore rules, and tooling ownership.
Collection contents are established by source packages and topic records, not
by this design. Each collection or migration task has its own authorization and
rights review; moving an asset also requires updating its consuming citations.
Neither this design nor ordinary ingestion authorizes a commit, push, PR, or
Git-hook change.

Planned runtime implementation covers parsing/cross-record validation,
rights-aware registration and staging checks, extraction with page fidelity,
and indexing / search. Review behavior with permitted documents and
link-only/local-only cases when validating those capabilities. Verification is
not legal assessment or human review.

## 12. Review points and remaining choices

The grading definitions and Markdown contracts support the manual workflow
independently of collection contents. Before building runtime tools, select the
programming language, PDF extractor, OCR engine, search tokenizer, and
cross-language query behavior using actual permitted documents. A local CLI
returning structured results is the recommended initial interface; an MCP adapter
can reuse its operations if needed. No such runtime/package choice or executable
tool is supplied by the file-based contracts.

No extra approval is requested for already accepted English content, topic-based
knowledge, substantive source guides, proactive figures, PDF page numbering,
per-source local storage, or retention of both kinds of source links.
