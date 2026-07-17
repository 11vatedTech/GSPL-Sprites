# Runtime persistence and replay

Living runtime state uses a canonical, bounded text representation. Every save
records the SHA-256 identity of the validated runtime program that owns it. A
snapshot therefore cannot be loaded against changed goals, actions, timings,
conditions, markers, or runtime limits without an explicit migration layer.

Serialization sorts map-like collections and writes integer state exactly. The
loader accepts only the canonical representation: unknown, missing, duplicate,
misordered, malformed, or out-of-range fields fail closed. Loaded state passes
the same semantic validation as live state, including memory sequence,
cooldown, active-action, tick, energy, and capacity invariants.

Replay begins from a validated state and applies strictly increasing,
tick-addressed frames. A frame may contain variable updates, perception
observations, and an action interruption. The runner advances every intervening
fixed tick, returns the ordered semantic event stream, and hashes the canonical
final state. Limits bound frames, ticks, and emitted events.

The persisted format is an internal `gspl.living-runtime-state/0.1` contract.
Format version changes require explicit readers and migrations; permissive
fallback parsing is prohibited. A final-state hash proves deterministic state
agreement for the same program and inputs, but is not a network protocol,
cryptographic authentication mechanism, rollback manager, or substitute for
periodic authoritative snapshots. Those replication layers remain future work.
