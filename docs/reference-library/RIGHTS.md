# Rights Handling

Use source-specific terms and the applicable legal basis for each actual action.
This procedure supports documented decisions; it is not an automatic legal
determination or a guarantee that a particular use is lawful. A source's grade,
public accessibility, or download button does not establish permission.

## Assess the material and the action

Before acquisition or reuse, identify the work, version, asset, responsible
publisher, and applicable notices. Record the rights assessment in the asset or
knowledge publication record defined by [FORMATS.md](FORMATS.md) and
[schemas/](schemas/). Preserve terms or permission links, the basis, conditions,
assessor, date, and scope.

Assess download, Git storage, and AI processing separately. Additional excerpt,
image, translation, or publication conditions may apply. Record `allowed`,
`prohibited`, or `unverified` for the actual action and explain the basis.
A basis may be a license, documented public-domain determination, specific
permission, or a documented statutory basis for the actual use. Record material
uncertainty rather than guessing or claiming that a famous publisher has no
restrictions.

NASA STI's [terms of use](https://sti.nasa.gov/disclaimers/) describe restrictions
and third-party material; assess the particular work instead of treating all
NASA-hosted content as unrestricted. Preserve third-party notices and do not
apply this repository's license to externally owned material.

| Actual situation | Storage and handling |
|---|---|
| Download prohibited or its basis unresolved | Keep permitted bibliography and links; do not acquire the original. |
| Local acquisition allowed, Git storage prohibited or unresolved | Keep the acquired asset in its source's `local/`; retain permitted metadata outside it. |
| Git storage allowed | Use the tracked source path and fulfill attribution and other conditions. |
| Derivative or publication has different conditions | Assess that derivative/action separately and use its permitted location. |

Do not bypass access restrictions. A technically reachable file is not
necessarily permitted for download, Git distribution, or another use.

## Keep discovery links for every source

Keep the source's official publication/catalog link and each version's download
link with status, reason, and check date, regardless of whether an asset is
tracked, local-only, missing, or never acquired. For an original post, its
publication page can be the official record.

Retain known URLs when they become inaccessible or downloads disappear. If no
URL is known, use `null` and explain why. Do not invent a download endpoint,
substitute a mirror for an official record, or store credentials/private access
tokens in tracked URLs. Link accessibility and permission are different facts.

## Originals, guides, knowledge, images, and OCR

Preserve original files and notices. OCR, translation, condensation, screenshots,
or a format conversion do not by themselves establish a right to reuse the
result. Selected figures and corrected text retain their source provenance;
automatic extraction and search results must respect actual restrictions too.

Assess authored explanations and factual findings in their actual context,
separately from reproducing expressive text or images. The
[Taiwan Intellectual Property Office explanation](https://www.tipo.gov.tw/tw/copyright/692-14988.html)
discusses ideas, adaptation, and quotation. Attribution alone does not settle
whether copying, translation, or publication is permitted.

Place local-only PDFs, figures, guides, corrected text, or other source assets
in that source's `local/`. Place local-only topic knowledge in
`knowledge/local/<topic-id>.md` as a supplement to the canonical topic. Do not
copy restricted prose, images, or OCR into tracked metadata or topic files merely
because the original is ignored. Generated data are not exempt from restrictions.

Keep one asset in its correct storage location rather than making tracked and
local duplicates. A checkout lacking a local-only asset may retain its permitted
catalog record; clearly report that the original is unavailable on that machine.

## Agent reading and external processing

Reading a file at a developer's request is not automatically copyright
infringement. A `local/` location controls Git storage; it does not itself forbid
lawful agent reading. Conversely, developer instruction does not make every use
lawful.

When a model runs remotely, supplied text or images are processed by a service.
That fact alone does not establish public publication, model training, or an
infringement. Evaluate the actual use, applicable terms, and any statutory basis;
Taiwan's [Copyright Act](https://law.moj.gov.tw/LawClass/LawAll.aspx?PCode=J0070017),
including Article 65, calls for a contextual assessment of fair use.

Do not require an expressly named "AI license" for all reading or processing.
Missing AI-specific wording alone is not a prohibition. When an actual
restriction or material unresolved issue affects processing, record it and the
affected action. Return permitted metadata and locators when content cannot be
supplied, explain the restriction, and do not imply full content was delivered.

## Git safeguards and current limitations

The root ignore rules exclude library `local/` directories and `generated/`.
As the [Git documentation](https://git-scm.com/docs/gitignore) explains, ignore
rules do not affect files already tracked. They also do not stop force-adds or
remove historical copies.

Before any authorized staging or commit, manually inspect the actual staged
library files and staged rights records. Do not stage content with prohibited
or unresolved Git-storage permission or known local-only content, including
derivatives. Inspect the staged bytes and records, not only a more permissive
unstaged description. Check already tracked paths as well as new ones.

No staged-rights hook or automated rights validator is implemented.
Future checks must inspect the actual staged records and content; neither ignore
rules nor future checks can promise to prevent deliberate bypass or decide law
from a file hash.

Before migrating material from `research/references/`, audit its rights and
citations, preserve the original bytes and provenance, and update consuming links.
Record the assessment in the destination source package. Report restricted
material already in history and propose remediation; do not silently rewrite history.
