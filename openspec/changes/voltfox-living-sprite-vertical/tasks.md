## 1. Seed Language + Compilation Pipeline

- [x] 1.1 Extend SpriteSeed with form/transformation/morphology/runtime fields in `core.hpp`
- [x] 1.2 Add FormDefinition, TransformationDelta, MorphologyDefinition, AnimationIntent structs to `core.hpp`
- [x] 1.3 Extend `parse_seed()` for `[form.<name>]`, `[transformation.<name>]`, `[morphology.<part>]`, `[runtime]` sections
- [x] 1.4 Extend `validate()` for form/transformation/morphology constraints
- [x] 1.5 Extend `compile()` to populate FormDefinition/TransformationDelta/MorphologyDefinition/AnimationIntent in SpriteIr
- [x] 1.6 Add form graph connectivity validation in compile()
- [x] 1.7 Rewrite `examples/voltfox.sprite` with full form/transformation/morphology/runtime declarations
- [x] 1.8 Add unit tests for seed parsing of forms, transformations, morphology, runtime
- [x] 1.9 Add unit tests for compile-time form graph validation
- [ ] 1.10 Verify MSVC strict build

## 2. 3D Voltfox Mesh, Skeleton, Skin, and glTF Export

- [ ] 2.1 Add `synthesize_projection3d_voltfox()` declaration to `synthesis.hpp`
- [ ] 2.2 Implement 8-primitive mesh assembly from morphology (box, cap, sphere, cone, capsule, cylinder)
- [ ] 2.3 Implement 13-bone skeleton hierarchy with bind-pose transforms
- [ ] 2.4 Implement per-vertex skinning weights (0-4 influences)
- [ ] 2.5 Implement linear-blend-skin animation clips with keyframes
- [ ] 2.6 Implement glTF 2.0 export (mesh, skin, skeleton, animations, accessors)
- [ ] 2.7 Wire synthesize_projection3d_voltfox into synthesize_unified_entity()
- [ ] 2.8 Add unit tests: mesh vertex count, skeleton bone hierarchy, skinning weights
- [ ] 2.9 Add glTF export test with validation
- [ ] 2.10 Verify GCC + MSVC strict builds pass

## 3. 2.5D Voltfox Plane Synthesis

- [ ] 3.1 Add `synthesize_projection25d_voltfox()` declaration to `synthesis.hpp`
- [ ] 3.2 Implement z-sorted plane-image generation from morphology parts
- [ ] 3.3 Add parallax metadata to each plane
- [ ] 3.4 Support all six forms with distinct plane arrangements
- [ ] 3.5 Wire synthesize_projection25d_voltfox into synthesize_unified_entity()
- [ ] 3.6 Add unit tests: multi-plane output, depth ordering, parallax factors
- [ ] 3.7 Verify GCC + MSVC strict builds pass

## 4. 2D Voltfox Frame Synthesis

- [ ] 4.1 Add `synthesize_projection2d_voltfox()` declaration to `synthesis.hpp`
- [ ] 4.2 Implement 2D raster part renderer (ellipse, triangle, circle, line primitives)
- [ ] 4.3 Implement z-order compositing of 11 morphology parts
- [ ] 4.4 Implement six form variants with distinct visual adjustments
- [ ] 4.5 Add collision AABB computation per form
- [ ] 4.6 Add ability timing metadata per frame
- [ ] 4.7 Wire synthesize_projection2d_voltfox into synthesize_unified_entity()
- [ ] 4.8 Add unit tests: pixel output non-zero, form visual distinctness, collision bounds
- [ ] 4.9 Verify GCC + MSVC strict builds pass

## 5. Living + Combat + Transformation Wiring

- [ ] 5.1 Extend SpriteIr to carry compiled form/transformation data into synthesis
- [ ] 5.2 Implement CombatPerceptionSensor translating combat events to utility signals
- [ ] 5.3 Implement form-selection utility action for LivingSpriteRuntime
- [ ] 5.4 Map directional_lightning as a combat ability with form-gating
- [ ] 5.5 Implement transformation VFX trigger in manifestation driver
- [ ] 5.6 Extend RuntimePersistenceHeader with form/combat/animation state
- [ ] 5.7 Add unit tests: combat perception signals, form selection utility, lightning gating
- [ ] 5.8 Add round-trip persistence test with full state
- [ ] 5.9 Verify GCC + MSVC strict builds pass

## 6. Headless Evidence and Deterministic Oracle

- [ ] 6.1 Implement `run_headless_evidence()` with fixed scenario, seeded RNG, 16ms tick
- [ ] 6.2 Implement EvidenceEvent and EvidenceTrace data structures
- [ ] 6.3 Implement `write_trace_json()` serialization
- [ ] 6.4 Generate and commit reference oracle JSON file
- [ ] 6.5 Add test running headless scenario and comparing against oracle
- [ ] 6.6 Add test verifying headless mode produces no graphical output
- [ ] 6.7 Verify GCC + MSVC strict builds pass

## 7. Package-Driven Preview Executable

- [ ] 7.1 Add CMake `BUILD_PREVIEW` option and preview target with SDL2 linkage
- [ ] 7.2 Implement package loading via `load_package()` in preview main
- [ ] 7.3 Implement SDL2 window, 2D sprite rendering, 2.5D plane compositing, 3D wireframe
- [ ] 7.4 Implement keyboard handling for mode switching (2/3/5/D keys)
- [ ] 7.5 Implement living runtime tick integration in preview loop
- [ ] 7.6 Implement HUD and debug overlay
- [ ] 7.7 Add test verifying preview builds (not functional test)
- [ ] 7.8 Verify GCC + MSVC strict builds pass

## 8. Fuzz Parsing Harness

- [ ] 8.1 Implement fuzz test target generating random seed byte sequences
- [ ] 8.2 Implement round-trip parse → canonicalize → write → parse fuzz test
- [ ] 8.3 Add time-boxed execution with configurable wall-clock limit
- [ ] 8.4 Use deterministic seed (42) with GSPL_FUZZ_SEED override
- [ ] 8.5 Add property tests for parse invariants
- [ ] 8.6 Verify GCC + MSVC strict builds pass

## 9. Integration, Final Verification, and Archive

- [ ] 9.1 Run full CTest suite (42 + new tests) under GCC, fix any failures
- [ ] 9.2 Run full CTest suite (42 + new tests) under MSVC, fix any failures
- [ ] 9.3 Run fuzz harness for 30 seconds under both compilers
- [ ] 9.4 Build preview target under both compilers
- [ ] 9.5 Final review: no warnings, no leaks, all specs covered
- [ ] 9.6 Git commit and push
- [ ] 9.7 Archive the change
