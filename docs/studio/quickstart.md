# GSPL Authoring Studio Quickstart

## What is GSPL Authoring Studio?

GSPL Authoring Studio is a visual and textual development environment for creating, editing, debugging, and publishing GSPL sprites. It combines a Qt6/QML graphical shell with an LSP-compatible language service, visual editors for sprite components, a semantic debugger, and publishing tooling.

## System Requirements

- **OS**: Windows 10/11 (x64)
- **Qt**: Qt 6.5+ (required for the Studio UI)
- **Build Tools**: Visual Studio 2022 Build Tools or MSVC 19.44
- **CMake**: 3.22+
- **Optional**: ONNX Runtime (not required for `GSPL_CORE_ONLY` builds)

> Core-only builds (no Studio) also support Linux GCC via the `GSPL_CORE_ONLY` profile.

## Building from Source

```powershell
cmake -B build -S . -DGSPL_BUILD_STUDIO=ON
cmake --build build
```

The first command configures the project with the Studio target. The second builds all binaries. For a Release build, add `-DCMAKE_BUILD_TYPE=Release`.

## First Launch

1. Run the built `gspl-studio` executable from `build/src/studio/`
2. The **Startup Wizard** appears — choose **Create New Project**
3. Enter a project name and location. A `gspl-project.jsonc` manifest and `src/` directory are generated
4. The **MainWindow** opens with the text editor, file explorer, and tool panes

## Opening the Voltfox Example

```powershell
File > Open Workspace > browse to examples/voltfox-workspace/
```

This loads a complete reference entity with morphology, animation, and behavior modules.

## Basic Workflow

1. **Edit** a `.gspl` source file in the text editor — syntax highlighting activates automatically
2. **Diagnostics** from the Language Service appear inline and in the Problems panel
3. **Preview** your sprite in the PreviewViewport — switch between canonical, spritesheet, and runtime modes
4. **Build** your workspace using F5 or Build > Build Workspace

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+S` | Save current document |
| `Ctrl+Shift+P` | Command Palette |
| `F5` | Build workspace |
| `Ctrl+Z` / `Ctrl+Y` | Undo / Redo |
| `Ctrl+Shift+D` | Toggle debugger panel |
| `Ctrl+P` | Quick open file |

## Where to Go Next

- [User Guide](user-guide.md) — comprehensive manual
- [Plugin Dev Guide](plugin-dev.md) — writing plugins
- [Architecture Overview](architecture.md) — system design
- [API Reference](api.md) — SDK classes and namespaces
