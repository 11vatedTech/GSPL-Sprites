# ADR-0001: Typed, open-versioned Sprite Genes

Status: accepted — 2026-07-17

## Decision

Use a closed `GeneKind` vocabulary for domain routing, stable string type IDs
and integer schema versions for serialization, and a typed payload variant for
implemented gene families. A registry owns dependencies, conflicts, relevance,
and purpose. Enum membership does not imply implementation support.

## Consequences

Unknown types and payload mismatches are fatal. New families require a typed
payload, descriptor, validation, lowering, migration policy, and test vectors.
This is more deliberate than an arbitrary property dictionary and prevents
silent target loss, while stable IDs and versions preserve an extension path.

