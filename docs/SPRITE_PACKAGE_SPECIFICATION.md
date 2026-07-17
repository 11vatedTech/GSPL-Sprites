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

Package verification is a pending completion gate: release packages may not be
accepted from untrusted storage until a reader validates schema, paths, hashes,
graph closure, provenance closure, rights authorization, and resource limits.

