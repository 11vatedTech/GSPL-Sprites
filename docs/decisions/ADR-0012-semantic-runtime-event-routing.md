# ADR-0012: Semantic runtime event routing

## Status

Accepted.

## Decision

Route ordered living-runtime events through validated engine-neutral bindings
to typed animation, combat, effect, and audio commands. Preserve event identity
on every command, impose explicit fan-out bounds, and commit the routing cursor
only after the complete batch succeeds.

## Consequences

Presentation systems remain projections of semantic runtime authority, combat
does not depend on visual frame timing, and target adapters share one replayable
command contract. Consumers still require concrete implementations and durable
side-effect idempotency; the router itself does not play animation or audio,
spawn effects, or apply damage.
