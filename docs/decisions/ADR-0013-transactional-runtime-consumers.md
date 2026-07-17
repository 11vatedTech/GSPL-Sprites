# ADR-0013: Transactional replay-safe runtime consumers

## Status

Accepted.

## Decision

Assign a contiguous sequence to every routed consumer command and apply command
batches to engine-neutral preview state transactionally. Model animation as
current state and combat, effect, and audio work as explicitly bounded pending
emissions retaining their originating event identity.

## Consequences

Fan-out from one semantic event is replayable without conflating event and
command identity. Invalid order or exhausted capacity cannot leave partially
advanced cursors or partially enqueued side effects. Concrete engine adapters
must drain and durably checkpoint emissions; this layer intentionally does not
claim that presentation or damage side effects have executed.
