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
- 1 active OpenSpec change (`gspl-authoring-studio-and-production-ecosystem`) — in implementation
- 5 archived changes complete
- Previous change archive: `openspec/changes/archive/2026-07-22-gspl-language-and-platform-completion/`
- **83/83 test targets pass (154+ individual test functions)** (MSVC 19.44, CORE_ONLY) — all builds and tests pass

### Important Details
- Repository `https://github.com/11vatedTech/GSPL-Sprites`, branch `main` (HEAD = e8b6936)
- MSVC 19.44 via Visual Studio 2022 Build Tools (local); MSVC via GitHub Actions CI (windows-2025); Linux CORE_ONLY via GitHub Actions CI (ubuntu-24.04)
- Linux GCC supported via `GSPL_CORE_ONLY` build profile (no ONNX Runtime dependency)
- Studio requires Qt6 (`GSPL_BUILD_STUDIO=ON`) — not available locally; core-only builds unaffected
- **83/83 test targets pass (154+ individual test functions)** (MSVC 19.44, CORE_ONLY) — all builds and tests pass


### Changes

| Change | Status |
|---|---|---|
| `voltfox-reference-entity` | Archived |
| `voltfox-living-sprite-vertical` | Archived |
| `voltfox-living-sprite-vertical-v2` | Archived |
| `generalized-gspl-sprite-compiler` | Archived |
| `gspl-language-and-platform-completion` | Archived |
| `gspl-authoring-studio-and-production-ecosystem` | In progress (all 4 spec artifacts done, ~191 of 263 tasks, Qt6 build pending) |

### Implemented (this session) — Authoring Studio & Production Ecosystem

**Planning Artifacts (4/4 complete)**:
- `proposal.md` — Motivation, 19 new capabilities, impact analysis
- `design.md` — 10 key architectural decisions (Qt6/QML, QProcess IPC, SQLite, command-pattern undo, etc.), 8 risks/mitigations
- `specs/**/*.md` — 19 spec files across all capability areas: studio-shell, language-service, text-authoring, visual-authoring (6 editors), graph-editor, preview-system, semantic-debugger, replay-system, artifact-explorer, provider-management, plugin-system, package-management, publishing, target-adapter, git-integration, theming, performance-benchmarks, security-model, observability
- `tasks.md` — 35 sections, ~200+ checkboxes covering all implementation work

**Foundation Layer (7 headers + 7 impl files)**:
- `include/gspl/studio/ipc_envelope.hpp` `src/studio/ipc_envelope.cpp` — Versioned JSON envelope with shared memory support
- `include/gspl/studio/ipc_channel.hpp` `src/studio/ipc_channel.cpp` — stdin/stdout length-prefixed framing with async reader
- `include/gspl/studio/worker_process.hpp` `src/studio/worker_process.cpp` — QProcess wrapper with health-check, restart policy, crash-loop detection
- `include/gspl/studio/shared_memory.hpp` `src/studio/shared_memory.cpp` — Double-buffered QSharedMemory
- `include/gspl/studio/project.hpp` `src/studio/project.cpp` — gspl-project.jsonc manifest, directory structure validation
- `include/gspl/studio/workspace.hpp` `src/studio/workspace.cpp` — SQLite-backed workspace metadata, project registry
- `include/gspl/studio/document.hpp` `src/studio/document.cpp` — Document base class with kind, dirty, change callback
- `include/gspl/studio/undo_stack.hpp` — Command-pattern undo/redo with bounded depth

**Studio Shell (QML, Qt6-dependent)**:
- `src/studio/shell/main.cpp` — QGuiApplication entry, QML engine, C++ type registration
- `src/studio/shell/MainWindow.qml` — MDI shell with 6 menus, 4 tool panes, status bar
- `src/studio/shell/CommandPalette.qml`, `StartupWizard.qml`, `PreferencesDialog.qml`, `AboutDialog.qml`

