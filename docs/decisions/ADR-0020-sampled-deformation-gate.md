# ADR-0020: Sampled skin-deformation gate

## Status

Accepted.

## Decision

Evaluate fixed-tick joint clips through bounded linear-blend skinning before
animated target export. Sample authored keys plus deterministic intermediate
ticks, and reject triangle-area collapse, excessive vertex displacement, or
workloads beyond configured evaluation limits.

## Consequences

Animation integrity is established from deformed geometry rather than track
syntax alone. Export cost increases predictably and has explicit bounds. The
gate is conservative and does not replace self-intersection, physics, silhouette,
or perceptual deformation analysis.
