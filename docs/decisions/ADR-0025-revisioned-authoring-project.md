# ADR-0025: Revisioned authoring state before canonical seeds

- Status: Accepted
- Date: 2026-07-17

## Context

Canonical Sprite Seeds are immutable and fully validated, while real creation contains incomplete choices, alternatives, locks, and variants. Editing the seed directly would either reject useful intermediate states or weaken canonical invariants.

## Decision

Introduce a separate versioned authoring project. Canonicalize its complete state for revision identity, use optimistic concurrency for edits, require explicit unlock revisions, retain unresolved mandatory choices, and lower only to a fully valid seed. Variants operate only within declared alternatives and cannot override locks.

## Consequences

The authoring UI can preserve uncertainty without contaminating production semantics. Regeneration and collaboration gain deterministic conflict detection and ancestry. Every newly authorable domain still requires a typed field or node contract plus lowering and validation; generic property bags are not admitted.