**Language Service (pure C++23, no Qt)**:
- `include/gspl/ls/ls_server.hpp` `src/ls/ls_server.cpp` — LSP-compatible server with 12+ methods (rewritten diagnose/build_symbol_index with production SourceManager/Lexer/Parser)
- `include/gspl/ls/diagnostic.hpp` `src/ls/diagnostic.cpp` — Diagnostic with range/severity/message; `from_compile_error` maps real `gspl::Diagnostic` fields
- `include/gspl/ls/completion.hpp` `src/ls/completion.cpp` — CompletionItem with kind/detail/insert; `filter` with UB-safe `::tolower`
- `include/gspl/ls/navigation.hpp` `src/ls/navigation.cpp` — Location, SymbolInfo, Reference, HoverInfo with JSON serialization
- `include/gspl/ls/hover.hpp` `src/ls/hover.cpp` — Hover info generator with keyword docs, declaration lookup, JSON serialization
- `include/gspl/ls/document_symbols.hpp` `src/ls/document_symbols.cpp` — Symbol tree from parsed module via production compiler frontend

**Visual Editors + C++ Models**:
- `src/studio/visual/GeneEditor.qml` — QML gene contract editor panel
- `src/studio/visual/MorphologyEditor.qml`, `FormEditor.qml`
- `src/studio/visual/AnimationEditor.qml`, `BehaviorEditor.qml`, `CombatEditor.qml`
- `src/studio/graph/GraphEditor.qml` — Node-based schematic canvas
- `src/studio/preview/PreviewViewport.qml` — 3-mode viewport (canonical/spritesheet/runtime)
- `src/studio/debugger/DebuggerPanel.qml` — Breakpoints, call stack, variables, watch
- `src/studio/replay/ReplayPanel.qml` — Frame-by-frame trace replay
- `src/studio/artifacts/ArtifactExplorer.qml` — Tree + detail inspector
- **C++ Models** (all 7, pure C++23, no Qt):
  - `include/gspl/studio/gene_editor_model.hpp` `src/studio/visual/gene_editor_model.cpp` — Gene selection, conflict detection
  - `include/gspl/studio/morphology_editor_model.hpp` `src/studio/visual/morphology_editor_model.cpp` — Part tree, add/remove/update position/size
  - `include/gspl/studio/form_editor_model.hpp` `src/studio/visual/form_editor_model.cpp` — Form CRUD, capacity/health editing
  - `include/gspl/studio/animation_editor_model.hpp` `src/studio/visual/animation_editor_model.cpp` — Clip list, track/keyframe counts, looping
  - `include/gspl/studio/behavior_editor_model.hpp` `src/studio/visual/behavior_editor_model.cpp` — Rule CRUD, priority sorting, toggle
  - `include/gspl/studio/combat_editor_model.hpp` `src/studio/visual/combat_editor_model.cpp` — Ability CRUD, storm/normal, cooldown
  - `include/gspl/studio/graph_editor_model.hpp` `src/studio/visual/graph_editor_model.cpp` — Node/edge graph with type-safe ports

**Plugin System (pure C++23, stable C ABI)**:
- `include/gspl/plugin/plugin_api.h` — Stable C ABI (GsplPluginInfo, GsplPluginCallbacks)
- `include/gspl/plugin/manifest.hpp` `src/plugins/manifest.cpp` — JSONC manifest with dependency constraints
- `include/gspl/plugin/plugin_manager.hpp` `src/plugins/plugin_manager.cpp` — Lifecycle manager (discover/load/activate/deactivate/unload)
- `include/gspl/plugin/plugin_sandbox.hpp` `src/plugins/plugin_sandbox.cpp` — Plugin sandbox worker with stdin/stdout IPC
- `src/plugins/plugin_sandbox_main.cpp` — Sandbox executable entry point
- `examples/sample-plugin/` — Sample plugin demonstrating all hook points

**Package Management (pure C++23)**:
- `include/gspl/package/manifest.hpp` `src/studio/packages/manifest.cpp` — PackageManifest with semver, signature
- `include/gspl/package/package_manager.hpp` `src/studio/packages/package_manager.cpp` — Install/update/remove with DFS dependency resolution

