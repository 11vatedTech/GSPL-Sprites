## ADDED Requirements

### Requirement: Declarative manifest
Every plugin SHALL include a manifest file (`plugin.json`) declaring its ID, display name, version, author, capability list, and dependencies.

#### Scenario: Valid manifest loads
- **WHEN** the plugin system encounters a plugin with a well-formed `plugin.json` containing all required fields
- **THEN** the manifest is parsed and the plugin is queued for discovery

#### Scenario: Missing manifest fields
- **WHEN** a `plugin.json` is missing the required `id` field
- **THEN** the plugin system rejects the plugin with a diagnostic specifying the missing field

### Requirement: Plugin lifecycle
The plugin system SHALL support five lifecycle stages: discover, load, activate, deactivate, and unload.

#### Scenario: Full lifecycle execution
- **WHEN** a plugin passes discovery, is loaded, and activated successfully
- **THEN** the plugin moves to the active state and its capabilities become available

#### Scenario: Deactivate and reload
- **WHEN** the user deactivates a plugin and then activates it again
- **THEN** the plugin transitions through deactivate back to activate without a full reload

#### Scenario: Unload removes plugin
- **WHEN** the user unloads a plugin
- **THEN** the plugin is removed from the in-memory registry and its resources are freed

### Requirement: Sandboxed worker process isolation
Each plugin SHALL execute in its own worker process with restricted filesystem and network access.

#### Scenario: Plugin crash is contained
- **WHEN** a plugin in a worker process throws an unhandled exception
- **THEN** only that worker process is terminated, the host application continues running, and the plugin is marked as crashed

#### Scenario: Restricted filesystem access
- **WHEN** a plugin attempts to write outside its designated plugin data directory
- **THEN** the sandbox denies the write and returns an access-denied error to the plugin

### Requirement: Hook points
The plugin system SHALL define four hook point types: editor extension, provider integration, tool window, and menu contribution.

#### Scenario: Editor extension hook
- **WHEN** a plugin registers an editor extension hook and the user opens a sprite file
- **THEN** the editor invokes the plugin's hook and the plugin can modify the editor UI or behavior

#### Scenario: Provider integration hook
- **WHEN** a plugin registers a provider integration hook and the provider pipeline runs
- **THEN** the plugin's hook is called at the integration point and can intercept or modify provider data

#### Scenario: Tool window hook
- **WHEN** a plugin registers a tool window hook
- **THEN** a new dockable window appears in the studio UI titled with the plugin's display name

#### Scenario: Menu contribution hook
- **WHEN** a plugin registers a menu contribution and the user opens the Tools menu
- **THEN** the plugin's menu item appears in the Tools menu and triggers the plugin's action when clicked

### Requirement: Plugin SDK header
The plugin system SHALL provide a stable C header (`gspl_plugin.h`) that defines the plugin API, version macros, and callback signatures.

#### Scenario: Plugin compiles against SDK header
- **WHEN** a third-party developer includes `gspl_plugin.h` and implements the required callbacks
- **THEN** their plugin compiles and links against the GSPL plugin SDK without errors

### Requirement: Manifest validation
The plugin system SHALL validate plugin manifests against a JSON schema and reject malformed or incomplete manifests before loading.

#### Scenario: Schema validation rejects bad types
- **WHEN** a manifest declares `version` as an integer `42` instead of a string `"1.0.0"`
- **THEN** the plugin system rejects the manifest with a schema validation diagnostic

### Requirement: Dependency resolution
The plugin system SHALL resolve plugin dependencies before activation and activate plugins in dependency order.

#### Scenario: Dependency order activation
- **WHEN** plugin B depends on plugin A and both are being activated
- **THEN** plugin A is activated before plugin B

#### Scenario: Missing dependency prevents activation
- **WHEN** plugin B depends on plugin C which is not installed
- **THEN** plugin B's activation fails with a diagnostic listing C as an unsatisfied dependency

### Requirement: Plugin version compatibility
The plugin system SHALL check plugin version compatibility against the host application's SDK version.

#### Scenario: Incompatible SDK version
- **WHEN** a plugin requires SDK version `^2.0` but the host provides SDK version `1.5`
- **THEN** the plugin is rejected at load time with a version mismatch diagnostic

### Requirement: Plugin enumeration
The plugin system SHALL provide an API to enumerate all discovered plugins with their state, version, and capabilities.

#### Scenario: Enumerate active plugins
- **WHEN** the host queries the plugin registry for active plugins
- **THEN** the registry returns a list of all active plugin descriptors including ID, version, and capability flags
