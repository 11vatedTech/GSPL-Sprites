# ADR-0039: Semantic structure graph

## Status

Accepted.

## Decision

Represent anatomy as a general typed semantic hierarchy: regions →
elements/joints/masses/appendages, with relationship kinds (parent, child,
attachment, articulation, adjacency, symmetry, correspondence,
containment, overlap, surface_ownership). No body plan is hardcoded;
bipeds, quadrupeds, serpentines, insects, machines, vehicles and abstract
entities differ only in structure data.

## Rationale

Comparative anatomy (Ellenberger) and creature construction (Whitlatch)
show believability comes from functional structure, not part lists.
Humanoid-templated rigs would force non-human entities into foreign
abstractions and make generalization fixtures (fox, humanoid, mech, flyer)
impossible to share a compiler.

## Consequences

Every other canon binds to structures by id. Validation rejects cycles,
dangling refs and ill-typed relationships. Retargeting (future) works by
role correspondence across structure graphs.
