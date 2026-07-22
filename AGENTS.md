# GSPL Sprites engineering contract

GSPL Sprites is an independent downstream product. The adjacent GSPL canon and
all `Reference-repos_and_planning` repositories are read-only evidence. Never
import them, modify them, or require them to build, test, run, or package this
repository.

Production code must be deterministic by default, typed, bounded, validated,
and covered by meaningful tests. Semantic entity state is authoritative;
rendered assets are target projections. Unsupported semantics must produce
diagnostics and must never be silently discarded.

No generated model output may enter a release package without model identity,
license, source hash, determinism class, and provenance. No external input is
trusted. Do not add network access to the compiler or runtime core.

## Anchored Summary

### Status
- 1 active OpenSpec change (`gspl-language-and-platform-completion`) — implementation in progress
- 4 archived changes complete
- Release-closure final report: `openspec/changes/archive/2026-07-21-generalized-gspl-sprite-compiler/release-closure-report.md`

### Important Details
- Repository `https://github.com/11vatedTech/GSPL-Sprites`, branch `main`
- GCC 16.1.0 MinGW-W64 with `-Werror` (local); MSVC via GitHub Actions CI (windows-2025 only)
- Linux GCC not currently supported — ONNX Runtime Windows-x64 dependency boundary
- Windows Device Guard blocks freshly-linked executables (BAD_COMMAND) on clean rebuild — purely environmental, not code failures

### Changes

| Change | Status |
|---|---|
| `voltfox-reference-entity` | Archived |
| `voltfox-living-sprite-vertical` | Archived |
| `voltfox-living-sprite-vertical-v2` | Archived |
| `generalized-gspl-sprite-compiler` | Archived (23/24 IMPLEMENTED, 1 APPROVED DEFERRAL) |
| `gspl-language-and-platform-completion` | Active — implementing full GSPL 1.0 language + platform |

### Implemented (this session)
- **CanonicalEntity bridge** (`include/gspl/semantics.hpp`, `include/gspl/lowering.hpp`, `src/semantics.cpp`, `src/lowering.cpp`):
  - `CanonicalEntity`: unified semantic bridge between typed GSPL AST and production `SpriteSeed` (parts, forms, transformations, abilities, morphology, bones, sockets, animation clips/states/transitions, collision shapes/windows, runtime attributes, genes, provenance)
  - `Canonicalizer`: lowers `ModuleDecl` + `GeneInstance` vector to `CanonicalEntity`
  - `SpriteIrLowering`: direct `CanonicalEntity` → production `gspl::sprites::SpriteIr` (authoritative path)
  - `SpriteSeedLowering`: `CanonicalEntity` → `gspl::sprites::SpriteSeed` (compatibility adapter)
  - `CanonicalEntitySerializer`: to_json / to_yaml / from_json
  - `CanonicalEntityValidator`: validates required fields, rights, cross-references
  - `CanonicalEntityIdentity`: deterministic hash from semantic fields (no memory addresses, absolute paths, wall-clock values, or unordered-container iteration order)
  - `CanonicalEntityDiff`: structured field-by-field comparison with CHANGED/UNCHANGED
- **Four new compiler passes**: `CanonicalizePhase`, `CanonicalValidatePhase`, `SpriteIrLowerPhase`, `SeedLowerPhase` (added to pass pipeline with topological dependencies)
- **Production-grade diagnostics**: typed `LoweringDiagnostic` codes (RIGHTS_INVALID, COLOR_INVALID, FORM_UNREPRESENTABLE, SEMANTIC_LOSS, INTERNAL_FAILURE, etc.)
- **CLI updated**: `gsplc` registers all new passes; default target list includes `canonicalize` → `canonical_validate` → `sprite_ir_lower` → `seed_lower`
- **Modern GSPL example**: `examples/gspl/voltfox/main.gspl` — Voltfox entity using all supported declaration types
- **Comprehensive end-to-end test**: `tests/semantic_pipeline_tests.cpp` — 11 tests covering full pipeline, CanonicalEntity serializer/validator/identity/diff, SpriteIrLowering, SpriteSeedLowering, production compile, PassManager topology, and diagnostic reporting
- **Parser bugfixes**: removed double-advance in `parse_literal()`, added semicolon consumption in `parse_rights()`
- **CLI bugfix**: fixed `argv[0]` being treated as input file (start loop at index 1)
- **Lowering enhancement**: RIGHTS_INVALID diagnostic when rights classification is unrecognized
- **C++23 compat fixes**: `std::divides` replaced with generic lambda; guarded `visit_numeric` against `bool` operand; added `else` branch for C4702 unreachable code

### Next Milestones
- `--package` CLI flag to call `build_package()` after compilation
- Full AST → CanonicalEntity field population from ability/form body attributes (currently uses defaults)
- Grammar feature completion (traits, expressions, operators, morphologies, collisions, joints, sockets, palettes, animations, behaviors, transitions, hyphenated identifiers)
- Provider abstraction to decouple ONNX Runtime dependency (GSPL_CORE_ONLY build profile)
