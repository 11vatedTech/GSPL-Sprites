## 1. Project Foundation & Build

- [x] 1.1 Create `src/studio/` directory tree, stub CMakeLists.txt for Qt6/QML targets
- [x] 1.2 Add Qt6 Find/package resolution to root CMakeLists.txt (optional, not required for CORE_ONLY)
- [x] 1.3 Create `include/gspl/studio/` public header directory
- [x] 1.4 Add `GSPL_BUILD_STUDIO` option to CMake with CORE_ONLY guard
- [ ] 1.5 Create CI matrix entry for studio build (Windows MSVC + Qt6)
- [x] 1.6 Verify root build still passes with `GSPL_CORE_ONLY` (no Qt dep)
- [x] 1.7 Create `tests/studio/` and `tests/ls/` test directories with stub tests

## 2. IPC Worker Framework

- [x] 2.1 Define `include/gspl/studio/ipc_envelope.hpp` — versioned JSON envelope type
- [x] 2.2 Define `include/gspl/studio/ipc_channel.hpp` — stdin/stdout channel with framing
- [x] 2.3 Implement `src/studio/ipc_channel.cpp` — async reader/writer with timeout
- [x] 2.4 Define `include/gspl/studio/worker_process.hpp` — QProcess wrapper
- [x] 2.5 Implement `src/studio/worker_process.cpp` — launch, health-check, restart policy
- [x] 2.6 Define `include/gspl/studio/shared_memory.hpp` — QSharedMemory payload transfer
- [x] 2.7 Implement `src/studio/shared_memory.cpp` — double-buffered frame push
- [x] 2.8 Add IPC integration tests (echo worker, timeout, crash recovery)

## 3. Project & Workspace Model

- [x] 3.1 Define `include/gspl/studio/project.hpp` — Project class with manifest path
- [x] 3.2 Implement `src/studio/project.cpp` — load/save gspl-project.jsonc, validate
- [x] 3.3 Define `include/gspl/studio/workspace.hpp` — Workspace class with project list
- [x] 3.4 Implement `src/studio/workspace.cpp` — SQLite-backed workspace metadata
- [x] 3.5 Define `include/gspl/studio/document.hpp` — Document base class
- [x] 3.6 Implement `src/studio/document.cpp` — text doc and visual doc subclasses
- [x] 3.7 Implement command-pattern undo/redo stack
- [x] 3.8 Add project/workspace/document unit tests

## 4. Studio Shell (Qt Quick Application)

- [x] 4.1 Create `src/studio/shell/main.cpp` — QGuiApplication entry point
- [x] 4.2 Create `src/studio/shell/MainWindow.qml` — MDI shell with menu bar, tool windows
- [x] 4.3 Create `src/studio/shell/MenuBar.qml` — File/Edit/View/Project/Build/Tools/Help
- [x] 4.4 Create `src/studio/shell/StatusBar.qml` — progress, messages, worker status
- [x] 4.5 Create `src/studio/shell/CommandPalette.qml` — searchable action palette
- [x] 4.6 Create `src/studio/shell/StartupWizard.qml` — new/open/recent project
- [x] 4.7 Create `src/studio/shell/PreferencesDialog.qml` — settings editor
- [x] 4.8 Create `src/studio/shell/AboutDialog.qml` — version, credits, licenses
- [x] 4.9 C++ backend for shell actions (project open/close, window management)
- [x] 4.10 Implement recent files list with MRU ordering
- [ ] 4.11 Implement drag-and-drop file open from OS file manager

## 5. Project Tree & File Browsing

- [x] 5.1 Create `src/studio/tree/ProjectTree.qml` — file tree with source/artifacts/packages
- [x] 5.2 Create C++ project tree model (`src/studio/tree/ProjectTreeModel.cpp`)
- [ ] 5.3 Implement file creation/deletion/rename in project tree
- [ ] 5.4 Implement drag-drop file organization within tree
- [ ] 5.5 Add status indicators (Git dirty, error, compiled)

## 6. Language Service

