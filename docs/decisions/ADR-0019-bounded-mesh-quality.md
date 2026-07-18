# ADR-0019: Bounded UV and tangent quality gate

## Status

Accepted.

## Decision

Analyze textured render meshes with bounded UV broad-phase and exact positive-
area clipping, validate normal orientation and UV degeneracy, and generate
explicit orthogonal tangent frames before GLB export.

## Consequences

Malformed parameterization and inconsistent shading fail before engine import,
while legitimate shared UV boundaries remain valid. Worst-case overlap workloads
have an explicit failure bound. Generated tangents are deterministic within the
fixed-point output contract; formal MikkTSpace equivalence remains future work.
