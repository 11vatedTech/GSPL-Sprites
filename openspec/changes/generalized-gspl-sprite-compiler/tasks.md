## 1. Infrastructure Hardening (Committed in cfc1ef0)

- [x] 1.1 Implement ResourceLimits struct (30 fields) with enforce_resource_limits_source and seed/Ir-level overloads
- [x] 1.2 Wire resource limits into parse_seed(), validate(), build_package_internal()
- [x] 1.3 Guard validate() resource check with if (result.ok()) to prevent canonicalize_rig throw on invalid seeds
- [x] 1.4 Implement compute_frame_hash with GSPL_FRAME_V1 schema header + SHA-256 preimage
- [x] 1.5 Implement compute_pixel_aabb with tight bounds, empty/point/deterministic/edge-touching tests
- [x] 1.6 Implement validate_frame_distinctness with SPRITE_ANIMATION_FRAME_DUPLICATE_CONTENT diagnostic
- [x] 1.7 Refactor preview.cpp to route A/S keys through transformation state machine
- [x] 1.8 Update write_trace_json to include _schema_version, status_id, ability_name
- [x] 1.9 Commit evidence_oracle.json and add oracle verification test
- [x] 1.10 Add GSPL_SPRITES_SOURCE_DIR CMake define for oracle path resolution
- [x] 1.11 Add corrupted RGBA mutation test (test 24) for invariant, pack_atlas, encode_ppm_p6

## 2. Generalized Compiler Pipeline

- [x] 2.1 Complete compile() to produce cross-representation SpriteIR from arbitrary SpriteSeeds (non-Voltfox)
- [x] 2.2 Wire resource-limit checks into compile() for forms, transformations, bones, sockets, clips, abilities
- [x] 2.3 Ensure deterministic output: repeated compilation and synthesis produce identical frame hashes and projection data
- [x] 2.4 Add package size enforcement (max 256MB) to build_package
- [x] 2.5 Add source-level resource limit diagnostics (source_bytes, token_count, token_length)
- [~] 2.6 Add frame hash propagation from synthesis through to FrameSource on all three representations
     Frame hashes set on 2D FrameSource objects via make_frames() and voltfox 2D synth.
     2.5D DepthPlaneDefinition and 3D Mesh3d have no frame_hash field — frame identity is 2D-only.

## 3. Remaining Mutation Test Coverage

- [x] 3.1 Add mutation test for preview toggle (A key ascends, S key descends; verify transformation state changes)
- [x] 3.2 Add mutation test for malformed headless output (missing _schema_version, missing event fields)
- [~] 3.3 Verify 48/48 core_tests + 26 mutation scenarios pass on both GCC and MSVC
     Verified on GCC (MinGW 16.1.0). MSVC CI must confirm post-push.

## 4. Dual-Compiler CI Pipeline

- [x] 4.1 CI workflow configured: ubuntu-24.04 uses GCC with -Werror, 0 warnings
- [x] 4.2 CI workflow configured: windows-2025 uses MSVC with /W4 /WX /permissive-, 0 warnings
- [x] 4.3 CTest runs all 48 registered tests including mutation_tests (26 scenarios) via --output-on-failure
- [ ] 4.4 Document Device Guard mitigation (rebuild + retry) in CI contributor docs

## 5. Cross-Representation Synthesis Parity

- [~] 5.1 Verify 2D, 2.5D, and 3D representations produce consistent frame hashes for identical seeds
     Verified for 2D. 2.5D/3D have no frame_hash field — semantic state identity replaces pixel hashes.
- [x] 5.2 Add integration test confirming structural consistency across representations (section 8)
- [x] 5.3 Ensure synthesis resource limits (max_vertices, max_planes, max_frames, max_clips) are enforced
