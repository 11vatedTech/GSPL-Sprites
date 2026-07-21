## Context

The GSPL sprite compiler pipeline: SpriteSeed → parse_seed → SpriteSeed → validate → SpriteSeed → compile → SpriteIR → synthesize → SynthesisResult → build_package → Package. Infrastructure hardening (resource limits, frame identity, preview authority, headless contract) is committed (cfc1ef0). The remaining work completes the generalized compiler that accepts arbitrary seeds, produces all three representations deterministically, and integrates into CI.

## Goals / Non-Goals

**Goals:**
- Arbitrary SpriteSeeds produce correct cross-representation output (2D, 2.5D, 3D) with frame hashes
- Resource limits enforced at all pipeline stages with clear diagnostics
- Mutation test coverage reaches 10/10 mutants killed
- Dual-compiler CI enforces 0 warnings and 48/48 test pass rate
- All remaining compiler features (cross-representation parity, state management, collision) are fully wired

**Non-Goals:**
- Adding new external dependencies
- Rewriting existing validated modules
- GPU acceleration or runtime loading optimizations

## Decisions

- **Frame hash schema before pixel content**: The SHA-256 preimage prepends `GSPL_FRAME_V1` + dimensions + format metadata before RGBA bytes. This ensures schema evolution is detectable and hash collisions across schema versions are impossible.
- **Resource limits as last-resort guard**: Limits are checked before and after each pipeline stage. The limits are soft (throw on exceed) rather than silently clamped, so callers always know when a seed exceeds constraints.
- **Preview state driven by transformation state**: SDL preview reads current_form and active transformation from the authoritative TransformationState. Keyboard inputs call begin_transformation/advance_transformation_to rather than modifying display fields directly.
- **Oracle-driven headless contract**: The committed evidence_oracle.json is the single source of truth for the headless output format. The test generates current output and compares byte-for-byte against the oracle.

## Risks / Trade-offs

- [Device Guard policy] Environment blocks freshly-linked executables → Mitigation: rebuild and retry; not a code issue
- [Test execution time] 48 tests run ~93s on GCC → Acceptable for pre-commit but could be parallelized
- [Mutation test gaps] Preview toggle and malformed headless output not tested → Addressed in remaining tasks
