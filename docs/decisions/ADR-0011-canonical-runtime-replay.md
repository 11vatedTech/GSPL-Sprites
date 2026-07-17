# ADR-0011: Canonical hash-bound runtime persistence and replay

## Status

Accepted.

## Decision

Persist living runtime snapshots in a strict canonical representation bound to
the SHA-256 identity of the complete validated runtime program. Replay applies
bounded, strictly tick-ordered input frames through the ordinary runtime and
returns ordered events plus a canonical final-state identity.

## Consequences

Save data cannot silently drift across behavior-program changes. Equivalent
state has one serialized form, malformed snapshots fail closed, and deterministic
replays can compare both event streams and terminal state. Schema evolution now
requires an explicit migration path. Network transport, authentication,
rollback prediction, and authoritative replication are deliberately separate
layers and are not implied by replay determinism.
