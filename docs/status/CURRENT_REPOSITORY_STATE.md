# Current Repository State

Updated: 2026-07-29

## Repository identity

- Remote: `https://github.com/11vatedTech/GSPL-Sprites`
- Active branch: `validation/qt-studio-bootstrap`
- Implementation SHA: `e6dc40044224ac090fe013676b6ccfb8b3447d18`
- Remote parity: YES
- Main HEAD: `e8b69369ddbbc61d3a96fd10b70651bb28b542de`
- Merge base: `e8b69369`
- Commits ahead of main: 25
- Working tree: clean

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

- 83 test targets declared in CMake
- **Live evidence (2026-07-29)**: 83/83 CTest PASSED (Debug, MSVC /W4 /WX, 86s total)
- Targeted test suites: `semantic_pipeline_tests`, `gene_tests`, `compiler_tests`, `ir_persistence_tests` — ALL PASSED
- Branch scope (ws-ignored): 135 files, +16,412 / -455 lines

## Serialization architecture (current at e6dc400)

- `include/gspl/json.hpp`: `BoundedJsonReader` with configurable limits, per-container stack (`ContainerFrame`), `GeneValueTag` enum, result-based reads
- `src/json.cpp`: Governed container APIs (begin/end_object/array, next_*_member/element, record_*), per-frame item counting, trailing comma rejection, strict numbers, Unicode support
- `include/gspl/ir.hpp`: `SpriteIrDeserializeResult`, configurable `deserialize(json, config)`, graph methods
- `include/gspl/semantics.hpp`: `CanonicalSerializationResult`, typed codecs
- `src/ir.cpp`: Fail-closed deserialization with result-based lambdas, enum validation (compile-time assertions), collection-specific node kind enforcement, duplicate field/gene value name rejection, saw_* root field tracking
- `src/semantics.cpp`: CanonicalEntity deserialization via BoundedJsonReader with typed codecs
- `tests/ir_persistence_tests.cpp`: 36 adversarial tests covering empty/truncated/malformed input, missing fields, invalid enums, duplicates, wrong-collection kinds
- `tests/semantic_pipeline_tests.cpp`: Tests 16-22 (round-trip, fail-closed, IR, optimization)

## Current audit observations

- `docs/canon/` and `docs/status/` are being reconstructed.
- Build artifacts and vendored dependency build trees exist in ignored `build*` paths and should not be treated as source truth.
- Historical TypeScript seven-package monorepo is not the current truth; present implementation is a C++23 CMake repository with optional Qt Studio.
- DEF-0001 through DEF-0011 RESOLVED. DEF-0012 through DEF-0014 OPEN (see KNOWN_DEFECTS.md).

## Live command evidence (2026-07-29)

- **BUILD (Debug)**: `cmake --build build/windows-msvc --config Debug` — 0 errors, 0 warnings (`/W4 /WX /permissive-`)
- **FULL CTEST**: 83/83 passed (Debug, 86.36s total)
- **TARGETED SUITES**: semantic_pipeline (ALL PASSED), gene_tests (ALL PASSED), compiler_tests (ALL PASSED), ir_persistence_tests (ALL PASSED)
- **CI STATUS**: Pending — no remote CI run for e6dc400 yet

### Defects status
- DEF-0001 through DEF-0016: ALL RESOLVED ✅
- DEF-0017: OPEN (remaining adversarial gaps — see KNOWN_DEFECTS.md)
