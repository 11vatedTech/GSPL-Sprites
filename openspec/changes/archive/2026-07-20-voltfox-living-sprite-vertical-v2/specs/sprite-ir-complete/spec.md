# Complete Typed Sprite IR

## Requirements
1. SpriteIr carries entity_id, name, classification, rights, provenance, schema version
2. Morphology with hierarchy tree (parent/child part relationships)
3. FormDefinitions with form-specific attributes (resource_capacity, collision_scale, ability_envelope)
4. TransformationDeltas with morphology overrides, duration_ticks, resource_cost
5. Palette entries with semantic names (primary, accent, storm_primary, storm_accent, emissive, aura)
6. Full RigDefinition, AnimationIntents with track data, compiled AbilitySeed list
7. ProjectileDefinition (origin_socket, speed, collision, damage, status)
8. Per-form collision shapes and windows
9. Runtime attribute defaults (health, resource, cooldowns, behavior states)
10. Immutable: versioned, deterministic, serializable, hashable

## Scenarios
1. Full Voltfox SpriteIr contains all semantic fields populated from seed
2. SpriteIr hash changes when any semantic field changes
3. SpriteIr serializes/deserializes round-trip with identical hash
4. Missing required field produces validation error
