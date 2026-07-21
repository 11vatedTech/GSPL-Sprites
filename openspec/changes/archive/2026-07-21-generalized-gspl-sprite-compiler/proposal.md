## Why

The GSPL sprite compiler must be production-grade: deterministic, bounded, validated, and verifiable across all 2D/2.5D/3D representations. The infrastructure hardening (resource limits, frame identity, preview authority, headless contract) was committed as foundation; the remaining gap is completing the generalized compiler that accepts arbitrary SpriteSeeds and produces correct, efficient, cross-representation sprite output.

## What Changes

- **Resource-limit enforcement** (committed): 30-field ResourceLimits with enforce_resource_limits_source wired into all entry points
- **Frame content identity** (committed): GSPL_FRAME_V1 schema-bound SHA-256 hashing with validate_frame_distinctness
- **Preview authority** (committed): Transformation state machine drives preview; input flows through validate→state→manifestation
- **Headless output contract** (committed): Versioned JSON (_schema_version: gspl_evidence_v1), committed oracle, oracle verification test
- **PixelAABB semantics** (committed): Tight bounding box of non-transparent pixels with comprehensive coverage
- **Generalized compiler**: Complete the SpriteSeed→SpriteIr→cross-representation synthesis pipeline for arbitrary (non-Voltfox) seeds
- **Mutation test coverage**: 7/10 mutants killed; 3 remaining gaps (preview toggle malformed headless output, continuous integration)
- **Continuous integration**: Dual-compiler validation on every change (GCC + MSVC, 0 warnings, all tests pass)

## Capabilities

### New Capabilities
- `generalized-compiler`: Accept arbitrary SpriteSeeds, compile to SpriteIR, synthesize all three representations (2D, 2.5D, 3D) with deterministic cross-representation parity
- `resource-limits-enforcement`: Bounded resource consumption at every pipeline stage (parsing, compilation, synthesis, packaging, runtime)
- `frame-content-identity`: Schema-bound SHA-256 hashing for frame content distinctness, PixelAABB for collision/visibility queries
- `headless-evidence-contract`: Versioned JSON evidence trace with committed oracle and verification
- `preview-authority`: Transformation-state-machine-driven preview with input→validate→state→manifestation pipeline

### Modified Capabilities
- `headless-evidence` (existing spec): Add _schema_version field, status_id/ability_name serialization, oracle verification requirement
- `seed-forms` (existing spec): Form validation now enforces resource limits; transformation state machine drives form transitions
- `compile-forms` (existing spec): Compilation produces cross-representation forms; compilation enforces resource limits

## Impact

- `src/core.cpp`: Resource-limit enforcement added to parse_seed(), validate(), build_package_internal()
- `src/image.cpp`: compute_frame_hash redesigned; validate_frame_distinctness, compute_pixel_aabb added
- `src/preview.cpp`: Transformation state machine replaces direct display toggles
- `src/headless_evidence.cpp`: Versioned JSON output; resource-limit enforcement in run_headless_evidence
- `src/synthesis.cpp`: Frame hash propagation; resource-limit enforcement
- `tests/mutation_tests.cpp`: 24 mutation tests covering 7/10 mutant classes
- `tests/headless_evidence_tests.cpp`: Oracle verification test
- `tests/evidence_oracle.json`: Committed deterministic oracle
- `CMakeLists.txt`: GSPL_SPRITES_SOURCE_DIR define for oracle path resolution
