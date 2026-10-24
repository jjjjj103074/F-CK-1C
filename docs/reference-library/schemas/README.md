# Record contracts

These JSON Schemas describe structured library records. They do not download,
register, read, or legally approve a source. This phase provides contracts and
instructions; the library parser and cross-record validator are not implemented.

See [FORMATS.md](../FORMATS.md) for serialization, locators, identifiers, and
the checks that JSON Schema cannot establish on its own.

## Offline schema loading

Load both files into a local JSON Schema Draft 2020-12 registry, using the `$id`
declared in each file. References use those IDs; do not fetch a network schema.
Enable the `date`, `date-time`, and `uri` format assertions in the validator.

| Contract | What to validate |
|---|---|
| [source.schema.json](source.schema.json) root | Parsed `source.yaml` |
| `source.schema.json#/$defs/guide_metadata` | A guide's parsed YAML front matter |
| [knowledge.schema.json](knowledge.schema.json) root | Canonical topic or local supplement front matter |
| `knowledge.schema.json#/$defs/entry` | A knowledge entry's parsed YAML block |

Read the actual `$defs` names and `$id` values in the files rather than treating
this table as a second schema. A general validator can evaluate these contracts;
there is no project-specific validation command in this foundation.

## Additional verification

Before treating a record as checked, also inspect its references and content:

- IDs are unique in their documented scope and targets exist.
- Each PDF locator addresses the exact asset, uses one-based pages, and has an
  inclusive end not earlier than its start or beyond the real page count.
- Video time ranges are ordered and match the actual source.
- File paths remain inside their source package; actual hashes and availability
  agree with the record. Do not access files by following untrusted paths.
- Topic relationships and entry citations match their declared scopes.
- Guides and knowledge sections match their metadata and the actual source.
- Permission decisions have applicable evidence and satisfy their conditions.

These are manual obligations until an implemented tool checks them. Schema
validation is structural evidence, not a substitute for source or rights review.
