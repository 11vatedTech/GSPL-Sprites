# ADR-0038: Visual Canon representation

## Status

Accepted.

## Decision

Introduce a VisualCanon as the authoritative machine-readable description
of how an entity becomes visually recognizable: structural, proportion,
landmark, silhouette, surface, material, color, facial, marking,
attachment, deformation, resolution and identity-invariant sub-canons. The
canon is a derived manifestation-level authority within the Visual
Manifestation Identity taxonomy; CanonicalEntity remains the entity
semantic authority.

## Rationale

Model-sheet practice (Williams, Blair, Thomas & Johnston) shows identity is
a relational invariant set, not a drawing. Without an explicit canon,
visual consistency depends on either frozen images or per-entity code —
both structurally trap fidelity below the professional ceiling. A typed,
validated canon makes identity relational, measurable and compiler-
consumable.

## Consequences

Entities differ by canon data, never by compiler branches. New body plans,
styles and forms are data additions. Canon identity feeds Visual
Manifestation Identity. Authoring tools (future) write canon data.
Validation and determinism requirements are explicit and testable.
