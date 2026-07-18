# ADR-0033: Typed transactional combat authority

## Status

Accepted.

## Decision

Represent combat abilities and effects as bounded typed domain structures and
execute commands transactionally at deterministic integer ticks. Keep combat
state authoritative and independent from visual, animation, audio, and engine
adapters.

## Rationale

Animation markers can request a hit but cannot define gameplay truth. Explicit
target, range, resource, cooldown, health, and status semantics make ability
behavior testable, portable, and reproducible. Candidate-state execution
prevents multi-effect abilities from leaving partial mutations after a later
effect fails.

## Consequences

Damage, healing, and timed statuses now have a stable runtime boundary and
ordered event evidence. Spatial collision adapters and combo/transformation
systems can integrate without owning health or cooldown state. The current
Euclidean point-range rule is intentionally conservative; shaped areas and
physics hit confirmation require explicit future contracts.
