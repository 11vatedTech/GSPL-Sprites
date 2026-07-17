# Asset Graph Specification 0.1

Every artifact node records a stable identity, type, schema version, sorted
dependencies, compiler pass, provenance identity, target, content hash, and
validation state. Dependencies must already exist, which makes cycles
unrepresentable through the public API. IDs hash semantic metadata plus content
and dependency identities. Re-adding an identical artifact is idempotent;
identity collisions with different immutable data are fatal.

Incremental invalidation is the deterministic transitive reverse dependency
closure. Canonical manifests sort nodes by identity and dependencies by
identity. Artifact bytes live outside the graph; storage and cache layers must
verify their bytes against `content_hash` before use.

