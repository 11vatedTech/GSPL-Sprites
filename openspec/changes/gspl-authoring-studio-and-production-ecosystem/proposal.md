## Why

GSPL 1.0 has a complete compiler, SDK, CLI, passes, provider abstraction, and legacy migration — but no professional authoring environment. Designers and developers creating living GSPL sprites need a desktop IDE with visual editors, semantic debugging, cross-representation preview, packaging, publishing, and plugin extensibility. Without it, GSPL remains a command-line-only tool inaccessible to the broader audience of living-sprite creators.

## What Changes

### Studio Shell (new)
- Qt 6 / QML desktop application shell with project/workspace/document management
- MDI windowing, menu system, tool windows, status bar, command palette
- Theming (light/dark/high-contrast) and accessibility (WCAG 2.2 AA)
- Internationalization framework
- Git-aware project context: diff, stage, commit within the studio

### Language Service (new)
- GSPL-specific language server protocol implementation (or embedded equivalent)
- Real-time diagnostics (syntax, type, semantic errors) as you type
- Code completion, hover info, go-to-definition, find references, rename
- Outline view, document symbols, folding ranges
- Inline documentation rendering

### Text Authoring (new)
- GSPL syntax highlighting with full token-class colorization
- Multi-cursor editing, bracket matching, auto-indent, code folding
- Split-view editing, minimap, incremental search
- Inline error/warning squiggles with quick-fix suggestions

### Visual Authoring (new)
- Gene editor: typed gene contract creation, dependency management, conflict resolution
- Morphology editor: visual limb/segment layout, joint configuration, attachment points
- Form editor: color palette, gradient, pattern, material, emissive, scale, opacity
- Animation editor: timeline-based keyframe editor, state machine graph view, blending curves
- Behavior editor: condition/action rules, priority scheduling, stimulus mapping
- Combat editor: ability trees, cooldown curves, damage/hitbox configuration, targeting rules

### Graph Editor (new)
- Node-based graph/schematic editor for animation state machines, behavior trees, combat flows, gene dependency graphs
- Drag-and-drop node placement, edge routing, input/output port mapping
- Subgraph grouping, minimap, zoom-to-fit

### Preview System (new)
- Cross-representation preview: canonical → sprite sheet → runtime rendering
- Real-time property adjustment with immediate feedback
- Multi-viewport: canonical model view, sprite sheet view, runtime simulation view
- Side-by-side comparison across entity versions
- Provider-side preview integration for inference-based synthesis

### Semantic Debugger (new)
- GSPL runtime debugger: breakpoints (line, gene, field, expression), step-over/into/out
- Call stack, watch expressions, variable inspection (structured tree view)
- State timeline: step through deterministic synthesis history
- Conditional breakpoints and expression evaluation
- Attach to running synthesis or load saved traces

### Replay System (new)
- Deterministic replay from captured synthesis traces
- Frame-by-frame stepping with full state exposure
- Trace capture at configurable granularity (full, seed-only, gene-level)
- Comparison across replays to detect non-determinism

### Artifact Explorer (new)
- Tree view of compiled artifact graph with content-addressed keys
- Inspect/inspect-diff/validate subcommands mirrored from CLI
- Manual invalidation and cache pruning
- Integrity verification on demand

### Provider Management (new)
- Provider registry UI: install, configure, enable/disable providers
- Provider-side preview rendering in studio
- Monitoring: inference latency, throughput, error rates
- Provider isolation via worker process

### Plugin System (new)
- Declarative plugin manifest (ID, version, hooks, capabilities, dependencies)
- Plugin lifecycle: discover, load, activate, deactivate, unload
- Sandboxed worker process isolation for plugins
- Hook points: editor extension, provider integration, tool window contribution, menu extension
- Plugin SDK and packaging

### Package Management (new)
- Package creation with manifest, versioning, dependency resolution
- Hierarchical package identifiers (publisher/package/version)
- Local registry, remote registry pull/push
- Package verification (integrity hash, author signature)
- Package installation into workspace

### Publishing System (new)
- Publishing workflow: version bump, changelog, signing, upload
- Publishing channels: stable, beta, nightly
- Publishing targets: local registry, GitHub Releases, SpriteForge (future)
- Publishing validation: integrity, license, metadata completeness

### Target Adapters (new)
- Build profiles for compile-target pairs (e.g., desktop-runtime-v1, mobile-runtime-v1)
- Target-specific optimizations and constraints
- Target SDK detection and validation
- Cross-compilation support

### Performance Benchmarks (new)
- Benchmarking framework with configurable workloads
- Compilation throughput, preview FPS, provider inference latency
- Memory usage tracking, artifact size tracking
- Regression detection against baselines

### Security Model (new)
- Process isolation for compiler, provider, preview, and plugin workers
- Workspace-scoped authority model
- Package signature verification
- No network access from compiler/runtime core
- Input validation on all external data paths

### Observability (new)
- Structured logging with severity levels and component tags
- Telemetry bus for studio-internal instrumentation
- System health dashboard
- Diagnostics export for crash reporting

## Capabilities

### New Capabilities
- `studio-shell`: Application shell, window management, project/workspace/document model, theming, accessibility, i18n
- `language-service`: GSPL language server: parse, diagnose, complete, navigate, refactor
- `text-authoring`: Text editor with GSPL syntax highlighting, inline diagnostics, editing features
- `visual-authoring`: Visual gene/morphology/form/animation/behavior/combat editors
- `graph-editor`: Node-based graph/schematic editor for state machines, behavior trees, combat flows
- `preview-system`: Cross-representation preview rendering and comparison
- `semantic-debugger`: GSPL runtime breakpoints, stepping, watch, state inspection
- `replay-system`: Deterministic trace capture and replay
- `artifact-explorer`: Compiled artifact browsing, inspection, validation
- `provider-management`: Provider registration, configuration, monitoring, preview integration
- `plugin-system`: Plugin lifecycle, sandboxing, registry, manifest, SDK
- `package-management`: Package creation, versioning, dependency resolution, registries
- `publishing`: Publishing pipeline, signing, validation, distribution
- `target-adapter`: Target platform build profiles and SDK detection
- `git-integration`: Git-aware diff, stage, commit within studio
- `performance-benchmarks`: Benchmarking framework, regression detection
- `security-model`: Process isolation, workspace authority, input validation
- `observability`: Structured logging, telemetry, health dashboard
- `theming`: Theme system (light/dark/high-contrast), user-customizable

## Impact

- `CMakeLists.txt`: New components (studio shell, plugins, providers, tests), new Qt6/QML dependency, new build targets
- `src/studio/`: New Qt/QML source tree for the desktop application
- `src/ls/`: New language service implementation
- `src/plugins/`: Plugin infrastructure and SDK
- `include/gspl/studio/`: Public studio SDK headers
- `include/gspl/ls/`: Language service interfaces
- `tests/studio/`: Studio test suite (unit, integration, UI snapshot)
- `tests/ls/`: Language service tests
- `tests/plugins/`: Plugin system tests
- `.github/workflows/ci.yml`: Extended CI matrix for Qt build, UI tests, performance benchmarks
- `openspec/specs/`: 19 new capability specs + design + tasks
