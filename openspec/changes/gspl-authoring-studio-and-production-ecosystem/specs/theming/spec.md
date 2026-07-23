## ADDED Requirements

### Requirement: Built-in themes
The studio SHALL provide light, dark, and high-contrast themes as built-in presets.

#### Scenario: Light theme applied
- **WHEN** the user selects the light theme
- **THEN** all UI surfaces SHALL render with light background and dark text per the theme palette

#### Scenario: Dark theme applied
- **WHEN** the user selects the dark theme
- **THEN** all UI surfaces SHALL render with dark background and light text per the theme palette

#### Scenario: High-contrast theme applied
- **WHEN** the user selects the high-contrast theme
- **THEN** all UI elements SHALL use maximum-contrast color pairs and visible focus indicators

### Requirement: User-customizable colors
The user SHALL be able to customize individual color tokens in the active theme.

#### Scenario: Color token overridden
- **WHEN** the user changes a color token (e.g., `editor.background`)
- **THEN** the UI SHALL immediately reflect the new color on all matching surfaces

#### Scenario: Invalid color value rejected
- **WHEN** the user enters an invalid color value (e.g., not a valid hex or named color)
- **THEN** the system SHALL reject the value and show a validation error

### Requirement: Font selection
The studio SHALL allow per-theme configuration of editor and UI font families and sizes.

#### Scenario: Font changed
- **WHEN** the user selects a different font family for the editor
- **THEN** the editor SHALL re-render using the selected font

#### Scenario: Fallback font used
- **WHEN** the selected font is not installed on the system
- **THEN** the studio SHALL fall back to a system monospace font and log a warning

### Requirement: Theme file format
Themes SHALL be serializable to a JSON file that can be shared and imported.

#### Scenario: Theme exported to file
- **WHEN** the user exports the current theme
- **THEN** a JSON file SHALL be written containing all color tokens, fonts, and metadata

#### Scenario: Theme imported from file
- **WHEN** the user imports a theme file
- **THEN** the studio SHALL validate and apply the theme, or reject with diagnostics

#### Scenario: Corrupted theme file rejected
- **WHEN** the imported file is not valid JSON or missing required fields
- **THEN** the studio SHALL reject the import with a descriptive error

### Requirement: Accent color
The studio SHALL support a configurable accent color applied to highlights, selection, and interactive elements.

#### Scenario: Accent color changed
- **WHEN** the user sets an accent color
- **THEN** all accent-colored elements SHALL update to the new color immediately

#### Scenario: Accent contrast validation
- **WHEN** the chosen accent color does not meet WCAG AA contrast ratio against the background
- **THEN** the studio SHALL display a contrast warning

### Requirement: Per-window theme override
The user SHALL be able to set a different theme for individual windows or panels.

#### Scenario: Window override applied
- **WHEN** the user sets a dark theme override on a specific editor window
- **THEN** that window SHALL render with the dark theme while other windows retain their theme

#### Scenario: Override cleared
- **WHEN** the user clears the override on a window
- **THEN** that window SHALL revert to the global theme

### Requirement: OS dark mode detection
The studio SHALL detect the operating system's dark mode preference and default to a matching theme on first launch.

#### Scenario: OS dark mode matches
- **WHEN** the OS reports dark mode as preferred
- **THEN** the studio SHALL default to the dark theme on first launch

#### Scenario: OS light mode matches
- **WHEN** the OS reports light mode as preferred
- **THEN** the studio SHALL default to the light theme on first launch

### Requirement: Screen reader contrast support
The high-contrast theme SHALL be designed to work with screen reader focus tracking.

#### Scenario: Focus indicator visible
- **WHEN** a UI element receives keyboard focus in high-contrast mode
- **THEN** a visible focus indicator SHALL be rendered with at least 3:1 contrast against the adjacent color

#### Scenario: Screen reader role announced
- **WHEN** focus changes in any theme
- **THEN** the accessibility API SHALL announce the element's role and state
