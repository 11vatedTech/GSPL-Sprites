# ADR-0035: Identity-preserving transformation deltas

## Status

Accepted.

## Decision

Model transformations as timed transitions between typed forms under one stable
entity identity. Apply form differences as deterministic semantic deltas and
commit transformation history with affected combat state transactionally.

## Rationale

Replacing an entity wholesale loses identity, provenance, and runtime
continuity. Explicit forms and transitions make changed and preserved semantics
auditable. Delta application avoids accumulating mutations across repeated form
changes, while explicit reverse edges prevent ambiguous reversibility.

## Consequences

Combat capacity and form-specific ability changes are now authoritative and
reversible without drift. Other gene families need equivalent typed deltas.
Persistence, replication, visual manifestation, and animation transition layers
must bind to this form authority rather than maintaining independent form state.
