## ADDED Requirements

### Requirement: Window management
The studio shell SHALL support a multiple-document interface (MDI) with dockable, resizable, and closeable child windows.

#### Scenario: Open a new editor window via menu
- **WHEN** the user selects File > New from the main menu
- **THEN** a new editor window SHALL open as a child pane within the MDI area

#### Scenario: Dock a tool window to a side panel
- **WHEN** the user drags the Project Explorer tool window header to the left edge of the main frame
- **THEN** the tool window SHALL dock into a left-side panel and SHALL display a splitter handle for resize

### Requirement: Main menu bar
The studio shell SHALL display a main menu bar with File, Edit, View, Project, Build, Tools, Window, and Help menus.

#### Scenario: Menu items invoke commands
- **WHEN** the user clicks Help > About
- **THEN** the About dialog SHALL open as a modal window

#### Scenario: Keyboard accelerators
- **WHEN** the user presses Ctrl+S
- **THEN** the currently active document SHALL be saved

### Requirement: Tool windows
The studio shell SHALL support detachable, dockable, and hideable tool windows including Project Explorer, Outline, Properties, Output, and Error List.

#### Scenario: Show or hide a tool window
- **WHEN** the user toggles View > Error List
- **THEN** the Error List tool window SHALL become visible if hidden or hidden if visible

#### Scenario: Auto-hide a docked tool window
- **WHEN** the user clicks the pin icon on a docked tool window
- **THEN** the tool window SHALL auto-hide to a tab on the edge of the main frame

### Requirement: Status bar
The studio shell SHALL display a status bar at the bottom of the main window showing cursor position, project state, build status, and active configuration.

#### Scenario: Update after cursor movement
- **WHEN** the user moves the text cursor in an editor
- **THEN** the status bar SHALL update the line and column numbers within 100 ms

#### Scenario: Build progress indicator
- **WHEN** a build is in progress
- **THEN** the status bar SHALL display a progress animation and the current build target name

### Requirement: Command palette
The studio shell SHALL provide a command palette accessible via Ctrl+Shift+P that lists all available commands with fuzzy-search filtering.

#### Scenario: Execute command from palette
- **WHEN** the user types "save all" in the command palette search box and presses Enter on the matching result
- **THEN** all open unsaved documents SHALL be saved

#### Scenario: Command palette shows keyboard shortcuts
- **WHEN** the command palette displays a list of matching commands
- **THEN** each entry SHALL show its associated keyboard shortcut if one is bound

### Requirement: Project open and close
The studio shell SHALL support opening existing GSPL projects from a `.gsplproj` file and closing the currently open project.

#### Scenario: Open project from file dialog
- **WHEN** the user selects File > Open Project and chooses a `.gsplproj` file
- **THEN** the studio SHALL load the project, populate the Project Explorer, and restore the last-saved workspace layout

#### Scenario: Close project
- **WHEN** the user selects File > Close Project
- **THEN** the studio SHALL prompt to save unsaved documents and then unload the project and clear the Project Explorer

### Requirement: Workspace management
The studio shell SHALL persist workspace layout state including open document tabs, tool window positions, and window size across sessions.

#### Scenario: Restore layout on restart
- **WHEN** the studio is restarted and a project is opened
- **THEN** the docked tool windows and document tabs SHALL be restored to their positions from the previous session

#### Scenario: Reset workspace to defaults
- **WHEN** the user selects Window > Reset Workspace Layout
- **THEN** all tool windows SHALL return to their default positions and sizes

### Requirement: Document management
The studio shell SHALL manage open documents with tabbed browsing, dirty-file tracking, and close confirmation for unsaved changes.

#### Scenario: Dirty file indicator
- **WHEN** a document has unsaved changes
- **THEN** an asterisk SHALL appear next to the file name in the document tab

#### Scenario: Close with unsaved changes
- **WHEN** the user closes a document tab that has unsaved changes
- **THEN** a confirmation dialog SHALL appear with options Save, Discard, and Cancel

### Requirement: Recent files
The studio shell SHALL maintain a list of recently opened projects and files, accessible from the File menu.

#### Scenario: Open recent file
- **WHEN** the user hovers over File > Recent Files
- **THEN** a submenu SHALL list up to the 10 most recently opened files

#### Scenario: Recent files persist across sessions
- **WHEN** the studio is restarted
- **THEN** the recent files list SHALL be preserved from the prior session

### Requirement: Startup wizard
The studio shell SHALL display a startup wizard dialog on first launch that offers options to create a new project, open an existing project, or browse examples.

#### Scenario: New project from startup wizard
- **WHEN** the user clicks "New Project" on the startup wizard
- **THEN** the new-project dialog SHALL open

#### Scenario: Dismiss startup wizard
- **WHEN** the user checks "Don't show again" and closes the wizard
- **THEN** the startup wizard SHALL not appear on subsequent launches

### Requirement: Preferences dialog
The studio shell SHALL provide a preferences dialog with categorized settings (editor, build, appearance, language) that are persisted to disk.

#### Scenario: Change editor font size
- **WHEN** the user opens Preferences, navigates to Editor > Font, and changes the font size to 14
- **THEN** all open editors SHALL immediately render text at 14pt and the setting SHALL be persisted

#### Scenario: Reset category to defaults
- **WHEN** the user clicks "Reset" on the Editor category in Preferences
- **THEN** all settings under that category SHALL revert to their default values

### Requirement: About dialog
The studio shell SHALL display an About dialog showing application name, version, build date, copyright, and license information.

#### Scenario: Open About dialog
- **WHEN** the user selects Help > About
- **THEN** a modal dialog SHALL appear displaying "GSPL Studio", the version string, build date, copyright notice, and a link to the license

#### Scenario: Copy version info
- **WHEN** the user clicks "Copy Info" in the About dialog
- **THEN** the version string and build date SHALL be copied to the clipboard
