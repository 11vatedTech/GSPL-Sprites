## 1. Seed Verification

- [x] 1.1 Verify `examples/voltfox.sprite` parses, compiles, and canonicalizes correctly
- [x] 1.2 Confirm `build_package(seed, temp_output)` produces a self-verifying package with expected artifacts

## 2. Visual Set Authoring

- [x] 2.1 Create `examples/voltfox.visual.txt` manifest with frame definitions for all animations (idle, attack), directions (south), layers (body, head, tail), and ordinals
- [x] 2.2 Create or generate PNG frame assets for each frame definition, placed under `examples/assets/`
- [x] 2.3 Add channel map definitions (depth, effects) for at least one frame
- [x] 2.4 Verify `load_authored_visual_set("examples/voltfox.visual.txt")` succeeds

## 3. CMake Integration Test Target

- [x] 3.1 Add `voltfox_integration_tests.cpp` under `tests/`
- [x] 3.2 Register test target `gspl_sprites_voltfox_integration_tests` in `CMakeLists.txt` with appropriate source and link dependencies
- [x] 3.3 Add `add_test` call for the new target

## 4. Integration Test Implementation

- [x] 4.1 Test: Parse voltfox seed, validate, compute identity — verify deterministic SHA-256
- [x] 4.2 Test: Build package from seed-only path, verify all required artifacts present, self-verify passes
- [x] 4.3 Test: Synthesize all six projections via `synthesize_unified_entity`, validate each with its validator
- [x] 4.4 Test: Validate transformation manifestation programs against combat + transformation programs
- [x] 4.5 Test: Build package with authored visual set, verify additional raster artifacts present
- [x] 4.6 Test: Deterministic build — two consecutive builds produce identical package identity
- [x] 4.7 Test: Render SVG from seed, verify non-empty valid output

## 5. Verification

- [x] 5.1 Build the new test target under MinGW GCC
- [x] 5.2 Run `gspl_sprites_voltfox_integration_tests` and confirm all tests pass
- [x] 5.3 Build the new test target under MSVC — MSVC BuildTools 2022 configured manually, full 42/42 tests pass with /W4 /WX /permissive-

## 6. Connection, Negative, and Integrity Tests

- [x] 6.1 Add `synthesize_unified_entity(const SpriteIr& ir)` that auto-extracts palette, rig, and collisions from compiled IR
- [x] 6.2 Connection test: seed → compile → synthesize → verify entity_id, rig bones, sockets, collision shapes/windows propagate
- [x] 6.3 Negative tests: parse-time rejections (invalid rights, bad numeric, oversized source, duplicate field)
- [x] 6.4 Negative tests: validation failures (missing schema, missing id, empty classification, prohibited rights)
- [x] 6.5 Integrity test: corrupt a package artifact, verify `verify_package` rejects
- [x] 6.6 Integrity test: two different seeds produce different entity_id and package identity

## 7. Cross-Compiler Integrity

- [x] 7.1 Full repository build + full CTest suite under MinGW GCC 16.1.0 — all 42/42 tests pass
- [x] 7.2 Full repository build + full CTest suite under MSVC 19.44.35228 — all 42/42 tests pass
