# Current Repository State

Updated: 2026-07-27

## Repository identity

- Repository root: `C:/Users/11vat/OneDrive/Desktop/claude/11vatedTech_canon-workspace/Inventions/GSPL-Sprites`
- Remote: `https://github.com/11vatedTech/GSPL-Sprites` (from `.git/config`)
- Active branch: `validation/qt-studio-bootstrap` (from `.git/HEAD`)
- Current HEAD: `47394c83a3585952a3334df9a850db134c8eb291` (from `.git/refs/heads/validation/qt-studio-bootstrap`)
- Local `main`: `a9df638566610110f98772bbc31883c21c6cba38` (from `.git/refs/heads/main`).
- Last known `origin/main`: `e8b69369ddbbc61d3a96fd10b70651bb28b542de` (from `.git/packed-refs`; no loose `refs/remotes/origin/main` file exists).
- Live `git status` / `git fetch --all --prune`: not yet executable-verified because every Bash/PowerShell/cmd attempt was blocked by the harness safety-classification outage.
- Recent local history source: `.git/logs/HEAD`; last recorded commit is `validation: qt studio bootstrap checkpoint`.
- Local branch relationship from file evidence: `validation/qt-studio-bootstrap` is local-only in `.git/config` (no upstream configured), is ahead of local `main`, and contains at least the checkpoint commit `47394c83` on top of `a9df638`. Remote recency remains unverified until `git fetch` can run.

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

## Current audit observations

- `docs/canon/` and `docs/status/` were absent at session start; status/canon reconstruction is now being created.
- Build artifacts and vendored dependency build trees exist in ignored `build*` paths and should not be treated as source truth.
- Historical TypeScript seven-package monorepo is not the current truth; present implementation is a C++23 CMake repository with optional Qt Studio.
- Foundational false-completion risk confirmed: `GeneCompositionPhase` was a no-op, tests explicitly documented that genes were not applied, and `CanonicalizePhase` lowered with an empty gene vector. This session began repairing that gap.
- Additional false-completion risks found during inspection: `IrSerializer::deserialize()` returns hardcoded placeholder data, `IrSerializer::dependencies()` returns empty, `IrOptimizePhase` does no work, and `CanonicalEntitySerializer::from_json()` returns `std::nullopt`.

## Command evidence pending

The following required fields are not yet executable-evidence verified in this session:

- BUILD COMMAND result
- TYPECHECK COMMAND result
- TEST COMMAND result
- LINT COMMAND result
- FORMAT COMMAND result
- PACKAGE COMMAND result
- FUZZ COMMAND result
- SANITIZER COMMAND result
- RELEASE COMMAND result
- CURRENT TEST TOTAL
- CURRENT FAILURES
- CURRENT SKIPS
- CURRENT WARNINGS
- CURRENT CI STATUS

Reason: Bash safety classification was intermittently unavailable for git/build commands. File inspection continued; commands must be rerun before any pass claim.
