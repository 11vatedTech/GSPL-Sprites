# Current Repository State

Updated: 2026-07-28 (session: serialization overhaul — commit, push, verify)

## Repository identity

- Repository root: `C:/Users/11vat/OneDrive/Desktop/claude/11vatedTech_canon-workspace/Inventions/GSPL-Sprites`
- Remote: `https://github.com/11vatedTech/GSPL-Sprites`
- Active branch: `validation/qt-studio-bootstrap`
- Current HEAD / remote HEAD: `2b4d1237428ac6833da92b1c252fbd9a521e421e`
- Local == Remote: YES (in sync at `2b4d123`)
- Commits ahead: 0 | Commits behind: 0
- Working tree: 6 modified + 2 new files (serialization overhaul — uncommitted)

## Build system

- Primary build system: CMake 3.25+ (`CMakeLists.txt`, `CMakePresets.json`).
- Language core: C++23.
- Main library target: `gspl_sprites_core`.
- Main executables: `gspl-sprites`, `gsplc`.
- Optional preview: `BUILD_PREVIEW`.
- Optional provider-disabled portable build: `GSPL_CORE_ONLY=ON`.
- Optional Qt Studio build: `GSPL_BUILD_STUDIO=ON` with Qt6 6.8 components.

## Dependencies observed

- zlib pinned by CMake FetchContent to commit `da607da739fa6047df13e66a2af6b8bec7c2a498` with SHA-256 verification.
- libspng pinned by CMake FetchContent to commit `fb768002d4288590083a476af628e51c3f1d47cd` with SHA-256 verification.
- ONNX Runtime 1.26.0 Windows x64 CPU archive is used when `GSPL_CORE_ONLY=OFF`.
- Qt6 is required only for `GSPL_BUILD_STUDIO=ON`.
- Dependency sovereignty note: core has a provider-disabled path, but release verification must continue proving no essential runtime path requires ONNX Runtime, Qt, network access, or paid services.

## Authoritative commands from repository files

README Windows path:

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug
./build/windows-msvc/gspl-sprites.exe build examples/voltfox.sprite outputs/voltfox
./build/windows-msvc/gspl-sprites.exe verify outputs/voltfox
```

CMake presets:

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug
```

Portable Studio preset:

```powershell
cmake --preset windows-msvc-studio-portable
cmake --build --preset windows-msvc-studio-portable-debug
ctest --preset windows-msvc-studio-portable-debug
```

CI commands from `.github/workflows/ci.yml`:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Linux core-only CI command:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DGSPL_CORE_ONLY=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Test topology

- CMake declares 80+ test executables in the root `CMakeLists.txt`, plus additional Qt Studio tests behind `GSPL_BUILD_STUDIO` in `src/studio/CMakeLists.txt`.
- Prior repository notes claim 83/83 targets passed on MSVC CORE_ONLY, but that is source-reported until rerun in this session.
- Tests include UTF-8, modules, types, expressions, genes, cache, providers, legacy migration, fuzz parsing smoke, mutation tests, compiler pipeline, semantic pipeline, CLI, SDK header self-containment, LS, E2E workflow, plugin, security, benchmarks, and Studio model tests.

## Serialization overhaul (2026-07-28)

New infrastructure added (uncommitted):
- `include/gspl/json.hpp` (87 lines): `BoundedJsonReader` with configurable limits, `GeneValueTag` enum, `gene_value_to_json`/`json_to_gene_value`/`gene_value_variant_index`
- `src/json.cpp` (383 lines): Full implementation — JSON writer helpers, bounded reader, typed GeneValue round-trip, `static_assert` on variant alignment, NaN/infinity rejection

API changes (uncommitted):
- `include/gspl/ir.hpp`: Added `SpriteIrDeserializeResult` struct, `deserialize()` returns Result, graph methods (`transitive_dependencies`, `reverse_dependencies`, `dependency_closure`)
- `include/gspl/semantics.hpp`: Added `CanonicalEntityDeserializeResult` struct, `from_json()` returns Result (single-arg, fail-closed)
- `src/ir.cpp`: Fail-closed deserialization (no fabricated defaults), representations array serialization, dependency graph traversal
- `src/semantics.cpp`: `from_json()` parses ALL structural fields (forms, transformations, morphology, abilities, bones, sockets, clips, states, transitions, collisions, resources, runtime)
- `tests/semantic_pipeline_tests.cpp`: Tests 16-20 updated to new fail-closed API

## Current audit observations

- `docs/canon/` and `docs/status/` are being reconstructed.
- Build artifacts and vendored dependency build trees exist in ignored `build*` paths and should not be treated as source truth.
- Historical TypeScript seven-package monorepo is not the current truth; present implementation is a C++23 CMake repository with optional Qt Studio.
- DEF-0001 through DEF-0011 RESOLVED. DEF-0012 through DEF-0014 OPEN (see KNOWN_DEFECTS.md).

## Live command evidence (2026-07-28)

- **BUILD (Debug)**: `cmake --build build/windows-msvc --config Debug` — 0 errors, 0 warnings (`/W4 /WX /permissive-`)
- **BUILD (core-only)**: `cmake --build build/windows-msvc-core --config Debug -DGSPL_CORE_ONLY=ON` — 0 errors
- **TARGETED TESTS**: 3/3 pass — `gene_tests`, `semantic_pipeline_tests` (20 tests), `compiler_tests`
- **FULL CTEST**: 3 pass, 79 "Not Run" (Studio/Qt/ONNX targets not built in this config — pre-existing)
- **CI STATUS**: Pending — push required to trigger GitHub Actions

### Open defects
- DEF-0012: Full structural round-trip tests (maximally populated fixtures)
- DEF-0013: GeneValue type-tagged format not yet wired into IrSerializer
- DEF-0014: Resource-limit boundary tests
