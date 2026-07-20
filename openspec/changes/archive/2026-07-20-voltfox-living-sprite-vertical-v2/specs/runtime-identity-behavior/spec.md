# Authoritative Runtime Identity and Full Behavior

## Requirements
1. EntityStateIdentity: entity_def_id, instance_id, form, transformation, tick, health, resource, cooldowns, statuses, ability, projectile, combo, behavior, deterministic_event_state
2. Excluded from identity: memory addresses, paths, wall-clock, process IDs, render interpolation, device state
3. Single identity across 2D/2.5D/3D manifestations
4. Behavior lifecycle: perceive(target) → decide(utility) → act(approach/attack/transform)
5. Perception: target acquisition, distance, damage awareness, status awareness
6. Decision: utility-based selection considering cooldowns, resources, range, form
7. Action: move, idle, attack, ascend, descend, use ability
8. Separation: immutable definition, mutable state, perception input, decision output, command

## Scenarios
1. EntityStateIdentity serializes/deserializes round-trip with identical hash
2. Identity matches across 2D/2.5D/3D at same tick
3. Voltfox idles then attacks when target in range and cooldown expired
4. Voltfox transforms under health-threshold condition
5. After save/restore, behavior resumes from saved state
6. Replay produces identical event sequence
