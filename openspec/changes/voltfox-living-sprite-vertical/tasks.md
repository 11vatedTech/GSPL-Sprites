## Group 1: Seed Refactor — Base/Storm Forms and Ascend/Descend

- [x] 1.1 Rewrite `examples/voltfox.sprite` with `[form.base]`, `[form.storm]`, `[transformation.ascend]`, `[transformation.descend]`
- [x] 1.2 Add storm morphology deltas (elongated ears, forked tail, electrical markings, emissive aura, expanded collision)
- [x] 1.3 Update `parse_seed()` to handle base/storm form syntax and ascend/descend transformations
- [x] 1.4 Update `validate()` for new form/transformation constraints
- [x] 1.5 Update `compile()` to produce base/storm FormDefinitions and ascend/descend TransformationDeltas
- [x] 1.6 Add unit tests for new form/transformation parsing, validation, compilation
- [x] 1.7 Verify GCC + MSVC strict builds

## Group 2: Complete Typed Sprite IR

- [x] 2.1 Expand SpriteIr with: entity_id, name, classification, rights, provenance hash, schema version
- [x] 2.2 Add hierarchy tree to MorphologyDefinition (parent/child part relationships)
- [x] 2.3 Add ProjectileDefinition to SpriteIr (origin_socket, speed, collision, damage, status)
- [x] 2.4 Add form-specific attributes: resource_capacity, collision_scale, ability_envelope
- [x] 2.5 Add semantic palette entries to SpriteIr (primary, accent, storm_primary, storm_accent, emissive, aura)
- [x] 2.6 Make SpriteIr immutable, versioned, serializable, hashable
- [x] 2.7 Wire expanded SpriteIr through compile() to synthesis
- [x] 2.8 Add unit tests for SpriteIr serialization, hashing, validation
- [x] 2.9 Verify GCC + MSVC strict builds

## Group 3: Frame-Distinct 2D Synthesis

- [x] 3.1 Add SHA-256 pixel hash to each generated SpriteFrame
- [x] 3.2 Add shape primitives (polygons, tapered segments, curves, controlled outlines, masks, glow)
- [x] 3.3 Upgrade morphology-driven renderer to produce provably distinct frames per animation pose
- [x] 3.4 Generate alpha, outline, depth, emissive channel maps alongside RGBA
- [x] 3.5 Implement ascend/descend progressive frame generation in 2D
- [x] 3.6 Add frame metadata (pivots, sockets, collision AABB, animation events)
- [x] 3.7 Add test: compare pixel hashes across frames — distinct IDs → distinct hashes
- [x] 3.8 Add test: ascend early vs late frames have different hashes
- [x] 3.9 Add test: muzzle socket position matches rendered muzzle
- [x] 3.10 Verify GCC + MSVC strict builds

## Group 4: Actual 2.5D Rendered Plane Images

- [x] 4.1 Extend synthesize_projection25d_voltfox() to produce ImageRgba8 buffers per depth plane
- [x] 4.2 Compute SHA-256 hash for each plane image and store in metadata
- [x] 4.3 Ensure every plane ID in metadata has corresponding rendered image
- [x] 4.4 Make morphology parts drive plane content (torso, head, ears, tail, limbs, aura)
- [x] 4.5 Palette input materially affects plane output
- [x] 4.6 Storm form produces different plane content than base form
- [x] 4.7 Stable depth intervals and parallax metadata per plane
- [x] 4.8 Add tests: all plane IDs resolve, base vs storm plane hashes differ, palette change affects hashes
- [x] 4.9 Verify GCC + MSVC strict builds

## Group 5: Proper 3D Mesh Topology

- [x] 5.1 Upgrade primitive generators (add_box, add_sphere, add_cone, add_capsule, add_cylinder) to compute normals and UVs
- [x] 5.2 Ensure all triangles are nondegenerate (nonzero area)
- [x] 5.3 Add material references per triangle (body, head, eyes, aura, storm_markings)
- [x] 5.4 Verify 13-bone skeleton hierarchy with valid bind-pose transforms
- [x] 5.5 Verify per-vertex skinning weights sum to 65535
- [x] 5.6 Generate additional animation clips: ascend, storm_idle, storm_attack, descend
- [x] 5.7 Validate glTF export structure
- [x] 5.8 Add tests: no degenerate triangles, unit-length normals, weight sums, glTF structural correctness
- [x] 5.9 Verify GCC + MSVC strict builds

## Group 6: Authoritative Runtime Identity and Full Behavior

- [x] 6.1 Define EntityStateIdentity struct with all authoritative fields
- [x] 6.2 Integrate EntityStateIdentity into LivingRuntimeState
- [x] 6.3 Exclude non-authoritative state from identity (memory addresses, paths, wall-clock, process IDs, render interpolation)
- [x] 6.4 Implement perceive() → decide() → act() behavior lifecycle
- [x] 6.5 Implement target acquisition, distance assessment, damage awareness
- [x] 6.6 Implement utility-based decision considering cooldowns, resources, range, form
- [x] 6.7 Implement transform-under-condition behavior (health threshold or command)
- [x] 6.8 Verify identity serializes/deserializes round-trip with identical hash
- [x] 6.9 Verify identity matches across 2D/2.5D/3D at same tick
- [x] 6.10 Verify behavior resumes from saved state after restore
- [x] 6.11 Add tests for behavior lifecycle, identity round-trip, cross-rep identity
- [x] 6.12 Verify GCC + MSVC strict builds

