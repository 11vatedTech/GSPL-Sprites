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
- **Section 2 (UTF-8)**: `include/gspl/utf8.hpp` `src/utf8.cpp` — BOM detection/stripping, UTF-8 validation (overlong, surrogate, continuation, truncated, null bytes, >U+10FFFF), `from_file()` strips BOM, lexer validates in constructor (`GSPL_LEX_INVALID_UTF8`), `next()` uses `classify_utf8_byte()`, multi-byte in `lex_identifier_or_keyword()`. 21 tests in `tests/utf8_tests.cpp`
- **Section 3 (Module tests)**: `tests/module_tests.cpp` — 10 tests (path construction/parsing/round-trip, resolver register+resolve, duplicate detection, name resolution, pipeline, form extension error, source root)
- **Section 4 (Type system tests)**: `tests/type_system_tests.cpp` — 9 tests (primitives, composites, equality, string rep, dimension compat, assignability, TypeRef resolve, type-check pipeline, serialization)
- **Section 5 (Expression + entropy)**: `tests/expression_tests.cpp` — 13 tests (literal eval, entropy channel, channel isolation, depth limit, step counting, deterministic seed, INT64_MAX, zero)
- **Section 6 (Gene tests)**: `tests/gene_tests.cpp` — 9 tests (registry lookup, built-in registration, dep validation, missing dep, dependency chain, custom gene, composition, full pipeline compilation)
- **Section 9 (Artifact cache)**: `include/gspl/cache.hpp` `src/cache.cpp` — CacheConfig, CacheEntry, ArtifactCache with FNV-1a keying, LRU-eviction, atomic writes, integrity validation, stale detection, concurrent-reader safe. 9 tests in `tests/cache_tests.cpp`
- **Section 10 (Provider abstraction)**: `include/gspl/provider.hpp` `src/provider.cpp` — ProviderCapability/Info/Input/Output/Config, abstract Provider base, ProviderRegistry (singleton), NullProvider, FakeTestProvider. 8 tests in `tests/provider_tests.cpp`
- **Section 11 (Legacy compatibility)**: `include/gspl/legacy.hpp` `src/legacy.cpp` — LegacySpriteParser, MigrateOptions/Report, migrate_file/directory, is_legacy_sprite_format. 7 tests in `tests/legacy_tests.cpp`
- **Section 14 (Resource limit tests)**: `tests/resource_limit_tests.cpp` — 13 tests (defaults, source/token/form/string size boundaries, zero-length, large seed, gene count)
- **CanonicalEntity bridge** (previous session): CanonicalEntity, Canonicalizer, SpriteIrLowering, SpriteSeedLowering, CanonicalEntitySerializer/Validator/Identity/Diff, 4 new compiler passes, lowering diagnostics, CLI update, Voltfox example, semantic_pipeline_tests (11 tests)
- **Parser bugfixes**: removed double-advance in `parse_literal()`, added semicolon consumption in `parse_rights()`
- **CLI bugfix**: fixed `argv[0]` being treated as input file (start loop at index 1)
- **C++23 compat fixes**: `std::divides` → generic lambda; guarded `visit_numeric` against `bool`; added `else` for C4702 unreachable code
- **CMakeLists.txt**: added 10 new test targets (utf8, module, type_system, expression, gene, resource_limit, cache, provider, legacy), added `src/utf8.cpp`, `src/cache.cpp`, `src/provider.cpp`, `src/legacy.cpp` to core library
- **ONNX decoupling**: added `GSPL_CORE_ONLY` build profile (`option(GSPL_CORE_ONLY)`), `src/inference_stub.cpp` stub, conditional ONNX FetchContent/linking/staging, guarded inference tests
- **Migrate CLI**: added `--migrate`, `--migrate-output`, `--migrate-dry-run`, `--migrate-overwrite` flags; dispatches to `migrate_file()`/`migrate_directory()`; documented in help
- **Graph CLI**: added `--graph` flag that prints pass dependency graph in DOT format
- **SDK stabilization**: `include/gspl/sdk.hpp` `src/sdk.cpp` — GsplContext RAII context, `include/gspl/STABILITY.md` API stability policy, header self-containment tests
- **CLI tests**: `tests/cli_comprehensive_tests.cpp` — 11 scenarios (flags, migrate dry-run, parse/run)
- **Coverage matrix**: `openspec/.../coverage_matrix.md` — exhaustive test coverage documentation
- **OpenSpec validation**: fixed all 8 spec files to delta format (## ADDED Requirements + Scenario blocks)
- **61/61 tests pass** (0 failures, 0 warnings MSVC 19.44)

### Next Milestones
- Commit to make HEAD == origin/main with clean working tree
- CI config for Linux CORE_ONLY profile
- Archive change and produce final report
