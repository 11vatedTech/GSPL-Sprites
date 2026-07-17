# Portable Sprite Package Specification 0.1

A package is created transactionally in a sibling staging directory and renamed
only after every artifact is written. Existing destinations are never
overwritten. A failed build removes its staging directory.

Required files are `manifest.json`, `seed.canonical.json`, `asset-graph.json`,
`provenance.json`, `rights.json`, and the declared visual artifacts. Entities
with rig semantics additionally contain `rig.json`, `animations.json`,
`collisions.json`, and, when present, `animation-state-graph.json`.

The manifest records the format, entity identity, seed identity, artifact paths
and SHA-256 hashes, and governance-file locations. The asset graph independently
records content hashes, dependencies, compiler passes, provenance identities,
targets, types, schema versions, and validation states. Paths are package-root
relative with `/` separators; future archive packaging must reject absolute,
device, traversal, link, and duplicate-normalized paths.

The `verify` command requires the canonical manifest grammar, a non-symlink
root, strictly sorted unique safe relative ASCII paths, no backslashes, drives,
empty components, or traversal, no symlink components, a closed declared file
set, required governance/visual artifacts, valid lowercase SHA-256 hashes, seed
identity agreement, an export-authorizing rights document, and configured
manifest/artifact/count/total-byte limits.
Directory traversal itself is independently bounded so a package containing a
large undeclared tree cannot evade the manifest artifact-count limit.

Full semantic asset-graph and provenance-closure validation remains a release
gate. The current verifier proves package containment, byte integrity, declared
identity, rights authorization, canonical manifest structure, and resource
bounds; it does not claim cryptographic authorship or trust without a future
signature policy.