- [x] 6.1 Define `include/gspl/ls/ls_server.hpp` — LsServer interface declaration
- [x] 6.2 Implement `src/ls/ls_server.cpp` — server loop over IPC channel
- [x] 6.3 Define `include/gspl/ls/diagnostic.hpp` — Diagnostic with range, severity, message
- [x] 6.4 Implement `src/ls/diagnostic.cpp` — map compile errors to source ranges
- [x] 6.5 Define `include/gspl/ls/completion.hpp` — CompletionItem with label, kind, detail
- [x] 6.6 Implement `src/ls/completion.cpp` — keyword, symbol, and context-aware completion
- [x] 6.7 Define `include/gspl/ls/navigation.hpp` — goto-definition, find-references, rename
- [x] 6.8 Implement `src/ls/navigation.cpp` — symbol index with source spans
- [x] 6.9 Implement `include/gspl/ls/document_symbols.hpp` — outline tree builder
- [x] 6.10 Implement `src/ls/document_symbols.cpp` — symbol tree from parsed module
- [ ] 6.11 Implement hover info (`include/gspl/ls/hover.hpp`, `src/ls/hover.cpp`)
- [x] 6.12 Create standalone LsServer main (`src/ls/main.cpp`) for external editors
- [x] 6.13 Add language service unit and integration tests

## 7. Text Authoring

- [x] 7.1 Create `src/studio/text/TextEditor.qml` — code editor component
- [x] 7.2 Implement GSPL syntax highlighter (`include/gspl/studio/syntax_highlighter.hpp`)
- [x] 7.3 Implement `src/studio/syntax_highlighter.cpp` — token-class colorization via lexer
- [ ] 7.4 Implement bracket matching and auto-indent
- [ ] 7.5 Implement multi-cursor editing
- [ ] 7.6 Implement code folding
- [ ] 7.7 Implement minimap
- [ ] 7.8 Implement split-view editing
- [ ] 7.9 Implement incremental search and replace
- [ ] 7.10 Implement inline error squiggles from language service diagnostics
- [ ] 7.11 Implement quick-fix suggestions and application
- [ ] 7.12 Implement code snippets
- [ ] 7.13 Add text editor unit and integration tests

## 8. Visual Authoring — Gene Editor

- [x] 8.1 Create `src/studio/visual/GeneEditor.qml` — gene contract editor panel
- [ ] 8.2 Implement gene dependency graph visualization and management
- [ ] 8.3 Implement gene conflict resolution UI
- [ ] 8.4 Implement gene composition with inheritance/override controls
- [ ] 8.5 C++ model for gene editing (`src/studio/visual/GeneEditorModel.cpp`)

## 9. Visual Authoring — Morphology Editor

- [x] 9.1 Create `src/studio/visual/MorphologyEditor.qml` — limb/segment layout canvas
- [ ] 9.2 Implement joint configuration panel
- [ ] 9.3 Implement attachment point management
- [ ] 9.4 C++ model for morphology editing

## 10. Visual Authoring — Form Editor

- [x] 10.1 Create `src/studio/visual/FormEditor.qml` — color palette and material editor
- [ ] 10.2 Implement gradient and pattern editor
- [ ] 10.3 Implement emissive, scale, opacity controls
- [ ] 10.4 C++ model for form editing

## 11. Visual Authoring — Animation Editor

- [x] 11.1 Create `src/studio/visual/AnimationEditor.qml` — timeline with keyframes
- [ ] 11.2 Implement keyframe manipulation (add, move, delete, interpolate)
- [ ] 11.3 Implement animation state machine graph view
- [ ] 11.4 Implement blending curve editor
- [ ] 11.5 C++ model for animation editing

## 12. Visual Authoring — Behavior Editor

- [x] 12.1 Create `src/studio/visual/BehaviorEditor.qml` — condition/action rule editor
- [ ] 12.2 Implement priority scheduling controls
- [ ] 12.3 Implement stimulus mapping editor
- [ ] 12.4 C++ model for behavior editing

## 13. Visual Authoring — Combat Editor

- [x] 13.1 Create `src/studio/visual/CombatEditor.qml` — ability tree editor
- [ ] 13.2 Implement cooldown curve editor
- [ ] 13.3 Implement damage/hitbox configuration
- [ ] 13.4 Implement targeting rule editor
- [ ] 13.5 C++ model for combat editing

## 14. Graph Editor

