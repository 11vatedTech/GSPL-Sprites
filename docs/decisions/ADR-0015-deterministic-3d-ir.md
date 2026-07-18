# ADR-0015: Deterministic validated 3D geometry IR

## Status

Accepted.

## Decision

Represent production 3D projections with bounded indexed geometry, PBR
materials, skeletons, fixed-point skin weights, morph deltas, collision meshes,
and monotonic LOD contracts. Validate geometry and hierarchy directly before
interchange export, using canonical integer units.

## Consequences

Malformed or over-budget meshes, invalid topology, broken rigs, incomplete
morphs, and false LODs fail before reaching an engine importer. Canonical builds
do not depend on platform float text. The fixed-point IR is not itself glTF and
requires a checked conversion/export layer; advanced UV, tangent, deformation,
animation, retargeting, and optimization validation remains required.
