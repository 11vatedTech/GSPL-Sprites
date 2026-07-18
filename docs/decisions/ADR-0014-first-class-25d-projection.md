# ADR-0014: First-class deterministic 2.5D projection

## Status

Accepted.

## Decision

Model 2.5D as a typed projection of depth planes, angular views, multi-plane rig
bindings, lighting channels, depth collision, and optional hybrid geometry.
Use integer millimeters, microunits, millidegrees, and parts-per-million for
canonical and runtime calculations.

## Consequences

2.5D has enforceable semantics distinct from flat 2D and complete 3D. View and
plane completeness, generated-view lineage, lighting inputs, collision depth,
and hybrid attachments can fail closed before target export. Providers still
need to generate the referenced assets, and target adapters must implement the
camera deformation, lighting, billboard, and hybrid draw behavior.