- [x] 14.1 Create `src/studio/graph/GraphEditor.qml` — node canvas component
- [ ] 14.2 Implement drag-and-drop node placement and connection
- [ ] 14.3 Implement edge routing with port snapping
- [ ] 14.4 Implement input/output port mapping panel
- [ ] 14.5 Implement subgraph grouping and collapse
- [ ] 14.6 Implement minimap navigation
- [ ] 14.7 Implement zoom-to-fit and auto-layout
- [ ] 14.8 Implement node property panel
- [ ] 14.9 C++ model for graph editing (`src/studio/graph/GraphModel.cpp`)

## 15. Preview System

- [x] 15.1 Create `src/studio/preview/PreviewViewport.qml` — render canvas component
- [ ] 15.2 Implement canonical model viewport (wireframe + semantic annotations)
- [ ] 15.3 Implement sprite sheet viewport (rendered cell grid)
- [ ] 15.4 Implement runtime simulation viewport (animated sprite playback)
- [ ] 15.5 Implement side-by-side comparison mode
- [ ] 15.6 Implement real-time property adjustment with preview feedback
- [ ] 15.7 Implement provider-side preview via IPC worker
- [ ] 15.8 Implement auto-refresh on source change (debounced)
- [ ] 15.9 Implement FPS control and performance overlay
- [ ] 15.10 Implement screenshot/recording export
- [ ] 15.11 Preview worker process harness (`src/studio/preview/PreviewWorker.cpp`)
- [ ] 15.12 Add preview integration tests

## 16. Semantic Debugger

- [x] 16.1 Create `src/studio/debugger/DebuggerPanel.qml` — debugger tool window
- [ ] 16.2 Implement breakpoint manager (line/gene/field/expression)
- [ ] 16.3 Implement step-over, step-into, step-out controls
- [ ] 16.4 Implement call stack display with navigation
- [ ] 16.5 Implement watch expression entry and evaluation
- [ ] 16.6 Implement variable inspection tree with structured values
- [ ] 16.7 Implement state timeline view (synthesis step history)
- [ ] 16.8 Implement conditional breakpoint configuration
- [ ] 16.9 Implement REPL for expression evaluation
- [ ] 16.10 Debugger worker harness (`src/studio/debugger/DebuggerWorker.cpp`)
- [ ] 16.11 Add debugger integration tests

## 17. Replay

- [x] 17.1 Create `src/studio/replay/ReplayPanel.qml` — replay control panel
- [ ] 17.2 Implement trace capture at configurable granularity
- [ ] 17.3 Implement frame-by-frame playback with state exposure
- [ ] 17.4 Implement trace load/save (`.gspl-trace` format)
- [ ] 17.5 Implement trace comparison for non-determinism detection
- [ ] 17.6 Add replay integration tests

## 18. Artifact Explorer

- [x] 18.1 Create `src/studio/artifacts/ArtifactExplorer.qml` — tree + detail panel
- [ ] 18.2 C++ model wrapping existing ArtifactCache (`ArtifactExplorerModel.cpp`)
- [ ] 18.3 Implement inspect/inspect-diff/validate command integration
- [ ] 18.4 Implement manual invalidation and cache pruning
- [ ] 18.5 Implement integrity verification trigger

## 19. Provider Management

- [x] 19.1 Create `src/studio/providers/ProviderPanel.qml` — provider configuration UI
- [ ] 19.2 Implement provider registry browsing (installed, available)
- [ ] 19.3 Implement provider install/configure/enable/disable
- [ ] 19.4 Implement provider monitoring dashboard (latency, throughput, errors)
- [ ] 19.5 Implement provider-side preview trigger
- [ ] 19.6 Provider worker process harness

## 20. Plugin System

- [x] 20.1 Define `include/gspl/plugin/plugin_api.h` — stable C ABI
- [x] 20.2 Define `include/gspl/plugin/manifest.hpp` — PluginManifest type
- [x] 20.3 Implement `src/plugins/manifest.cpp` — load/validate manifest.jsonc
- [x] 20.4 Implement `src/plugins/plugin_loader.cpp` — LoadLibrary/dlopen wrapper
- [x] 20.5 Implement plugin lifecycle manager (`src/plugins/PluginManager.cpp`)
- [ ] 20.6 Implement plugin sandbox worker process
- [ ] 20.7 Create `src/studio/plugins/PluginPanel.qml` — plugin management UI
- [ ] 20.8 Add plugin system unit and integration tests
- [ ] 20.9 Create sample plugin demonstrating all hook points

