# ADR-0004: Semantic animation and collision timing

Status: accepted — 2026-07-17

## Decision

Represent rig topology, clips, state transitions, sockets, skin weights, and
collision windows as typed engine-neutral data. Time is integer simulation
ticks. Transition priority is explicit and unique within a state. Damage windows
must be contained by their semantic ability timing.

## Consequences

Animation can be validated independently of a target engine, deterministic
simulation does not depend on floating wall-clock time, and target adapters
cannot infer gameplay rules from filenames or visual frames.

