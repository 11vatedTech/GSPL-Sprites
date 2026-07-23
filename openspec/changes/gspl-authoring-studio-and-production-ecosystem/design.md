## Context

GSPL 1.0 compiler is complete (61/61 tests, all profiles, CI defined) but has no graphical authoring environment. The existing codebase is a C++23 command-line compiler with typed passes, SDK, artifact cache, provider abstraction, legacy migration, and UTF-8 validation. Building a professional IDE requires a Qt 6 / QML desktop shell, process-isolated workers for compile/preview/provider/plugin, a language service for real-time diagnostics, visual editors, a semantic debugger, replay, package management, and publishing — all while keeping the compiler/runtime core unchanged (no network access, no new globals, no trusted external input).

### Constraints
- Compiler/runtime core must not gain network access, new global state, or untrusted-input trust
- All workers must be process-isolated (no in-process third-party code)
- No Electron unless Qt 6 is evaluated and found insufficient — Qt 6 is the presumptive choice
- Must build on Windows (MSVC), Linux (GCC CORE_ONLY), and Windows CI (MSVC via GitHub Actions)
- AGENTS.md engineering contract applies: deterministic by default, typed, bounded, validated, meaningful tests
- 61 existing tests must continue to pass

## Goals / Non-Goals

**Goals:**
- Desktop application shell (Qt 6 / QML) with project/workspace/document management
- GSPL language service providing real-time diagnostics, completion, navigation
- Text editor with full GSPL authoring support
- Visual editors for genes, morphology, form, animation, behavior, combat
- Graph editor for state machines, behavior trees, combat flows
- Cross-representation preview (canonical → sprite sheet → runtime)
- Semantic debugger with breakpoints, stepping, state inspection, watch
- Deterministic replay from captured traces
- Artifact explorer for compiled output
- Provider management with worker isolation
- Plugin system with sandboxed workers
- Package management and publishing
- Target adapters for platform build profiles
- Theming (light/dark/high-contrast), accessibility (WCAG 2.2 AA), i18n
- Git integration (diff, stage, commit within studio)
- Performance benchmarks, security model, observability

**Non-Goals:**
- Edit the compiler core or add network access to it
- Replace the existing CLI — studio wraps CLI tools
- Browser-based IDE — this is a native desktop application
- Mobile or web target for the studio itself (target adapters are for sprite output)
- Real-time collaborative editing (multi-user) in v1
- Built-in asset creation (raster/vector art tools) — asset pipeline is separate
- Self-host the studio (studio is not written in GSPL)

## Decisions

### D1: Qt 6 / QML (not Electron)
**Choice**: Qt 6.8+ with Qt Quick (QML) for UI, C++ backends for services
**Rationale**: Direct C++23 integration with existing compiler; no IPC overhead for in-process services; native performance for preview rendering; no Chromium memory footprint; well-established desktop UI framework with accessibility (WCAG via Qt Quick accessibility), theming, i18n. Electron would add 200+ MB per install and force JS interop layer for C++ compiler calls.
**Alternatives considered**: Electron (rejected: memory, complexity, JS-C++ bridge), FLTK (rejected: no QML/declarative UI), Dear ImGui (rejected: not a desktop app framework), Sciter (rejected: proprietary, small ecosystem)

### D2: Process isolation via QProcess + versioned JSON IPC
**Choice**: Worker processes launched via QProcess, communicating over stdin/stdout with a structured JSON protocol (versioned, typed envelopes). Shared memory (QSharedMemory) for large payloads (preview frames, compiled artifacts).
**Rationale**: QProcess is cross-platform, zero extra dependencies, and provides natural stdin/stdout/stderr separation. JSON is debuggable, versionable, and sufficient for command/response latency. Shared memory avoids serializing multi-MB sprite sheets through pipe.
**Alternatives considered**: Named pipes (rejected: platform-specific), gRPC (rejected: heavyweight for local IPC, protobuf compilation overhead), Unix domain sockets (Windows support immature in Qt 6), ZeroMQ (rejected: extra dependency, overkill for 1:1 worker communication)

### D3: Language service as embedded C++ library + optional LSP bridge
**Choice**: Language service is a C++ library (`libgspl-ls`) that the studio links directly for in-process use, and separately a standalone LSP server binary for external editors.
**Rationale**: In-process gives zero-latency diagnostics during typing; LSP bridge enables VSCode/Neovim/etc. support. Shared codebase between both. Reuse existing compiler frontend (lexer, parser, name resolution, type checking) from `libgspl` — no separate fork.
**Alternatives considered**: LSP-only (rejected: IPC latency for every keystroke), tree-sitter grammar (rejected: EBNF grammar already exists, tree-sitter would be a separate parser to maintain)

### D4: Plugin system with shared library + sandboxed worker
**Choice**: Plugins are compiled shared libraries (.dll/.so) loaded in a dedicated worker process. The plugin SDK provides a stable C ABI. Plugins communicate with the main process via the same JSON IPC as other workers.
**Rationale**: Process isolation prevents plugin crashes from taking down the studio. Stable C ABI avoids C++ ABI compatibility issues across compilers. Worker process model is consistent with other isolation boundaries.
**Alternatives considered**: Script plugins (Lua/Python) — rejected: performance overhead for provider/render plugins; in-process plugins — rejected: crash risk, memory corruption

### D5: SQLite for workspace metadata
**Choice**: SQLite stores workspace settings, recent projects, window layout, bookmarks, breakpoints, trace metadata.
**Rationale**: Zero-configuration, single-file, battle-tested, Qt has built-in QSqlDatabase driver. No separate database server. File-per-workspace, no migration needed for v1 schema.
**Alternatives considered**: JSON files (rejected: no concurrent access safety, no query capability), LMDB (rejected: less Qt integration, overkill for metadata)