## 21. Package Management

- [x] 21.1 Define `include/gspl/package/manifest.hpp` — PackageManifest type
- [x] 21.2 Implement `src/studio/packages/PackageManager.cpp` — core package logic
- [x] 21.3 Create `src/studio/packages/PackagePanel.qml` — package management UI
- [ ] 21.4 Implement package creation wizard
- [x] 21.5 Implement dependency resolution with version constraints
- [x] 21.6 Implement local registry (file-system based)
- [ ] 21.7 Implement remote registry pull/push (HTTP)
- [x] 21.8 Implement package verification (integrity hash, signature)
- [ ] 21.9 Add package management tests

## 22. Publishing

- [x] 22.1 Create `src/studio/publishing/PublishWizard.qml` — publishing workflow
- [x] 22.2 Implement version bump and changelog generation
- [x] 22.3 Implement package signing
- [x] 22.4 Implement publishing targets (local registry, GitHub Releases)
- [x] 22.5 Implement publishing validation (integrity, license, metadata)
- [x] 22.6 Implement publishing history view and rollback
- [ ] 22.7 Add publishing integration tests

## 23. Target Adapters

- [x] 23.1 Define `include/gspl/studio/target_adapter.hpp` — TargetAdapter interface
- [x] 23.2 Implement `src/studio/targets/TargetAdapterManager.cpp` — registry and resolution
- [x] 23.3 Create built-in target adapters (desktop-runtime-v1)
- [x] 23.4 Create `src/studio/targets/TargetPanel.qml` — target configuration UI
- [x] 23.5 Implement target SDK detection and validation
- [x] 23.6 Implement cross-compilation profile application
- [ ] 23.7 Add target adapter unit tests

## 24. Git Integration

- [x] 24.1 Create `src/studio/git/GitIntegration.cpp` — git CLI wrapper
- [ ] 24.2 Implement diff view for GSPL source files
- [ ] 24.3 Implement stage/unstage and commit workflow
- [ ] 24.4 Implement branch display and switching
- [ ] 24.5 Implement status indicators in project tree
- [ ] 24.6 Implement blame annotations
- [x] 24.7 Create `src/studio/git/GitPanel.qml` — git tool window
- [ ] 24.8 Add git integration tests (with git test repo fixture)

## 25. Theming

- [x] 25.1 Define `include/gspl/studio/theme.hpp` — Theme type and ThemeManager
- [x] 25.2 Implement `src/studio/theme/ThemeManager.cpp` — load/apply themes
- [x] 25.3 Create built-in light theme
- [x] 25.4 Create built-in dark theme
- [x] 25.5 Create built-in high-contrast theme (WCAG 2.2 AA compliant)
- [ ] 25.6 Implement OS dark mode detection and auto-switch
- [ ] 25.7 Implement user-customizable accent color and font selection
- [x] 25.8 Implement theme file format (JSONC) and user theme loading
- [ ] 25.9 Create `src/studio/shell/ThemeSettings.qml` — theme settings panel
- [x] 25.10 Add theme unit tests

## 26. Accessibility

- [ ] 26.1 Audit all QML components for Accessible.attachItem() usage
- [ ] 26.2 Implement keyboard navigation for graph editor (tab, arrow, enter, escape)
- [ ] 26.3 Implement keyboard navigation for visual editors
- [ ] 26.4 Implement keyboard navigation for preview viewport
- [ ] 26.5 Implement focus indicators for all interactive controls
- [ ] 26.6 Implement screen reader announcements for state changes
- [ ] 26.7 Ensure WCAG 2.2 AA color contrast in all themes
- [ ] 26.8 Add accessibility integration tests with accessibility tooling

## 27. Internationalization

- [ ] 27.1 Set up Qt i18n framework (`.ts`/`.qm` files)
- [ ] 27.2 Extract all user-facing strings to tr() calls
- [ ] 27.3 Create English (en) translation file
- [ ] 27.4 Create locale selector in preferences
- [ ] 27.5 Add i18n build step to CMake

## 28. Security Model

