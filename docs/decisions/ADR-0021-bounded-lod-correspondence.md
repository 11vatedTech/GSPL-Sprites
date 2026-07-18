# ADR-0021: Bounded geometric LOD correspondence

## Status

Accepted.

## Decision

Gate LOD export with symmetric vertex-and-centroid to triangle-surface distance,
an explicit evaluation budget, absolute geometric-error policy, and material
compatibility. Preserve level and coverage thresholds as GSPL target metadata.

## Consequences

Lower triangle counts cannot masquerade as valid LODs after losing too much
shape or changing material identity. The metric is deterministic and bounded,
but remains a sampled approximation; animated and perceptual LOD equivalence
requires additional validation.
