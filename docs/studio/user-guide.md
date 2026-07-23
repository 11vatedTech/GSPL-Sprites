# GSPL Authoring Studio User Guide

## Workspace Management

A **workspace** is a directory containing one or more GSPL projects. To create one:

- **Startup Wizard**: On first launch, choose **Create New Project**
- **File > New Workspace**: Creates an empty workspace at a chosen path
- **File > Open Workspace**: Opens an existing workspace directory

To add or remove projects within a workspace, use **Workspace > Add Project** or right-click in the file explorer.

## Project Structure

Each project is a directory with:

```
my-project/
├── gspl-project.jsonc       — Project manifest (name, version, dependencies)
├── src/                      — Source files
│   ├── entity.gspl
│   ├── morphology.gspl
│   └── animation.gspl
└── assets/                   — Optional: textures, metadata
```

The `gspl-project.jsonc` manifest declares the project name, semantic version, author, and any inter-project dependencies.

## Text Editor

Open any `.gspl` file from the file explorer to edit in the built-in text editor:

- **Syntax highlighting** for GSPL keywords, types, literals, and comments
- **Error diagnostics** appear as wavy underlines and in the Problems panel
- **Code completion** (Ctrl+Space) offers context-sensitive suggestions
- **Hover info** shows type signatures and documentation

Keyboard shortcuts: `Ctrl+S` save, `Ctrl+Z/Y` undo/redo, `Ctrl+F` find, `Ctrl+H` find and replace.

## Visual Editors

Six visual editors accessed via tabs at the bottom of the editor area:

| Editor | Purpose |
|--------|---------|
| **Gene Editor** | Define genetic traits and heritable properties |
| **Morphology Editor** | Design body structure — limbs, segments, joints |
| **Form Editor** | Configure visual form — shapes, colors, textures |
| **Animation Editor** | Create and blend animation sequences |
| **Behavior Editor** | Define state machines and event-driven behaviors |
| **Combat Editor** | Configure combat stats, hitboxes, damage zones |

Each editor provides form fields, property grids, and live preview updates as you make changes.

## Graph Editor

The **Graph Editor** provides a node-based schematic canvas for:

- Visualizing data flow between sprite components
- Connecting node outputs to inputs via drag-and-drop
- Creating subgraphs to encapsulate reusable logic
- Zooming and panning with mouse or touch gestures

Access via **View > Graph Editor** or the graph tab in any visual editor.

## Preview System

The **PreviewViewport** renders your sprite in three modes:

- **Canonical** — Single sprite view with rotation and zoom
- **Spritesheet** — Grid layout showing all animation frames
- **Runtime** — Interactive playback with speed control

Use **View > Preview** to open the viewport. Side-by-side comparison mode lets you diff two versions.

## Debugger

The **Debugger Panel** (Ctrl+Shift+D) supports:

- **Breakpoints**: Click the gutter in the text editor to set breakpoints
- **Step Through**: Step over, step into, and step out of synthesis operations
- **Call Stack**: View the current call stack with source locations
- **Variables**: Inspect sprite state, local variables, and watch expressions
- **Watch**: Add custom expressions to monitor during synthesis

## Replay

The **Replay Panel** captures synthesis traces for debugging and analysis:

- **Capture**: Record a trace during synthesis
- **Step**: Advance frame-by-frame through the trace
- **Compare**: Overlay two traces to find differences
- **Export**: Save traces for sharing or regression testing

## Plugin System

Plugins extend the Studio with custom functionality.

- **Installing plugins**: Place a `.gspl-plugin` directory in `workspace/plugins/` or the user config directory, then enable via **Plugins > Manage**
- **Managing plugins**: The Plugin Panel lists installed plugins with activate/deactivate toggles, version info, and dependency status
- **Lifecycle**: Plugins go through discover → load → activate → deactivate → unload

See the [Plugin Dev Guide](plugin-dev.md) for writing your own.

## Package Management

Packages are reusable sprite components distributed via registries.

- **Install**: `Package > Install Package` — specify a package name and version
- **Create**: `Package > Create Package` — packages the current project with manifest
- **Dependency resolution**: Automatic DFS-based resolution with conflict detection

## Publishing

Publish your workspace to distribution channels.

- **Local registry**: Publish to a local directory for testing
- **GitHub**: Publish as a GitHub release
- **SpriteForge**: Publish to the community registry
- **Versioning**: Semantic versioning with channels (stable, beta, nightly)
- **Rollback**: Revert to a previous published version

## Target Adapters

Target adapters configure builds for different platforms.

- **Build profiles**: Select from profiles like `windows-msvc`, `linux-gcc`, `wasm`
- **SDK detection**: Automatic detection of installed SDKs and toolchains
- **Cross-compilation**: Configure cross-compilation toolchains

Access via **Build > Target Adapters**.

## Git Integration

Basic Git operations from within the Studio.

- **View changes**: Unstaged and staged diffs in the Git Panel
- **Stage/Unstage**: Select files to include in a commit
- **Commit**: Write a commit message and commit
- **Log**: Browse commit history
- **Blame**: Annotate source lines with commit authorship

Access via **View > Git Panel**.

## Theming

Switch between light, dark, and high-contrast themes via **View > Themes**. Custom themes can be placed in `workspace/themes/` or the user config directory. See the [Theming Guide](theming.md) for details.

## Keyboard Shortcuts Summary

| Shortcut | Action |
|----------|--------|
| `Ctrl+S` | Save |
| `Ctrl+Shift+P` | Command Palette |
| `Ctrl+Z` / `Ctrl+Y` | Undo / Redo |
| `Ctrl+F` | Find in file |
| `Ctrl+Space` | Code completion |
| `F5` | Build workspace |
| `Ctrl+Shift+D` | Toggle debugger |
| `Ctrl+P` | Quick open |
| `Ctrl+Shift+F` | Find in workspace |