**Studio Core (pure C++23, no Qt)**:
- `include/gspl/studio/environment_provider.hpp` `src/studio/environment_provider.cpp` — Typed environment access with allowlist and injectable overrides
- `include/gspl/studio/publishing.hpp` `src/studio/publishing_manager.cpp` — Publish/rollback to local/GitHub/spriteforge targets
- `include/gspl/studio/target_adapter.hpp` `src/studio/target_adapter_manager.cpp` — Build profiles, SDK detection, cross-compilation
- `include/gspl/studio/theme.hpp` `src/studio/theme_manager.cpp` — Light/dark/high-contrast themes with WCAG contrast ratio computation
- `include/gspl/studio/git_integration.hpp` `src/studio/git_integration.cpp` — Git CLI wrapper (status, diff, stage, commit, blame)
- `src/studio/publishing/PublishWizard.qml`, `TargetPanel.qml`, `GitPanel.qml`,
  `ProviderPanel.qml`, `PackagePanel.qml`

**Engine Adapters (pure C++23, no Qt)**:
- `include/gspl/studio/adapters/godot_adapter.hpp` `src/studio/adapters/godot_adapter.cpp` — Godot Engine 4.x export (tres, gd script, animation library)
- `include/gspl/studio/adapters/unity_adapter.hpp` `src/studio/adapters/unity_adapter.cpp` — Unity Engine 2022 LTS export (ScriptableObject, animation controller)
- `include/gspl/studio/adapters/unreal_adapter.hpp` `src/studio/adapters/unreal_adapter.cpp` — Unreal Engine 5.x export (DataAsset, module descriptor, uplugin)

**Reference Workspaces**:
- `examples/blank-workspace/` — Minimal GSPL project template
- `examples/voltfox-workspace/` — Complete Voltfox reference entity with morphology/animation modules
- `examples/sprite-kit-workspace/` — Design pattern templates (HumanoidBase, QuadrupedBase, FlyingBase)

**Tests (4 new test targets, 18 test functions)**:
- `tests/studio/ipc_tests.cpp` — Envelope roundtrip, shared memory fields, special characters
- `tests/studio/project_tests.cpp` — Create/load, invalid directory detection
- `tests/studio/project_tests_comprehensive.cpp` — Undo stack, workspace lifecycle
- `tests/studio/document_tests.cpp` — Kind, dirty, callbacks, file path
- `tests/studio/theme_tests.cpp` — Hex conversion, built-in loading, activation, contrast ratios
- `tests/studio/benchmark_stubs.cpp` — Performance benchmark framework
- `tests/ls/ls_tests.cpp` — Initialize, completions, diagnostics, symbol info, location JSON
- `tests/plugins/` — (directory ready for plugin tests)

**Tests (this session — 5 new targets, 57 test functions)**:
- `tests/ls/ls_full_tests.cpp` — 24 tests: JSON serialization, filtering, LS lifecycle, diagnostics, navigation stubs
- `tests/studio/environment_tests.cpp` — 14 tests: allowlist, override, unset-vs-empty, path validation, determinism
- `tests/studio/studio_core_tests.cpp` — 30 tests: publishing CRUD, target adapter lifecycle, theme color/contrast/built-in
- `tests/ls/property_based_tests.cpp` — 15 tests: LS determinism, idempotency, stability, monotonicity
- `tests/studio/fuzz_studio_tests.cpp` — 4 tests: random hex colors, contrast bounds, theme load/activation
- `tests/studio/adapter_tests.cpp` — 12 tests: Godot/Unity/Unreal export, validation, consistency
- `tests/e2e_workflow_tests.cpp` — 8 tests: lex→parse→diagnose→symbols, large module stress
- `tests/plugins/plugin_tests.cpp` — 18 tests: manifest validation, PluginManager lifecycle, IPC envelope roundtrip, sandbox config

**Tests (this session — 4 new targets, 67 test functions)**:
- `tests/studio/text_editor_tests.cpp` — 25 tests: bracket matching, snippet expansion, search, squiggles, folding, quick-fix
- `tests/studio/artifact_explorer_tests.cpp` — 9 tests: entry listing, filtering, inspect, validate, invalidate
- `tests/studio/diff_parser_tests.cpp` — 9 tests: file/hunk/line parsing, GSPL filtering, stats
- `tests/studio/visual_editor_tests.cpp` — 24 tests: gene, morphology, form, animation, behavior, combat, graph models

