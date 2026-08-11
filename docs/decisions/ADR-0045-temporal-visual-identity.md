# ADR-0045: Temporal visual identity

## Status

Accepted.

## Decision

Every persistent visual structure (contour segment, marking, highlight,
line, stroke cluster, effect emitter, landmark) carries a stable semantic
identity across frames, assigned deterministically from canon/state —
never from frame-local analysis. Incoherence (smear frames, stepped
animation) is a typed deliberate behavior that changes realization, never
structure IDs.

## Rationale

The NPR temporal-coherence state of the art (Benard 2011) and production
practice (Spider-Verse rigged lines) show coherence comes from structure
identity and surface anchoring; per-frame regeneration produces flicker.
GSPL renders from semantic geometry, so object-space-style coherence is
achievable by construction.

## Consequences

Markings/lines/highlights bind to canon structures and keep IDs across
frames. Thresholded behaviors are anchored. Temporal fidelity metrics
(jitter, drift, duplication) consume ID-tracked structures.
