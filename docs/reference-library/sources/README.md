# Source Packages

This directory holds original publications and their source records. There are
no registered sources yet. This README is a directory guide, not a source record.

## One package per identifiable publication

Use `sources/<source-id>/` with a stable source ID. Do not organize originals by
topic or publisher, and do not duplicate a PDF because it supports several topics.
Create a package only for material actually being registered.

Possible contents are:

| Path within a source package | Purpose |
|---|---|
| `source.yaml` | Bibliography, recorded versions, assets, rights, and independently assessed topic relationships |
| `document-v*.pdf` | Original PDF when Git storage is permitted |
| `guide-v*.md` | Substantive, section-based condensation of a specific version and original asset |
| `figures/<version-id>/` | Selected original figures with curated provenance when permitted |
| `local/` | Local-only originals and derivatives, ignored by Git |

Create optional files and directories only when actual content needs them.
Link-only packages can contain only `source.yaml`; no empty PDF or guide is
required. A record whose permitted local asset is absent on another machine
remains useful for discovery.

## Record formats and identity

Use [FORMATS.md](../FORMATS.md) for exact fields, locator conventions, and guide
metadata, and [source.schema.json](../schemas/source.schema.json) for structural
contracts. Shared assessments use [ratings.yaml](../ratings.yaml).

Manually check existing records before selecting unused IDs; automated
registration and collision checks are not implemented. A mirror of identical
bytes adds an acquisition link. A publisher revision adds a version. Different
scans of the same edition can be distinct assets within a version. A new paper
or independent commentary is a new source.

Keep internal version IDs separate from the publisher's revision labels. Do not
overwrite originals with OCR, annotations, reflowed text, or translations.
Register acquired files and retained authored/derived assets with provenance,
their exact relative paths, and SHA-256 hashes. Derived assets identify the
original assets they use.

## Bibliography and links

Every source records its official publication/catalog URL; every version records
its download URL. Include status, reason, and check date for both whether a PDF
is tracked, local-only, unavailable, or not acquired. Keep known broken URLs;
use `null` only when no URL is known. Do not invent an official record or call a
mirror official. Preserve meaningful acquisition history when links change.

Unknown authors, dates, and other facts remain explicitly unknown. Do not infer
an original author from an uploader or a publisher from the hosting domain.

## Guides and topic relationships

A guide condenses the content actually read: explanations, methods, findings,
assumptions, conditions, and limitations. Organize it by the original sections,
with stable section identities and exact original locators. "This section
discusses control laws" is not a useful condensation.

Create a basic substantive guide for lawfully readable content, then deepen
sections actually used. Declare outline completeness, summarized coverage, and
reviewed coverage separately. An abstract-only guide says so. Record why no
guide can be created for bibliography-only material; never fabricate unread
sections. Attribute the author's conclusions and preserve their actual aircraft
and conditions.

Store independently scoped `topic_relations` in the source version's record.
Each relationship identifies its topic, content scope, actual conditions,
verified locator when available, separate directness/relevance grades, and
assessment provenance. A report-level relationship can explicitly lack a precise
locator. Relationships are usable before knowledge entries exist; guides and
topic entries reference them instead of maintaining competing grades.

## Figures, locators, and storage

Selected source figures retain labels, axes, units, legends, captions, and exact
source/version/asset provenance. Preserve the original extraction or page render;
do not replace unclear evidence with an AI recreation. Automatically reproducible
renders and extraction belong in `generated/`; curated figures and corrected OCR
are retained assets in their permitted tracked or local-only location.

PDF locators always count from page 1 of the actual PDF, including covers and
unnumbered pages. Ranges are inclusive. Use section names, figure/table labels,
post identifiers, or timestamps as appropriate; do not invent PDF pages for
websites or videos.

Follow [RIGHTS.md](../RIGHTS.md) before acquisition, reuse, and publication. Keep
local-only source content together under this package's `local/`, including
restricted guides, figures, and corrected text. Keep permitted metadata outside
`local/`; do not leak restricted content through it. Source authority never
establishes usage permission.
