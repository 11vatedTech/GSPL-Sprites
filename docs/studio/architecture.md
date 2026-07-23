# GSPL Authoring Studio Architecture

## Process Model

The Studio uses a multi-process architecture for isolation and stability:

- **Main Process** — Qt6/QML GUI shell, document model, undo stack, plugin manager
- **Compiler Worker** — Isolated process for compiling GSPL sources
- **Language Service Worker** — LSP-compatible server providing diagnostics, completions, navigation
- **Preview Worker** — Headless rendering for the preview viewport
- **Provider Worker** — Remote registry and provider communication
- **Plugin Workers** — Each plugin runs in its own sandboxed process

Worker crashes do not affect the main process. Automatic restart with crash-loop detection is built in.

## IPC Layer

Inter-process communication uses a two-tier protocol:

- **IpcChannel** — Length-prefixed JSON envelopes over stdin/stdout. Used for commands, requests, and small payloads.
- **SharedMemory** — Double-buffered `QSharedMemory` for large payloads (preview frames, compiled assets). Synchronized via the envelope channel.

The `IpcEnvelope` type carries version, message kind, correlation ID, and optional shared memory region metadata.

## Component Diagram

```
┌─────────────────────────────────────────────────────────┐
│                    Studio Shell (QML)                    │
│  MainWindow · CommandPalette · Preferences · About      │
└────────────────────┬────────────────────────────────────┘
                     │ QML → C++ bindings
┌────────────────────▼────────────────────────────────────┐
│                   C++ Backend                            │
│  Document Model · UndoStack · Project · Workspace       │
│  ThemeManager · GitIntegration · Logger                 │
├─────────────────┬─────────────────┬────────────────────┤
│ Text Editor     │ Visual Editors  │ Graph Editor        │
│ (QML + syntax   │ Gene · Morph    │ Node canvas         │
│  highlighter)   │ Form · Anim     │ (QML)               │
│                 │ Behavior·Combat  │                     │
├─────────────────┴─────────────────┴────────────────────┤
│                    IPC Layer                             │
│  IpcChannel · IpcEnvelope · WorkerProcess · SharedMem   │
└──┬──────────────┬──────────────┬──────────────┬─────────┘
   │              │              │              │
   ▼              ▼              ▼              ▼
┌──────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐
│Compiler│  │Language  │   │ Preview  │   │ Provider │
│Worker  │  │Service   │   │ Worker   │   │ Worker   │
└────────┘  │Worker    │   └──────────┘   └──────────┘
            └──────────┘

Plugin System ↔ Plugin Worker 1 · Plugin Worker 2 · ...
Package Mgmt ↔ Local / Remote Registries
Publishing ↔ Target Adapters (local, GitHub, SpriteForge)
```

## Key Design Decisions

- **Qt6/QML** — Modern declarative UI with hardware-accelerated rendering and C++ backend integration
- **Process Isolation** — Workers are separate OS processes; a crash never takes down the shell
- **SQLite** — Workspace metadata, project registry, and local caches use SQLite via workspace.cpp
- **Command-Pattern Undo** — `UndoStack` stores bounded-depth command history with full undo/redo
- **Stable C ABI for Plugins** — Plugin API uses `extern "C"` with a versioned struct interface

## Directory Layout

```
include/gspl/studio/     — Public SDK headers (Project, Workspace, Document, IPC, etc.)
include/gspl/ls/         — Language service server and types
include/gspl/plugin/     — Plugin API and manifest
include/gspl/package/    — Package manifest and management
src/studio/              — Implementation (shell QML, C++ backend, tests)
src/studio/shell/        — QML files (MainWindow, dialogs, editors)
src/studio/visual/       — Visual editor QML components
src/studio/graph/        — Graph editor QML
src/studio/preview/      — Preview viewport QML
src/studio/debugger/     — Debugger panel QML
src/studio/replay/       — Replay panel QML
src/studio/artifacts/    — Artifact explorer QML
src/studio/publishing/   — Publishing UI QML
src/ls/                  — Language service implementation
src/plugins/             — Plugin infrastructure
tests/studio/            — Studio unit tests
```

## Security Boundaries

- All worker processes are fully isolated — no shared address space
- Plugin workers are sandboxed; plugins cannot access the filesystem or network except through approved IPC callbacks
- The compiler and runtime core have no network access
- External inputs (files, package registries) are validated before processing
