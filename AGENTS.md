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
- GSPL 1.0 language frontend core: `SourceManager`, `Lexer`, `Parser`, `AST` (module, entity, gene, form, transformation, morphology, resource, ability, rights)
- Diagnostics system with typed error codes, severity, JSON output (28 error codes)
- Module system: `ModulePath`, `ModuleResolver` (cycle detection), `NameResolver` (scope-based name resolution)
- Type system: `GsplType` (Bool/Int/UInt/Fixed/String/Color/Duration/Distance/Angle/etc.), `TypeChecker` (dimension compatibility, assignability)
- Expression evaluator: bounded `ExpressionEvaluator` with entropy channels, depth/step/node limits
- Gene system: `GeneRegistry` with 35 built-in gene kinds, dependency/conflict validation, composition with override
- Sprite IR: `SpriteIr` data model, `IrSerializer` (serialize/validate/diff)
- Compiler-pass architecture: `CompilationContext`, `PassManager`, 9 implemented passes (lex/parse/module-resolve/name-resolve/type-check/gene-compose/ir-gen/ir-validate/ir-optimize), topological scheduling
- CLI: `gsplc` binary with argument parsing, file compilation, pass pipeline, JSON/IR output
- `include/gspl/` SDK headers (13 headers) + `src/` implementations (11 source files)
- 20 test groups in `gspl_compiler_tests`, all passing
- All existing tests continue to pass (core, domain, fuzz, mutation, voltfox)

### Next Platform Milestone
- Grammar feature completion (traits, expressions, operators, morphologies, collisions, joints, sockets, palettes, animations, behaviors, transitions)
- Full linked-symbol resolver with module-import merging
- Dimensional safety analysis with unit inference
- Constraint/bound validation pass with contract enforcement
- Incremental artifact cache and IR-persistent compilation
- Provider abstraction to decouple ONNX Runtime dependency
- Cross-platform: restore portable Linux CI