## Group 7: Complete Combat Pipeline

- [x] 7.1 Implement form-gated directional_lightning (usable in storm form)
- [x] 7.2 Implement full ability lifecycle: activation → anticipation → release → projectile → collision → damage → status → cooldown
- [x] 7.3 Projectile originates from evaluated muzzle socket
- [x] 7.4 Bolt collision tested against target hurtbox
- [x] 7.5 Numeric damage applied on collision
- [x] 7.6 Timed electrical status with duration_ticks
- [x] 7.7 Every stage produces deterministic RuntimeEvent
- [x] 7.8 Early interruption cancels activation without resource cost
- [x] 7.9 Invalid target rejection without resource consumption
- [x] 7.10 Form mismatch produces diagnostic
- [x] 7.11 Add tests for full lifecycle, cancellation, form rejection, damage values, status duration
- [x] 7.12 Verify GCC + MSVC strict builds

## Group 8: Immutable Artifact Graph

- [x] 8.1 Extend AssetGraph with content-addressed artifact nodes (SHA-256 keyed)
- [x] 8.2 Add artifact metadata: type, schema_version, producing_pass, dependencies, semantic_owner, form, representation, validation_state, provenance, determinism_class
- [x] 8.3 Wire artifact graph through synthesis pipeline (each output becomes artifact node)
- [x] 8.4 Implement selective invalidation: changing input invalidates only transitive dependents
- [x] 8.5 Wire artifact graph into package building
- [x] 8.6 Add tests: palette change invalidates visuals but not behavior, ability timing change invalidates events but not geometry, storm morphology change invalidates only storm artifacts
- [x] 8.7 Add test: all vertical artifacts reachable from root
- [x] 8.8 Verify GCC + MSVC strict builds

## Group 9: Complete Portable Package

- [x] 9.1 Extend build_package() to include all vertical artifacts
- [x] 9.2 Implement deterministic file ordering for reproducible package identity
- [x] 9.3 Verify complete dependency closure (no unresolved IDs)
- [x] 9.4 Verify no duplicate artifact IDs
- [x] 9.5 Verify checksum integrity of all artifacts
- [x] 9.6 Implement safe path validation
- [x] 9.7 Add tests: identical-input reproducibility, per-category corruption rejection, missing dependency rejection
- [x] 9.8 Verify GCC + MSVC strict builds

## Group 10: Package-Driven Preview and Headless Acceptance

- [x] 10.1 Refactor preview.cpp to load Voltfox from portable package
- [x] 10.2 Run package verification before load
- [x] 10.3 Ensure 2D/2.5D/3D rendering uses only package assets
- [x] 10.4 Wire animation playback (idle, locomotion, attack) from package assets
- [x] 10.5 Wire directional lightning visual feedback with muzzle origin
- [x] 10.6 Wire ascend/descend progression rendering
- [x] 10.7 Implement save/restore in preview (save state, destroy, restore, verify identity)
- [x] 10.8 Implement headless acceptance mode with frame output, state, hashes, identity
- [x] 10.9 Add test verifying preview builds (non-functional)
- [x] 10.10 Verify GCC + MSVC strict builds

## Group 11: Resource Limits Enforcement

- [x] 11.1 Define constexpr limits in headers (seed size, forms, transformations, bones, sockets, clips, frames, planes, vertices, package size, entities)
- [x] 11.2 Enforce limits in parse_seed() and validate() and compile()
- [x] 11.3 Add diagnostics with limit name, current value, maximum
- [x] 11.4 Add boundary tests (at-limit accepted) for each limit
- [x] 11.5 Add over-boundary rejection tests for each limit
- [x] 11.6 Verify GCC + MSVC strict builds

## Group 12: Property, Mutation, and Fuzzing Tests

- [x] 12.1 Add deterministic canonicalization property test (100 random seeds)
- [x] 12.2 Add state identity round-trip property test
- [x] 12.3 Add save/restore property test with identity comparison
- [x] 12.4 Add replay property test (record, replay, compare final identity)
- [x] 12.5 Add cross-representation parity property test
- [x] 12.6 Add package reproducibility property test
- [x] 12.7 Add artifact-graph closure property test
- [x] 12.8 Add selective invalidation property test
- [x] 12.9 Implement and kill 12 critical mutants
- [x] 12.10 Extend fuzz parsing with 30-second time-boxed runs
- [x] 12.11 Verify GCC + MSVC strict builds

## Group 13: Final Verification and Archive

- [x] 13.1 Run full CTest suite under GCC, fix all failures
- [x] 13.2 Run full CTest suite under MSVC, fix all failures
- [x] 13.3 Run fuzz harness for 30 seconds under both compilers
- [x] 13.4 Build preview target under both compilers
- [x] 13.5 Final review: 0 warnings, 0 leaks, 44 gates pass
- [x] 13.6 Update documentation to match implementation
- [x] 13.7 Git commit and push
- [x] 13.8 Archive the change