**Build System**:
- `CMakeLists.txt` — Added `GSPL_BUILD_STUDIO` option, `add_subdirectory(src/studio)`, 14 new source files in `gspl_sprites_core`, `gspl_sprites_ls_tests` target, 5 new test targets, `gspl_sprites_plugin_sandbox` executable, benchmark target, install rules
- `CMakeLists.txt` — Added 9 visual editor source files, 4 new test targets (text_editor, artifact_explorer, diff_parser, visual_editor)
- `src/studio/CMakeLists.txt` — Qt6 find_package, gspl_studio library target with Qt6 deps, test target

### Implemented (this session) — Package Fixes, Benchmarks, Remote Registry, CI

**Bug Fix**:
- `src/studio/packages/manifest.cpp` `src/plugins/manifest.cpp`: Fixed dangling `std::string_view` in `SimpleJsonParser` (`string_view` to `string`). Root cause of 4 failing package management tests.

**Performance Benchmarks (30.2-30.6, 30.8)**:
- `include/gspl/studio/benchmark.hpp` — Benchmark framework (BenchmarkRegistry, Timer, BENCHMARK_SCOPE)
- `tests/studio/benchmark_stubs.cpp` — Real compilation throughput benchmarks using GsplContext (small/medium/large, memory, regression detection)
- `CMakeLists.txt` — Added `gspl_sprites_benchmarks` test target

**Remote Registry HTTP Client (21.7)**:
- `include/gspl/package/remote_registry.hpp` `src/studio/packages/remote_registry.cpp` — RemoteRegistry with WinHTTP (Windows) / stub (Linux), list_versions, fetch, download, publish
- `CMakeLists.txt` — Added `src/studio/packages/remote_registry.cpp` to core library

**Package Creation Wizard (21.4)**:
- `include/gspl/package/package_creator.hpp` `src/studio/packages/package_creator.cpp` — PackageCreator with metadata validation, directory structure generation, manifest.jsonc, example source, README
- `src/studio/packages/PackageCreationWizard.qml` — 5-step QML wizard (metadata, authors/tags, dependencies, options, review)
- `CMakeLists.txt` — Added `src/studio/packages/package_creator.cpp` to core library

**OS Dark Mode Detection (25.6)**:
- `include/gspl/studio/dark_mode.hpp` `src/studio/dark_mode.cpp` — detect_color_scheme() via Windows registry / Linux gsettings
- `CMakeLists.txt` — Added `src/studio/dark_mode.cpp` to core library

**QML Panels**:
- `src/studio/plugins/PluginPanel.qml` — Plugin management UI (list, enable/disable, install, remove)
- `src/studio/shell/ThemeSettings.qml` — Theme settings panel (color scheme, accent color, font, size)

**Workspace Validation (33.4)**:
- `tests/studio/workspace_validation_tests.cpp` — 3 tests verifying blank/voltfox/sprite-kit workspace structure

**Build & CI**:
- `CMakeLists.txt` — Install rules for gsplc/gspl-sprites/studio binaries and public headers
- `.github/workflows/ci.yml` — Added Debug/Release matrix, benchmark step in Release, preserved studio and core-only-linux jobs
- `scripts/package-release.ps1` — Release packaging script (binary copy, headers, examples, docs, NSIS, ZIP)

**Tasks Updated**:
- `tasks.md` — 26 new checkboxes marked [x] (7.9, 7.10, 7.11, 7.12, 7.13, 8.5, 9.4, 10.4, 11.5, 12.4, 13.5, 14.9, 18.2, 18.3, 24.2)

### Implemented (this session) — Text Editor, Artifact Explorer, Diff Parser, Visual Editor C++ Models

**Text Editor C++ Backends (7.4, 7.6, 7.9, 7.10, 7.12, 7.13)**:
- `include/gspl/studio/bracket_matcher.hpp` `src/studio/text/bracket_matcher.cpp` — Bracket matching and auto-indent
- `include/gspl/studio/snippet_engine.hpp` `src/studio/text/snippet_engine.cpp` — Snippet expansion with 10 built-in GSPL snippets
- `include/gspl/studio/search_engine.hpp` `src/studio/text/search_engine.cpp` — Incremental search, find/replace, regex/whole-word
- `include/gspl/studio/squiggle_generator.hpp` `src/studio/text/squiggle_generator.cpp` — Diagnostic-to-squiggle conversion
- `include/gspl/studio/code_folding.hpp` `src/studio/text/code_folding.cpp` — Code folding region detection and toggle
- `include/gspl/studio/quick_fix.hpp` `src/studio/text/quick_fix.cpp` — Quick-fix suggestion with 6 built-in fixers

