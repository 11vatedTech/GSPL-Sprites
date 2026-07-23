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

### Important Details
- Repository `https://github.com/11vatedTech/GSPL-Sprites`, branch `main` (HEAD = 40f771e)
- MSVC 19.44 via Visual Studio 2022 Build Tools (local); MSVC via GitHub Actions CI (windows-2025); Linux CORE_ONLY via GitHub Actions CI (ubuntu-24.04)
- Linux GCC supported via `GSPL_CORE_ONLY` build profile (no ONNX Runtime dependency)
- Studio requires Qt6 (`GSPL_BUILD_STUDIO=ON`) — not available locally; core-only builds unaffected
- 61/61 tests pass (MSVC 19.44) — including gspl_sprites_ls_tests


### Changes

| Change | Status |
|---|---|
| `voltfox-reference-entity` | Archived |
| `voltfox-living-sprite-vertical` | Archived |
| `voltfox-living-sprite-vertical-v2` | Archived |
| `generalized-gspl-sprite-compiler` | Archived |
| `gspl-language-and-platform-completion` | Archived |
| `gspl-authoring-studio-and-production-ecosystem` | In progress (all 4 spec artifacts done, 132 new files, Qt6 build pending) |

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
- `include/gspl/ls/ls_server.hpp` `src/ls/ls_server.cpp` — LSP-compatible server with 12+ methods
- `include/gspl/ls/diagnostic.hpp` `src/ls/diagnostic.cpp` — Diagnostic with range/severity/message
- `include/gspl/ls/completion.hpp` `src/ls/completion.cpp` — CompletionItem with kind/detail/insert
- `include/gspl/ls/navigation.hpp` `src/ls/navigation.cpp` — Location, SymbolInfo, Reference, HoverInfo with JSON serialization

**Visual Editors (QML, Qt6-dependent)**:
- `src/studio/visual/GeneEditor.qml`, `MorphologyEditor.qml`, `FormEditor.qml`
- `src/studio/visual/AnimationEditor.qml`, `BehaviorEditor.qml`, `CombatEditor.qml`
- `src/studio/graph/GraphEditor.qml` — Node-based schematic canvas
- `src/studio/preview/PreviewViewport.qml` — 3-mode viewport (canonical/spritesheet/runtime)
- `src/studio/debugger/DebuggerPanel.qml` — Breakpoints, call stack, variables, watch
- `src/studio/replay/ReplayPanel.qml` — Frame-by-frame trace replay
- `src/studio/artifacts/ArtifactExplorer.qml` — Tree + detail inspector

**Plugin System (pure C++23, stable C ABI)**:
- `include/gspl/plugin/plugin_api.h` — Stable C ABI (GsplPluginInfo, GsplPluginCallbacks)
- `include/gspl/plugin/manifest.hpp` `src/plugins/manifest.cpp` — JSONC manifest with dependency constraints
- `include/gspl/plugin/plugin_manager.hpp` `src/plugins/plugin_manager.cpp` — Lifecycle manager (discover/load/activate/deactivate/unload)

**Package Management (pure C++23)**:
- `include/gspl/package/manifest.hpp` `src/studio/packages/manifest.cpp` — PackageManifest with semver, signature
- `include/gspl/package/package_manager.hpp` `src/studio/packages/package_manager.cpp` — Install/update/remove with DFS dependency resolution

**Remaining Components (all implemented)**:
- `include/gspl/studio/publishing.hpp` `src/studio/publishing_manager.cpp` — Publish/rollback to local/GitHub/spriteforge targets
- `include/gspl/studio/target_adapter.hpp` `src/studio/target_adapter_manager.cpp` — Build profiles, SDK detection, cross-compilation
- `include/gspl/studio/theme.hpp` `src/studio/theme_manager.cpp` — Light/dark/high-contrast themes with WCAG contrast ratio computation
- `include/gspl/studio/git_integration.hpp` `src/studio/git_integration.cpp` — Git CLI wrapper (status, diff, stage, commit, blame)
- `src/studio/publishing/PublishWizard.qml`, `TargetPanel.qml`, `GitPanel.qml`,
  `ProviderPanel.qml`, `PackagePanel.qml`

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

**Build System**:
- `CMakeLists.txt` — Added `GSPL_BUILD_STUDIO` option, `add_subdirectory(src/studio)`, 12 new source files in `gspl_sprites_core`, `gspl_sprites_ls_tests` target
- `src/studio/CMakeLists.txt` — Qt6 find_package, gspl_studio library target with Qt6 deps, test target

### Remaining for Gate Completion (requires Qt6 for full build)
- Full Qt6 build and test execution (CI)
- Text editor implementation (QML + syntax highlighter)
- Accessibility audit and i18n setup
- Performance benchmark baselines
- CI studio builds
- Documentation (quickstart, user guide, plugin dev guide)
- Release packaging
- Final validation and archiving

### Next Milestones
- Complete remaining gates: CI studio build, documentation, accessibility audit, release packaging
- Archive `gspl-authoring-studio-and-production-ecosystem`
