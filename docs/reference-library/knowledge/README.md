# Topic Knowledge

This directory organizes directly reusable, source-grounded information by
canonical topic. Topic files define their own scopes and evidence-backed entries.
This README is a directory guide and does not define a research topic.

## Canonical topics

Each `knowledge/<topic-id>.md` defines one stable English topic and its explicit
scope in front matter. Those files are the authoritative topic catalog; there
is no second editable topic list. Check existing scopes before adding a topic.
Do not invent alternate topic names for abbreviations, translations, or spelling
variants, and do not merge distinct technical concepts because their names seem
similar. Preserve original terminology in source content and link related topics.

Use [FORMATS.md](../FORMATS.md) for exact front matter and entry structure and
[knowledge.schema.json](../schemas/knowledge.schema.json) for structural contracts.
The README and future tool-generated catalogs are not topic records.

## Useful entries with exact evidence

Put important findings, explanations, equations, data, methods, procedures,
observations, and limitations under the topic where they are useful. A topic
can combine several sources while retaining each contribution's conditions and
provenance. Keep one canonical home for each stable knowledge entry; related
topics link to it instead of copying its body.

Write substantive content an agent can use without immediately rereading an
entire PDF. Include assumptions, aircraft/configuration, units, operating or
experimental conditions, uncertainty, and transfer limitations. A result from
another aircraft does not become an F-CK-1C fact. Project settings and design
decisions remain outside this library.

Each entry has its stable knowledge ID, structured metadata, substantive body,
scoped review, publication basis, and evidence references as specified in
[FORMATS.md](../FORMATS.md). Link the source ID, version, and `relation_id`, with
an exact asset and locator where needed. Resolve authority from the source
version and directness/relevance from its canonical `topic_relations` record.
If a narrower point needs different conditions or assessment, create a separate
scoped relationship rather than silently overriding shared grades.

PDF citations use one-based PDF pages, including covers and unnumbered pages,
with inclusive ranges. Use clearly labeled citations such as `PDF p. 37`;
printed page labels are not substitutes. For other media use verified headings,
post identifiers, or time ranges.

Preserve competing reports and their evidence rather than creating a false
consensus. Attribute source authors' conclusions and inferences. Make any
additional interpretation explicit instead of presenting it as a source fact.

## Review and figure use

An agent can cross-check an entry against its actual evidence and record the
method, reviewer, date, and scope. Human review is distinguishable and must not
be invented. Bibliography checks do not verify every formula or diagram; partial
checks remain visible. See [AGENTS.md](../AGENTS.md) and
[ratings.yaml](../ratings.yaml) for the shared rules.

Reference relevant saved figures with captions and original locators. Agents
proactively display them when helpful and permitted; figure display is not
conditional on a separate confirmation request. Keep images with their source
packages instead of copying them into topic directories. State when a value,
label, or connection is unclear and identify unverified transcriptions.

## Local-only supplements

Follow [RIGHTS.md](../RIGHTS.md) for the entry itself as well as its underlying
sources. A source's Git-storage status does not automatically establish whether
every explanation, quotation, or image derived from it may be tracked.

Put local-only topic content in `knowledge/local/<topic-id>.md`. Its
`topic_supplement` front matter refers to an existing canonical topic rather than
defining a competing one. Keep the source PDFs and images in their source
package's `local/`. Do not duplicate restricted content in tracked topic files.
Create the optional `local/` directory only when real supplements exist.

## Current workflow

Read and edit these files manually under the format contracts. Before adding an
entry, inspect existing source/version/relationship IDs, topic scopes, evidence
locators, and rights. Manually check references and ID uniqueness, including
available local supplements. Automated parsing, validation, database indexing,
and library search commands are future implementation work, not checks already
provided by these instructions.
