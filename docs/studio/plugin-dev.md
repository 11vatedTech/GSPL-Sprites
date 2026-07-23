# GSPL Plugin Development Guide

## Architecture Overview

Plugins are shared libraries (.dll on Windows, .so on Linux) loaded by the Plugin Manager. Each plugin runs in an isolated worker process — a crash in a plugin never brings down the Studio. The plugin API is defined by a stable C ABI (`extern "C"`) so plugins can be written in any language that supports C linkage.

```
┌──────────────┐    IPC     ┌────────────────┐
│ Plugin       │◄──────────►│ Plugin Worker   │
│ Manager      │            │ (sandboxed proc)│
│ (main proc)  │            │ ┌──────────────┐│
└──────────────┘            │ │ plugin.dll   ││
                            │ │ callbacks    ││
                            │ └──────────────┘│
                            └────────────────┘
```

## Plugin Manifest

Each plugin ships with a `manifest.jsonc` file placed alongside the shared library:

```jsonc
{
  "id": "com.example.my-plugin",
  "version": "1.0.0",
  "name": "My Plugin",
  "description": "A minimal example plugin",
  "author": "Your Name",
  "capabilities": ["on_document_open", "on_build_end"],
  "dependencies": {
    "com.example.other-plugin": "^1.2.0"
  },
  "entry": "my_plugin.dll"
}
```

| Field | Required | Description |
|-------|----------|-------------|
| `id` | Yes | Reverse-domain unique identifier |
| `version` | Yes | Semantic version string |
| `name` | Yes | Human-readable name |
| `description` | No | Short description |
| `author` | No | Plugin author |
| `capabilities` | Yes | Array of hook point identifiers |
| `dependencies` | No | Map of plugin ID to semver constraint |
| `entry` | Yes | Filename of the shared library |

## Plugin API Reference

### `GsplPluginInfo`

Returned by the plugin's `GsplPluginInitFunc`:

```c
typedef struct {
  int api_version;         // Must be GSPL_PLUGIN_API_VERSION (1)
  const char* plugin_id;   // Must match manifest id
  const char* version;     // Must match manifest version
} GsplPluginInfo;
```

### `GsplPluginCallbacks`

Function pointer table passed to `GsplPluginLoadFunc`:

```c
typedef struct {
  void* reserved;  // Reserved for future use
} GsplPluginCallbacks;
```

### Hook Points

Capability | Hook Function | When Called
-----------|---------------|------------
`on_document_open` | `GsplPluginOnDocumentOpen` | When a document is opened in the editor
`on_document_save` | `GsplPluginOnDocumentSave` | When a document is saved
`on_build_start` | `GsplPluginOnBuildStart` | When a build begins
`on_build_end` | `GsplPluginOnBuildEnd` | When a build completes
`on_preview` | `GsplPluginOnPreview` | When the preview renders a frame
`on_plugin_activate` | `GsplPluginOnActivate` | When the plugin is activated
`on_plugin_deactivate` | `GsplPluginOnDeactivate` | When the plugin is deactivated

### Required Exports

```c
GSPL_PLUGIN_EXPORT GsplPluginInfo* GsplPluginInitFunc(void);
GSPL_PLUGIN_EXPORT int GsplPluginLoadFunc(GsplPluginCallbacks* callbacks);
GSPL_PLUGIN_EXPORT void GsplPluginUnloadFunc(void);
```

## Step-by-Step: Creating a Minimal Plugin

### 1. Create the directory structure

```
my-plugin/
├── manifest.jsonc
└── src/
    └── plugin.c
```

### 2. Write the manifest

```jsonc
{
  "id": "com.example.hello-plugin",
  "version": "0.1.0",
  "name": "Hello Plugin",
  "description": "Logs messages on document open",
  "capabilities": ["on_document_open"],
  "entry": "hello_plugin.dll"
}
```

### 3. Implement the plugin

```c
#include "gspl/plugin/plugin_api.h"
#include <stdio.h>

static GsplPluginInfo info = {
  .api_version = GSPL_PLUGIN_API_VERSION,
  .plugin_id = "com.example.hello-plugin",
  .version = "0.1.0"
};

GSPL_PLUGIN_EXPORT GsplPluginInfo* GsplPluginInitFunc(void) {
  return &info;
}

GSPL_PLUGIN_EXPORT int GsplPluginLoadFunc(GsplPluginCallbacks* callbacks) {
  printf("Hello plugin loaded!\n");
  return 0;  // 0 = success
}

GSPL_PLUGIN_EXPORT void GsplPluginUnloadFunc(void) {
  printf("Hello plugin unloaded!\n");
}

GSPL_PLUGIN_EXPORT void GsplPluginOnDocumentOpen(const char* path) {
  printf("Document opened: %s\n", path);
}
```

### 4. Build as a shared library

```powershell
cl /LD /I path/to/gspl/include src/plugin.c /Fehello_plugin.dll
```

On Linux:

```bash
gcc -shared -fPIC -I path/to/gspl/include src/plugin.c -o hello_plugin.so
```

### 5. Test

Place the plugin directory (with `manifest.jsonc` and the built `.dll`/`.so`) in `workspace/plugins/`. Launch the Studio — the plugin appears in the Plugin Panel. Activate it and observe the lifecycle callbacks.

## Best Practices

- **No global state** — The plugin worker may be restarted; use the init/load functions to initialize state
- **Handle errors gracefully** — Return non-zero from load functions to signal failure; the Plugin Manager logs diagnostics and skips the plugin
- **Respect workspace authority** — Do not modify files outside the workspace; use provided callbacks for UI interaction
- **Minimize work in callbacks** — Long-running operations should be deferred or run in a thread; callbacks are synchronous
- **Version your plugin** — Follow semver; bump major for breaking API changes to your plugin's own interface

## Limitations

- No direct UI access — plugin tool windows must be created through the host-provided tool window callback
- No network access from the plugin worker — all external communication must go through the host's IPC
- Maximum plugin size: 256 MB loaded image
- Maximum manifest size: 64 KB
