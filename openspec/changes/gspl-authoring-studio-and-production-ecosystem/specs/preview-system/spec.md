## ADDED Requirements

### Requirement: Cross-representation preview
The preview system SHALL render a GSPL entity in three representations: canonical (structured/abstract), sprite sheet (rendered frames), and runtime simulation (animated behavior).

#### Scenario: Canonical representation display
- **WHEN** the user selects the "Canonical" viewport
- **THEN** the preview SHALL display the entity's morphology skeleton with labeled limbs, joints, and attachment points in a wireframe overlay

#### Scenario: Sprite sheet representation display
- **WHEN** the user selects the "Sprite Sheet" viewport
- **THEN** the preview SHALL display the fully rendered sprite frames assembled from the entity's form, morphology, and animation data

#### Scenario: Runtime simulation display
- **WHEN** the user selects the "Runtime" viewport and presses Play
- **THEN** the preview SHALL simulate the entity's behavior rules and animation state machine, rendering the resulting animation in real time

### Requirement: Real-time property adjustment
The preview system SHALL reflect property edits (form color, limb position, animation speed) in all viewports within 200 ms of the change.

#### Scenario: Color change updates instantly
- **WHEN** the user changes the entity's primary color from red to blue in the form editor
- **THEN** all active viewports SHALL update to show the entity in blue within 200 ms

#### Scenario: Animation speed adjustment
- **WHEN** the user changes the animation playback speed from 1.0x to 2.0x
- **THEN** the runtime viewport SHALL animate at twice the speed

### Requirement: Multi-viewport layout
The preview system SHALL support displaying up to four simultaneous viewports (canonical, sprite sheet, runtime, and an optional custom view).

#### Scenario: Four-viewport grid
- **WHEN** the user selects View > Layout > Four Grid
- **THEN** four viewports SHALL be arranged in a 2x2 grid showing canonical (top-left), sprite sheet (top-right), runtime (bottom-left), and a blank custom viewport (bottom-right)

#### Scenario: Maximize a single viewport
- **WHEN** the user double-clicks a viewport tab
- **THEN** that viewport SHALL expand to fill the entire preview area

### Requirement: Side-by-side comparison
The preview system SHALL allow side-by-side comparison of two entities or two animation states.

#### Scenario: Compare two entities
- **WHEN** the user drags Entity A into the left viewport and Entity B into the right viewport
- **THEN** both viewports SHALL render their respective entities side by side with synchronized playback controls

#### Scenario: Compare animation states
- **WHEN** the user selects "Idle" on the left viewport and "Walk" on the right viewport
- **THEN** the left viewport SHALL loop the Idle animation and the right viewport SHALL loop the Walk animation

### Requirement: Provider-side preview
The preview system SHALL support rendering via provider plugins (e.g. local rasterizer, external game engine) as a preview back-end.

#### Scenario: Switch preview provider
- **WHEN** the user selects "ExternalEngine" as the preview provider in settings
- **THEN** the viewports SHALL render using the ExternalEngine provider's rendering pipeline

#### Scenario: Provider capability display
- **WHEN** a provider does not support runtime simulation
- **THEN** the Runtime viewport SHALL display a "Not supported by current provider" message

### Requirement: Auto-refresh on source change
The preview system SHALL detect changes to the GSPL source file and automatically refresh all viewports.

#### Scenario: File save triggers refresh
- **WHEN** the user saves the GSPL source file
- **THEN** all viewports SHALL refresh with the updated entity data within 500 ms

#### Scenario: External file modification
- **WHEN** a GSPL file is modified by an external tool
- **THEN** the preview system SHALL detect the change and prompt the user to refresh or auto-refresh

### Requirement: FPS control
The preview system SHALL allow the user to set the target frames per second (FPS) for runtime simulation and animation playback.

#### Scenario: Set FPS limit
- **WHEN** the user sets the FPS limit to 30
- **THEN** the runtime viewport SHALL render at a maximum of 30 frames per second

#### Scenario: Uncap FPS
- **WHEN** the user sets FPS limit to 0 (uncapped)
- **THEN** the runtime viewport SHALL render as fast as the display refresh rate allows

### Requirement: Playback controls
The preview system SHALL provide VCR-style playback controls (Play, Pause, Stop, Step Forward, Step Backward, Loop) for animation and simulation.

#### Scenario: Step forward advances one frame
- **WHEN** the preview is paused and the user clicks "Step Forward"
- **THEN** the animation SHALL advance by exactly one frame

#### Scenario: Loop toggle
- **WHEN** the user toggles Loop on during playback
- **THEN** the animation SHALL restart from the beginning when it reaches the end

### Requirement: Screenshot capture
The preview system SHALL allow capturing a screenshot of any viewport to the clipboard or to a file.

#### Scenario: Copy viewport to clipboard
- **WHEN** the user right-clicks a viewport and selects "Copy Screenshot"
- **THEN** the current viewport content SHALL be copied to the clipboard as a PNG image

#### Scenario: Save viewport to file
- **WHEN** the user right-clicks a viewport and selects "Save Screenshot As..."
- **THEN** a save-file dialog SHALL open with the default name "entityname_viewport.png"

### Requirement: Overlay debug information
The preview system SHALL display optional overlay information including FPS counter, frame number, memory usage, and entity statistics.

#### Scenario: Show FPS overlay
- **WHEN** the user enables View > Show FPS Overlay
- **THEN** the current FPS SHALL be displayed in the corner of each active viewport

#### Scenario: Show entity stats
- **WHEN** the user enables View > Show Entity Stats
- **THEN** each viewport SHALL overlay the active entity name, frame count, and active animation state

### Requirement: Light/dark preview background
The preview system SHALL allow toggling the preview background between light, dark, and checkerboard patterns.

#### Scenario: Toggle checkerboard background
- **WHEN** the user selects Background > Checkerboard
- **THEN** the viewport background SHALL display a checkerboard pattern to highlight transparency

#### Scenario: Dark background for runtime
- **WHEN** the user selects Background > Dark
- **THEN** all viewports SHALL use a dark gray (#1E1E1E) background
