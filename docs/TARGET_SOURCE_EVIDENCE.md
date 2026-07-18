# Target source evidence

Engine and interchange targets must not become detached from the verified GSPL
package that authorized their export. `TargetSourceEvidence` binds four
lowercase SHA-256 identities: the package manifest, canonical seed, authoring
provenance artifact, and target-compatibility artifact.

Evidence is issued only by `target_source_evidence_from_package`. That function
runs the independent package verifier before reading and hashing the governed
artifacts. The evidence type has no public constructor, preventing target
adapters from accepting arbitrary caller-authored identity claims.

Godot projects store the canonical document as `source-evidence.json` and cover
it with the target manifest. GLB output stores the same document under
`asset.extras.gsplSourceEvidence`, which is standards-compliant application
metadata. Low-level projections without a package state `sourcePackage: null`
explicitly. Evidence proves content linkage, not signer identity; cryptographic
authorship remains a separate signature-policy milestone.