- [x] 28.1 Implement process isolation for all worker types (compile, preview, provider, plugin)
- [ ] 28.2 Define workspace-scoped authority model (which dirs can workers access)
- [x] 28.3 Implement package signature verification
- [x] 28.4 Verify compiler core has no network access (audit includes)
- [x] 28.5 Implement input validation on all IPC messages
- [x] 28.6 Implement crash recovery with worker restart
- [ ] 28.7 Add security-focused integration tests

## 29. Observability

- [x] 29.1 Implement structured logging (`include/gspl/studio/logger.hpp`)
- [ ] 29.2 Implement telemetry bus for studio-internal instrumentation
- [x] 29.3 Create `src/studio/shell/HealthDashboard.qml` — system health display
- [ ] 29.4 Implement diagnostics export (log bundle)
- [ ] 29.5 Implement crash reporter
- [x] 29.6 Create `src/studio/shell/LogViewer.qml` — live log viewer
- [ ] 29.7 Implement metric collection and export
- [ ] 29.8 Add observability tests

## 30. Performance Benchmarks

- [x] 30.1 Create benchmark framework (`tests/studio/benchmarks/benchmark.hpp`)
- [ ] 30.2 Implement compilation throughput benchmark
- [ ] 30.3 Implement preview FPS benchmark
- [ ] 30.4 Implement provider inference latency benchmark
- [ ] 30.5 Implement memory usage tracking benchmark
- [ ] 30.6 Implement regression detection against stored baselines
- [ ] 30.7 Add benchmark CI step

## 31. CMake & Build System

- [x] 31.1 Add `GSPL_BUILD_STUDIO` option to root CMakeLists.txt
- [x] 31.2 Add Qt6 find_package with versioned requirement
- [x] 31.3 Create studio library CMakeLists (`src/studio/CMakeLists.txt`)
- [x] 31.4 Create LS library CMakeLists (`src/ls/CMakeLists.txt`)
- [x] 31.5 Create studio test CMakeLists (`tests/studio/CMakeLists.txt`)
- [x] 31.6 Create LS test CMakeLists (`tests/ls/CMakeLists.txt`)
- [ ] 31.7 Add install rules for studio binary
- [x] 31.8 Verify CORE_ONLY build still succeeds with no Qt dependency

## 32. CI & Release

- [x] 32.1 Add Windows MSVC + Qt6 CI job to `.github/workflows/ci.yml`
- [x] 32.2 Add Linux studio build to CI (reuse CORE_ONLY profile, no Qt6 on Linux for now)
- [x] 32.3 Add studio test execution to CI
- [ ] 32.4 Add benchmark CI step (nightly or on-demand)
- [ ] 32.5 Create release packaging script (NSIS for Windows, AppImage for Linux)

## 33. Reference Workspaces

- [x] 33.1 Create blank reference workspace (`examples/blank-workspace/`)
- [x] 33.2 Create Voltfox reference workspace (`examples/voltfox-workspace/`) with project source, compiled artifacts, Git history, theme
- [x] 33.3 Create sprite-kit reference workspace (`examples/sprite-kit-workspace/`) with design patterns and templates
- [ ] 33.4 Verify each workspace opens correctly in studio and compiles

## 34. Documentation

- [x] 34.1 Create `docs/studio/quickstart.md` — getting started guide
- [x] 34.2 Create `docs/studio/architecture.md` — system architecture overview
- [x] 34.3 Create `docs/studio/user-guide.md` — full user manual
- [x] 34.4 Create `docs/studio/plugin-dev.md` — plugin development guide
- [x] 34.5 Create `docs/studio/theming.md` — theming customization guide
- [x] 34.6 Create `docs/studio/api.md` — studio SDK API reference

## 35. Final Validation

- [ ] 35.1 Run full test suite (61 existing + all new studio/LS/plugin/package tests)
- [ ] 35.2 Verify all 110 completion gates pass
- [ ] 35.3 Verify release build on Windows (MSVC + Qt6)
- [ ] 35.4 Verify release build on Linux (CORE_ONLY profile)
- [x] 35.5 Update AGENTS.md with final state, HEAD, task status
- [ ] 35.6 Commit and push final state
- [ ] 35.7 Archive the change in OpenSpec
