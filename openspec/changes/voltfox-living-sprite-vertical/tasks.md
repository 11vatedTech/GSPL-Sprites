## Group 1: Seed Refactor — Base/Storm Forms and Ascend/Descend

- [ ] 1.1 Rewrite `examples/voltfox.sprite` with `[form.base]`, `[form.storm]`, `[transformation.ascend]`, `[transformation.descend]`
- [ ] 1.2 Add storm morphology deltas (elongated ears, forked tail, electrical markings, emissive aura, expanded collision)
- [ ] 1.3 Update `parse_seed()` to handle base/storm form syntax and ascend/descend transformations
- [ ] 1.4 Update `validate()` for new form/transformation constraints
- [ ] 1.5 Update `compile()` to produce base/storm FormDefinitions and ascend/descend TransformationDeltas
- [ ] 1.6 Add unit tests for new form/transformation parsing, validation, compilation
- [ ] 1.7 Verify GCC + MSVC strict builds

## Group 2: Complete Typed Sprite IR

- [ ] 2.1 Expand SpriteIr with: entity_id, name, classification, rights, provenance hash, schema version
- [ ] 2.2 Add hierarchy tree to MorphologyDefinition (parent/child part relationships)
- [ ] 2.3 Add ProjectileDefinition to SpriteIr (origin_socket, speed, collision, damage, status)
- [ ] 2.4 Add form-specific attributes: resource_capacity, collision_scale, ability_envelope
- [ ] 2.5 Add semantic palette entries to SpriteIr (primary, accent, storm_primary, storm_accent, emissive, aura)
- [ ] 2.6 Make SpriteIr immutable, versioned, serializable, hashable
- [ ] 2.7 Wire expanded SpriteIr through compile() to synthesis
- [ ] 2.8 Add unit tests for SpriteIr serialization, hashing, validation
- [ ] 2.9 Verify GCC + MSVC strict builds

## Group 3: Frame-Distinct 2D Synthesis

- [ ] 3.1 Add SHA-256 pixel hash to each generated SpriteFrame
- [ ] 3.2 Add shape primitives (polygons, tapered segments, curves, controlled outlines, masks, glow)
- [ ] 3.3 Upgrade morphology-driven renderer to produce provably distinct frames per animation pose
- [ ] 3.4 Generate alpha, outline, depth, emissive channel maps alongside RGBA
- [ ] 3.5 Implement ascend/descend progressive frame generation in 2D
- [ ] 3.6 Add frame metadata (pivots, sockets, collision AABB, animation events)
- [ ] 3.7 Add test: compare pixel hashes across frames — distinct IDs → distinct hashes
- [ ] 3.8 Add test: ascend early vs late frames have different hashes
- [ ] 3.9 Add test: muzzle socket position matches rendered muzzle
- [ ] 3.10 Verify GCC + MSVC strict builds

## Group 4: Actual 2.5D Rendered Plane Images

- [ ] 4.1 Extend synthesize_projection25d_voltfox() to produce ImageRgba8 buffers per depth plane
- [ ] 4.2 Compute SHA-256 hash for each plane image and store in metadata
- [ ] 4.3 Ensure every plane ID in metadata has corresponding rendered image
- [ ] 4.4 Make morphology parts drive plane content (torso, head, ears, tail, limbs, aura)
- [ ] 4.5 Palette input materially affects plane output
- [ ] 4.6 Storm form produces different plane content than base form
- [ ] 4.7 Stable depth intervals and parallax metadata per plane
- [ ] 4.8 Add tests: all plane IDs resolve, base vs storm plane hashes differ, palette change affects hashes
- [ ] 4.9 Verify GCC + MSVC strict builds

## Group 5: Proper 3D Mesh Topology

- [ ] 5.1 Upgrade primitive generators (add_box, add_sphere, add_cone, add_capsule, add_cylinder) to compute normals and UVs
- [ ] 5.2 Ensure all triangles are nondegenerate (nonzero area)
- [ ] 5.3 Add material references per triangle (body, head, eyes, aura, storm_markings)
- [ ] 5.4 Verify 13-bone skeleton hierarchy with valid bind-pose transforms
- [ ] 5.5 Verify per-vertex skinning weights sum to 65535
- [ ] 5.6 Generate additional animation clips: ascend, storm_idle, storm_attack, descend
- [ ] 5.7 Validate glTF export structure
- [ ] 5.8 Add tests: no degenerate triangles, unit-length normals, weight sums, glTF structural correctness
- [ ] 5.9 Verify GCC + MSVC strict builds

## Group 6: Authoritative Runtime Identity and Full Behavior

- [ ] 6.1 Define EntityStateIdentity struct with all authoritative fields
- [ ] 6.2 Integrate EntityStateIdentity into LivingRuntimeState
- [ ] 6.3 Exclude non-authoritative state from identity (memory addresses, paths, wall-clock, process IDs, render interpolation)
- [ ] 6.4 Implement perceive() → decide() → act() behavior lifecycle
- [ ] 6.5 Implement target acquisition, distance assessment, damage awareness
- [ ] 6.6 Implement utility-based decision considering cooldowns, resources, range, form
- [ ] 6.7 Implement transform-under-condition behavior (health threshold or command)
- [ ] 6.8 Verify identity serializes/deserializes round-trip with identical hash
- [ ] 6.9 Verify identity matches across 2D/2.5D/3D at same tick
- [ ] 6.10 Verify behavior resumes from saved state after restore
- [ ] 6.11 Add tests for behavior lifecycle, identity round-trip, cross-rep identity
- [ ] 6.12 Verify GCC + MSVC strict builds

