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
- All OpenSpec changes complete and archived — no active changes remain
- Release-closure final report produced: `openspec/changes/archive/2026-07-21-generalized-gspl-sprite-compiler/release-closure-report.md`

### Important Details
- Repository `https://github.com/11vatedTech/GSPL-Sprites`, branch `main`
- HEAD = origin/main = `a78c387` (0 ahead, 0 behind, working tree clean)
- GCC 16.1.0 MinGW-W64 with `-Werror` (local); MSVC via GitHub Actions CI (windows-2025 only)
- Linux GCC not currently supported — ONNX Runtime Windows-x64 dependency boundary
- Windows Device Guard blocks freshly-linked executables (BAD_COMMAND) on clean rebuild — purely environmental, not code failures

### Archived Changes (4)
1. `2026-07-19-voltfox-reference-entity` — Reference entity vertical
2. `2026-07-20-voltfox-living-sprite-vertical` — Living sprite vertical
3. `2026-07-20-voltfox-living-sprite-vertical-v2` — Living sprite vertical v2
4. `2026-07-21-generalized-gspl-sprite-compiler` — Generalized sprite compiler (14 main specs, 23/24 IMPLEMENTED, 1 APPROVED DEFERRAL, CI green)

### Next Platform Milestone
- Full GSPL language frontend (grammar, lexer, parser, AST, modules, imports, types, units, expressions)
- Typed Sprite Genes with composition, inheritance, and registry
- Compiler-pass architecture with deterministic scheduling and incremental execution
- Incremental artifact compilation and cache integrity
- Complete resource-boundary validation
- Provider abstraction (decouple core from Windows-only inference)
- Cross-platform strategy (restore portable Linux CI)
- SDK stabilization and CLI completion
