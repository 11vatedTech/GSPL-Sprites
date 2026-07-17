# ADR-0002: Immutable content-addressed Asset Graph

Status: accepted — 2026-07-17

## Decision

Artifact identities hash type, schema, content hash, versioned compiler pass,
provenance identity, target, and sorted dependency identities. Dependencies
must exist before insertion. Repeated insertion of the same immutable node is
idempotent. Incremental invalidation uses the complete reverse dependency
closure.

## Consequences

Cycles cannot enter through the public API, unrelated artifacts are not
invalidated, and cache entries are independently verifiable. Model outputs are
ordinary artifacts whose model provenance participates in identity; optimized
device binaries remain derived target artifacts, never semantic authority.