### D6: Project format = directory with gspl-project.jsonc
**Choice**: Each project is a directory containing `gspl-project.jsonc` (project manifest), `src/` (GSPL source), `artifacts/` (compiled output), `traces/` (debug traces), `packages/` (dependencies). Multi-project workspaces reference project directories.
**Rationale**: File-system-native, git-friendly, trivial to inspect/edit outside studio. JSONC allows comments for human readers. Compatible with existing CLI compiler (CLI compiles from directory).
**Alternatives considered**: Single monolithic project file (rejected: diff conflicts, poor scalability), database-only storage (rejected: non-portable, bad for git)

### D7: Undo/redo via command pattern with delta snapshots
**Choice**: Document model uses a command-pattern undo stack. Each mutation creates a command object that stores before/after deltas (JSON diff). Undo/redo applies/reverses the delta.
**Rationale**: Works for both text edits (via text buffer diffs) and visual editor changes (via model diffs). Delta snapshots are compact and composable. Uses existing JSON serialization of all model types.
**Alternatives considered**: Full snapshot per step (rejected: memory explosion for large files), operation-based undo (more complex, less general)

### D8: Preview system uses existing SDK + worker process
**Choice**: Preview worker links `libgspl` (SDK), loads a compiled artifact, and renders via a headless provider. Render output is pushed via shared memory to the main process.
**Rationale**: Reuses existing compilation pipeline and SDK. Zero code duplication. Worker isolation prevents render hangs from blocking UI.
**Alternatives considered**: In-process rendering (rejected: UI freeze risk), separate renderer binary (rejected: would duplicate SDK initialization)

### D9: Debugger uses trace capture at synthesis granularity
**Choice**: GSPL synthesis already produces deterministic output. Debugger inserts trace probes at pass boundaries, gene resolutions, and expression evaluations. Traces are captured to a ring buffer in the worker and streamed to the studio on demand.
**Rationale**: Leverages existing determinism guarantees. No modifications needed to the core synthesis path — tracing is opt-in via the SDK's trace callback. Ring buffer avoids unbounded memory during long runs.
**Alternatives considered**: Full execution log (rejected: unbounded memory), hardware breakpoints (not applicable to synthesis)

### D10: Accessibility via Qt Quick's built-in accessibility + custom focus management
**Choice**: Qt Quick offers Accessible.attachItem(), screen reader navigation, and keyboard navigation. Custom focus scopes for graph editors and visual editors. Target WCAG 2.2 AA.
**Rationale**: Qt Quick accessibility is mature in Qt 6.8+. Custom focus models for non-standard controls (graph nodes, timeline, viewport) require explicit implementation but avoid framework limitations.
**Alternatives considered**: UIA direct (rejected: Windows-only), AT-SPI direct (rejected: Linux-only)

## Risks / Trade-offs

- **[Risk] Qt 6 ABI stability across platforms**: MinGW, MSVC, and GCC have different Qt 6 builds → Mitigation: Provide a Qt 6.8+ build guide per platform; CI tests only on MSVC + Linux GCC. Use the plugin SDK C ABI to keep plugin compilation independent of Qt version.
- **[Risk] Process isolation overhead**: JSON serialization/deserialization for every IPC call adds latency → Mitigation: Batch IPC calls; use shared memory for payloads >64KB; keep hot paths (diagnostics after keystroke) in-process via language service library.
- **[Risk] Preview rendering performance**: Sprite sheet rendering at 60 FPS through IPC is demanding → Mitigation: Shared memory double-buffering; render at target FPS only when preview window is visible; throttle to 30 FPS when not interacted with.
- **[Risk] Plugin sandbox insufficient**: A malicious plugin could still escape via OS-level exploits → Mitigation: Plugin worker runs at reduced OS priority; no network access in plugin worker; restricted filesystem access (workspace only); signed plugin manifest verification.
- **[Risk] Git integration complexity**: Full git integration (diff, stage, commit) requires libgit2 or git CLI wrapping → Mitigation: Start with git CLI wrapping (reliable, well-defined interface); evaluate libgit2 for future rename/refactor tracking.
- **[Risk] 110 completion gates are extensive**: Risk of scope creep and unfinished features → Mitigation: Strict gating process; each gate has explicit passing criteria defined in specs; architectural foundation must support all gates even if some content is minimal.
- **[Risk] Cross-platform UI differences**: Qt Quick looks and behaves differently on Windows vs Linux → Mitigation: Abstract platform-specific behaviors behind QML singletons; CI tests on both platforms; custom styling via Qt Quick Controls 2 theme system.

### Trade-offs Accepted
- Native performance comes at the cost of longer initial build (Qt compilation, plugin SDK compilation)
- Process isolation adds architectural complexity but is mandatory for provider/plugin security
- JSON IPC is debuggable but slower than binary protocols — acceptable given the tollerate latency for design-time operations
- In-process language service means the studio cannot benefit from a separate LS process for crash recovery — trade-off accepted for latency
- 110 gates will require staged delivery — core editing + preview must ship before advanced features

## Open Questions

1. **Plugin SDK format**: Static archive (.a/.lib) or shared library (.dll/.so) for plugin development? → Decision deferred: start with shared library (consistent with plugin loading), revisit if static linking is required for plugin distribution.
2. **Update mechanism**: How will the studio self-update? → Not designing now: v1 ships with manual download, update system deferred to post-v1.
3. **SpriteForge publishing**: Future publishing target for community sprites → Placeholder in publishing spec, not implemented until the store exists.
4. **Multi-workspace sync**: Should workspace state sync across machines? → Deferred: no cloud sync in v1.
