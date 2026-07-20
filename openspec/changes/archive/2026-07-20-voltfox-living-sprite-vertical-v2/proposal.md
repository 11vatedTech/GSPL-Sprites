# Proposal: Voltfox Living-Sprite Vertical

## Problem

The existing Voltfox implementation has extensive infrastructure but does not satisfy the full living-sprite vertical acceptance criteria. Key gaps remain in:
- Forms named `Idle`/`Running`/`Jumping`/`Attack`/`Special`/`Hurt` instead of `base`/`storm`
- Missing `ascend`/`descend` transformations with progressive visual change
- Storm form is palette-only, lacking morphological deltas
- Sprite IR lacks full semantic depth (materials, rig, sockets, animation tracks, abilities, effects, collision, runtime attributes, representation plans, target requirements)
- 2D animation not provably frame-distinct (no pixel hash invariants)
- 2.5D produces metadata but no actual rendered RGBA plane images
- 3D uses placeholder box-sphere primitives without meaningful topology, normals, or UVs
- No complete authoritative living-state identity
- No full behavior lifecycle (perception → decision → action)
- No immutable artifact graph with selective invalidation proofs
- No complete portable package with vertical closure
- No package-driven preview acceptance mode
- No resource limits enforcement
- No property/mutation testing

## Solution

Complete the Voltfox living-sprite vertical by:

1. **Seed refactor**: Replace 6 generic forms with `base`/`storm` semantic forms; add `ascend`/`descend` transformations with progressive deltas; storm form alters silhouette, ears, tail, electrical markings, aura, emissive surfaces, ability envelope, resource capacity, collision.

2. **Complete Sprite IR**: Expand `SpriteIr` to include entity identity, rights, semantic parts with hierarchy, forms, transformations, materials, palette, rig, sockets, animation intents+tracks, abilities, projectiles, effects, collision, runtime attributes, representation plans, target requirements. Immutable, versioned, deterministic, serializable, hashable, validated, inspectable, diffable.

3. **Rig-driven 2D**: Morphology-driven 2D renderer producing distinct frames per animation clip. Every frame ID with different semantic pose produces different pixel content. Pixel hash comparison proves distinctness. Current 64×64 ellipse renderer upgraded with shape primitives (polygons, tapered segments, curves, controlled outlines, masks, glow).

4. **Actual 2.5D plane images**: Produce content-addressed RGBA plane images for each depth layer. All plane IDs resolve to rendered assets. Palette, rig, pose, and form inputs materially affect output.

5. **Meaningful 3D geometry**: Replace box-sphere primitives with proper mesh topology (nondegenerate triangles, normals, UVs, material references). 13-bone skeleton preserved and enhanced. glTF export verified structurally.

6. **Complete authoritative identity**: Living-state identity includes entity definition, stable instance, current form, active transformation, transformation progress, tick, health, resource, cooldowns, statuses, ability, projectile, combo, behavior, deterministic event state. Excludes memory addresses, absolute paths, wall-clock, process IDs, render interpolation.

7. **Full behavior**: Perception → decision → action cycle. Perceive target, idle, orient, approach, attack, react to damage, respect cooldowns/resources, transform under condition, storm-form behavior, returns after save/restore.

8. **Complete combat pipeline**: Directional lightning with form gate, resource cost, cooldown, activation, animation, release event, muzzle origin, projectile/collision, damage, timed electrical status, visual manifestation, deterministic events. Anticipation → release → projectile → collision → damage → status → resource consumption → cooldown → recovery.

9. **Immutable artifact graph**: Content-addressed artifacts with dependencies, selective invalidation tests. Changing palette changes dependent visuals but not behavior. Changing ability timing changes animation events but not geometry. Changing storm morphology changes only storm artifacts.

10. **Complete portable package**: All vertical artifacts with deterministic ordering, complete dependency closure, no unresolved/duplicate IDs, checksum integrity, safe paths, archive safety. Build twice from identical inputs → identical identity.

11. **Package-driven preview**: Reference preview loads only portable package. 2D/2.5D/3D display, animation, combat, transformation, save/restore, headless acceptance mode with frame output, machine-readable state, artifact hashes, package identity, final authoritative identity.

12. **Resource limits**: Seed size, forms, transformations, bones, sockets, clips, tracks, frames, image dimensions, atlas, planes, vertices, indices, materials, collision shapes, projectiles, package files, package size, runtime entities, memory, execution time. Each with enforcement, diagnostic, boundary test, over-boundary rejection.

13. **Testing**: Property tests for deterministic canonicalization, state identity, serialization round-trip, save/restore, replay, cross-representation parity, package reproducibility, artifact-graph closure, selective invalidation. Fuzzing of parsing, forms, transformations, animation, collision, packages, PNGs, meshes, glTF, persistence. Mutation testing killing critical mutants.

14. **44 completion gates**: All conditions verified before archive.

## Scope

- Sources: `include/gspl_sprites/*.hpp`, `src/*.cpp`, `examples/voltfox.sprite`, `tests/*.cpp`
- No new external dependencies
- No network access
- MSVC `/W4 /WX /permissive-`, GCC `-Wall -Wextra -Wpedantic -Werror`, 0 warnings
