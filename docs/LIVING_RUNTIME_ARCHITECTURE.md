# Living runtime architecture

The engine-neutral living runtime advances in explicit integer ticks. Its first
operational policy combines deterministic rules and utility scoring; it is not
coupled to rendering, physics, or one game engine.

A program declares bounded goals, activation conditions, actions, utility terms,
preconditions, durations, cooldowns, energy costs, interruptibility, and event
markers. Conditions read an ordered integer blackboard. Utility uses signed
integer arithmetic with parts-per-million weights. Higher scores win; stable
action identity breaks ties lexicographically, so input order cannot change a
decision.

Perception observations carry source entity, value, confidence, observed tick,
and lifetime. Memory is deterministically ordered and bounded. Oldest records
are evicted first; expiration or eviction refreshes the latest surviving value
for that perception key or removes its blackboard entries. No expired fact
silently remains authoritative.

Action admission checks active goals, rule preconditions, energy, and cooldown.
Starting an action consumes energy and emits an ordered event. Execution emits
semantic markers and completion; explicitly interruptible actions can emit an
interruption event. Event and observation sequences are monotonically assigned
within runtime state.

Program and state validation is fail closed. Limits cover goals, actions,
conditions, utility terms, markers, variables, memory, tick rate, action timing,
cooldowns, energy, and snapshot consistency. Runtime arithmetic avoids wall
clock time and floating point. Save/load, replay hashes, deterministic
replication, behavior-tree/GOAP adapters, navigation, and effect/audio consumers
remain subsequent runtime layers.