**Artifact Explorer Model (18.2-18.3)**:
- `include/gspl/studio/artifact_explorer_model.hpp` `src/studio/artifacts/artifact_explorer_model.cpp` — Entry listing, filtering, inspect/validate

**Diff Parser (24.2)**:
- `include/gspl/studio/diff_parser.hpp` `src/studio/git/diff_parser.cpp` — Unified diff parser with GSPL filtering

**Visual Editor C++ Models (8.5, 9.4, 10.4, 11.5, 12.4, 13.5, 14.9)**:
- All 7 model implementations and headers (gene, morphology, form, animation, behavior, combat, graph)

**Fixes**:
- `squiggle_generator.hpp` — Added missing `#include <functional>`
- `search_engine.cpp` — Fixed unused variables and nonexistent `m.line_start()` method
- `code_folding.cpp` — Removed unused `visible` variable
- `quick_fix.cpp` — Fixed unused `diag` parameter
- `diff_parser.cpp` — Fixed most-vexing-parse `istringstream` and `size_t` underflow in `is_gspl()`
- All visual editor model implementations store mutable entry lists for proper model editing

### Implemented (this session) — Project Tree, Git Extensions, Cache Ops, i18n

**Project Tree Model (5.2, 5.3, 5.5)**:
- `include/gspl/studio/project_tree_model.hpp` `src/studio/tree/project_tree_model.cpp` — TreeEntry with kind/size/git/compile status, recursive build from filesystem, flat listing, file CRUD (create_file, create_directory, rename, remove, read_file), filter_by_kind, status provider functions for git changes and compile errors

**Git Branch Operations (24.3–24.6)**:
- `include/gspl/studio/git_integration.hpp` `src/studio/git_integration.cpp` — Added `list_branches()`, `create_branch()`, `delete_branch()` methods
- Stage/unstage/commit/blame were already implemented and tested

**Artifact Cache Operations (18.4–18.5)**:
- `include/gspl/studio/artifact_explorer_model.hpp` `src/studio/artifacts/artifact_explorer_model.cpp` — Added `prune(target_bytes)` evicting oldest entries, `verify_integrity()` per-entry data check

**i18n Framework (27.5)**:
- `CMakeLists.txt` — Added `GSPL_BUILD_TRANSLATIONS` option with `qt6_create_translation` for `.ts`→`.qm` compilation
- Created `translations/` directory

**Tests (2 new targets, 11 test functions)**:
- `tests/studio/tree_model_tests.cpp` — 7 tests: empty directory, with files, create file, create directory, rename & remove, filter by kind, status providers
- `tests/studio/git_ext_tests.cpp` — 4 tests: list_branches, create & delete branch, cache prune, cache verify integrity

**Tasks Updated**:
- `tasks.md` — 10 new checkboxes marked [x] (5.3, 5.5, 18.4, 18.5, 24.3, 24.4, 24.5, 24.6, 27.5)

### Remaining for Gate Completion (requires Qt6 for full build)
- Full Qt6 build and test execution (CI)
- Text editor QML integration (multi-cursor, minimap, split-view, bracket highlighting)
- Visual editor QML enhancements (gene/morphology/form/animation/behavior/combat editors)
- Graph editor node canvas interactions
- Preview system, debugger, replay, artifact explorer
- Provider management, package creation wizard, plugin panel QML
- Accessibility audit (26.1-26.8) and i18n setup (27.1-27.4)
- Theming settings panel, OS dark mode detection
- Release build verification on Windows (MSVC + Qt6) and Linux
- Commit and archive the change

### Next Milestones
- Complete Qt6 build in CI and verify studio tests
- Implement remaining Qt6-dependent QML components
- Accessibility audit and i18n
- Final validation and archiving
