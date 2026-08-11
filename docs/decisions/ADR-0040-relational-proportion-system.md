# ADR-0040: Relational proportion system

## Status

Accepted.

## Decision

Encode proportions relationally as ratios between landmark/part measures
with preferred value, allowed range, deformation range, style range and
hard flag. Absolute pixel coordinates are never canonical truth.

## Rationale

Model sheets express proportions in modular units (head heights) precisely
because ratios survive scale, projection and restyle. Absolute
coordinates break identity across resolutions and representations.

## Consequences

Proportion validation is deterministic (preferred within allowed range;
deformation contains allowed). Canonicalization and identity are stable.
A pose or projection may change absolute geometry but never ratios outside
declared ranges.
