# Rights and Provenance Standard 0.1

Rights are evaluated against an intended use; a classification alone is never
an authorization. Prohibited and unknown inputs are denied for every use.
Restricted inputs are limited to private study. Research-only inputs permit
private study and research but deny commercial export and model training.
User ownership permits production export but does not implicitly grant model
training rights. Original, licensed, public-domain, and permissively licensed
inputs permit uses governed by their accompanying terms.

Every compiler-derived artifact has a provenance record containing a stable
record ID, actor class and identity, versioned operation, sorted input artifact
identities, and output identity. Generated-model actors additionally require a
model descriptor with source, version, license, file hash, task contract,
device, precision, and determinism class before their output can be valid.

The portable package includes `rights.json`, `provenance.json`, and
`asset-graph.json`. Absence or inconsistency of any of these records is a package
verification failure. Operational telemetry and wall-clock time are excluded
from canonical identities and provenance payloads.

