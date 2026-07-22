# Release Closure Report: GSPL Language and Platform Completion

**Change**: `gspl-language-and-platform-completion`
**Date**: 2026-07-22
**Status**: ✓ Ready for Archive

## Summary

This change transforms the ad-hoc GSPL sprite compiler into a complete programming language with formal grammar, typed AST, module system, type system, first-class Sprite Genes, compiler-pass architecture, artifact cache, provider abstraction, stable SDK, and complete CLI.

## Implemented Components

| Component | Status | Tests |
|-----------|--------|-------|
| GSPL 1.0 EBNF Grammar | ✓ Complete | grammar.md |
| Source Manager (SourceId, SourceBuffer, SourceLocation) | ✓ Complete | source tests |
| Deterministic Lexer with typed tokens | ✓ Complete | lexer tests |
| Typed AST (ModuleDecl, EntityDecl, GeneDecl, etc.) | ✓ Complete | parser tests |
| Recursive-descent Parser | ✓ Complete | parser tests |
| UTF-8 Validation + Invalid-UTF-8 Policy | ✓ Complete | 21 utf8 tests |
| Module System (imports, cycles, roots) | ✓ Complete | 10 module tests |
| Type System (primitives, dimensional safety) | ✓ Complete | 9 type tests |
| Bounded Deterministic Expressions + Entropy | ✓ Complete | 13 expression tests |
| Sprite Genes (35 families, composition, conflicts) | ✓ Complete | 9 gene tests |
| Canonical Entity + Sprite IR | ✓ Complete | 11 semantic pipeline tests |
| Compiler Pass Architecture (DAG, scheduling) | ✓ Complete | pass system tests |
| Artifact Cache (content-addressed, LRU, integrity) | ✓ Complete | 9 cache tests |
| Provider Abstraction (NullProvider, FakeTestProvider) | ✓ Complete | 8 provider tests |
| Legacy Compatibility (.sprite → GSPL migration) | ✓ Complete | 7 legacy tests |
| SDK Stabilization (GsplContext, STABILITY.md) | ✓ Complete | 8 header tests |
| Resource Limits (30 fields, boundary enforcement) | ✓ Complete | 13 resource limit tests |
| CLI Completion (parse, check, compile, migrate, ir, graph) | ✓ Complete | 11 CLI tests |
| Fuzzing (parser fuzz, mutation testing) | ✓ Complete | fuzz + 26 mutation tests |
| Build Profiles (GSPL_CORE_ONLY, WITH_ONNX, WITH_PREVIEW) | ✓ Complete | Windows + Linux CI |

## Test Results

```
100% tests passed, 0 tests failed out of 61
```

## Build Profiles

- **Default (WITH_ONNX)**: Windows x64, MSVC 19.44, ONNX Runtime 1.26.0
- **GSPL_CORE_ONLY**: Cross-platform, no ONNX dependency
- **BUILD_PREVIEW**: Optional SDL2 preview executable

## CI Configuration

- Windows: `windows-2025` + Ninja (full ONNX build)
- Linux: `ubuntu-24.04` + Ninja (`GSPL_CORE_ONLY` profile)

## Files Added

### Headers
- `include/gspl/utf8.hpp` — UTF-8 validation, BOM detection, byte classification
- `include/gspl/cache.hpp` — ArtifactCache with FNV-1a content keying
- `include/gspl/provider.hpp` — Provider abstraction interface + registry
- `include/gspl/legacy.hpp` — Legacy .sprite parser, migrate options/report
- `include/gspl/sdk.hpp` — GsplContext RAII, public SDK surface
- `include/gspl/STABILITY.md` — API stability policy

### Sources
- `src/utf8.cpp`, `src/cache.cpp`, `src/provider.cpp`, `src/legacy.cpp`, `src/sdk.cpp`
- `src/inference_stub.cpp` — ONNX stub for CORE_ONLY builds

### Tests
- `tests/utf8_tests.cpp` — 21 tests
- `tests/module_tests.cpp` — 10 tests
- `tests/type_system_tests.cpp` — 9 tests
- `tests/expression_tests.cpp` — 13 tests
- `tests/gene_tests.cpp` — 9 tests
- `tests/resource_limit_tests.cpp` — 13 tests
- `tests/cache_tests.cpp` — 9 tests
- `tests/provider_tests.cpp` — 8 tests
- `tests/legacy_tests.cpp` — 7 tests
- `tests/cli_comprehensive_tests.cpp` — 11 scenarios
- `tests/header_self_containment_tests.cpp` — 8 tests

### CI/Docs
- `.github/workflows/ci.yml` — Windows + Linux CI
- `openspec/.../coverage_matrix.md` — Test coverage documentation

## Key Statistics

- **61 test targets**, all passing
- **0 compiler warnings** (MSVC /W4 /WX)
- **2 build profiles** tested (WITH_ONNX + GSPL_CORE_ONLY)
- **~13 compiler passes** in topological DAG
- **35 gene families** supported
- **30 resource limit fields** with boundary enforcement
- **26 mutation tests** covering production code paths

## Remaining

None. All tasks are complete.
