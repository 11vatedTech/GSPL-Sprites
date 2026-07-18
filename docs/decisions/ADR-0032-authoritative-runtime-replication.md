# ADR-0032: Hash-bound authoritative runtime replication

## Status

Accepted.

## Decision

Replicate canonical full runtime states using an update bound to the validated
program identity, exact base-state hash, target-state hash, monotonic target
tick, and explicit payload length. Apply an update transactionally only after
all identities, bounds, canonical parsing, runtime invariants, and tick rules
pass.

## Rationale

The canonical state already defines deterministic runtime authority. Reusing it
avoids a second partial state schema and makes convergence independently
verifiable. Base binding detects divergent prediction or an update applied out
of order. Full snapshots are intentionally chosen before deltas: they establish
a correct recovery primitive on which bandwidth optimizations can later depend.

## Consequences

Replicas can fail closed and converge without renderer or engine ownership of
semantic state. The update is transport-neutral and bounded, but it is not
authenticated; deployment transports must provide peer authentication and
integrity. Delta compression, prediction, and rollback simulation require
separate measured designs and cannot weaken snapshot verification.