## Group 7: Complete Combat Pipeline

- [ ] 7.1 Implement form-gated directional_lightning (usable in storm form)
- [ ] 7.2 Implement full ability lifecycle: activation → anticipation → release → projectile → collision → damage → status → cooldown
- [ ] 7.3 Projectile originates from evaluated muzzle socket
- [ ] 7.4 Bolt collision tested against target hurtbox
- [ ] 7.5 Numeric damage applied on collision
- [ ] 7.6 Timed electrical status with duration_ticks
- [ ] 7.7 Every stage produces deterministic RuntimeEvent
- [ ] 7.8 Early interruption cancels activation without resource cost
- [ ] 7.9 Invalid target rejection without resource consumption
- [ ] 7.10 Form mismatch produces diagnostic
- [ ] 7.11 Add tests for full lifecycle, cancellation, form rejection, damage values, status duration
- [ ] 7.12 Verify GCC + MSVC strict builds

## Group 8: Immutable Artifact Graph

- [ ] 8.1 Extend AssetGraph with content-addressed artifact nodes (SHA-256 keyed)
- [ ] 8.2 Add artifact metadata: type, schema_version, producing_pass, dependencies, semantic_owner, form, representation, validation_state, provenance, determinism_class
- [ ] 8.3 Wire artifact graph through synthesis pipeline (each output becomes artifact node)
- [ ] 8.4 Implement selective invalidation: changing input invalidates only transitive dependents
- [ ] 8.5 Wire artifact graph into package building
- [ ] 8.6 Add tests: palette change invalidates visuals but not behavior, ability timing change invalidates events but not geometry, storm morphology change invalidates only storm artifacts
- [ ] 8.7 Add test: all vertical artifacts reachable from root
- [ ] 8.8 Verify GCC + MSVC strict builds

## Group 9: Complete Portable Package

- [ ] 9.1 Extend build_package() to include all vertical artifacts
- [ ] 9.2 Implement deterministic file ordering for reproducible package identity
- [ ] 9.3 Verify complete dependency closure (no unresolved IDs)
- [ ] 9.4 Verify no duplicate artifact IDs
- [ ] 9.5 Verify checksum integrity of all artifacts
- [ ] 9.6 Implement safe path validation
- [ ] 9.7 Add tests: identical-input reproducibility, per-category corruption rejection, missing dependency rejection
- [ ] 9.8 Verify GCC + MSVC strict builds

## Group 10: Package-Driven Preview and Headless Acceptance

- [ ] 10.1 Refactor preview.cpp to load Voltfox from portable package
- [ ] 10.2 Run package verification before load
- [ ] 10.3 Ensure 2D/2.5D/3D rendering uses only package assets
- [ ] 10.4 Wire animation playback (idle, locomotion, attack) from package assets
- [ ] 10.5 Wire directional lightning visual feedback with muzzle origin
- [ ] 10.6 Wire ascend/descend progression rendering
- [ ] 10.7 Implement save/restore in preview (save state, destroy, restore, verify identity)
- [ ] 10.8 Implement headless acceptance mode with frame output, state, hashes, identity
- [ ] 10.9 Add test verifying preview builds (non-functional)
- [ ] 10.10 Verify GCC + MSVC strict builds

## Group 11: Resource Limits Enforcement

- [ ] 11.1 Define constexpr limits in headers (seed size, forms, transformations, bones, sockets, clips, frames, planes, vertices, package size, entities)
- [ ] 11.2 Enforce limits in parse_seed() and validate() and compile()
- [ ] 11.3 Add diagnostics with limit name, current value, maximum
- [ ] 11.4 Add boundary tests (at-limit accepted) for each limit
- [ ] 11.5 Add over-boundary rejection tests for each limit
- [ ] 11.6 Verify GCC + MSVC strict builds

## Group 12: Property, Mutation, and Fuzzing Tests

- [ ] 12.1 Add deterministic canonicalization property test (100 random seeds)
- [ ] 12.2 Add state identity round-trip property test
- [ ] 12.3 Add save/restore property test with identity comparison
- [ ] 12.4 Add replay property test (record, replay, compare final identity)
- [ ] 12.5 Add cross-representation parity property test
- [ ] 12.6 Add package reproducibility property test
- [ ] 12.7 Add artifact-graph closure property test
- [ ] 12.8 Add selective invalidation property test
- [ ] 12.9 Implement and kill 12 critical mutants
- [ ] 12.10 Extend fuzz parsing with 30-second time-boxed runs
- [ ] 12.11 Verify GCC + MSVC strict builds

## Group 13: Final Verification and Archive

- [ ] 13.1 Run full CTest suite under GCC, fix all failures
- [ ] 13.2 Run full CTest suite under MSVC, fix all failures
- [ ] 13.3 Run fuzz harness for 30 seconds under both compilers
- [ ] 13.4 Build preview target under both compilers
- [ ] 13.5 Final review: 0 warnings, 0 leaks, 44 gates pass
- [ ] 13.6 Update documentation to match implementation
- [ ] 13.7 Git commit and push
- [ ] 13.8 Archive the change
