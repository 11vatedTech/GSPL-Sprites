## ADDED Requirements

### Requirement: Preview executable SHALL load a portable sprite package
A `preview` executable (built when `BUILD_PREVIEW=ON`) SHALL accept a path to a GSPL sprite package via command-line argument or hardcoded test path and load it using `load_package()`.

#### Scenario: Preview loads package without error
- **WHEN** launching preview with a path to a valid Voltfox package
- **THEN** the package loads without errors and the window opens

### Requirement: Preview SHALL render the current manifestation
The preview SHALL display the sprite's current visual manifestation in an SDL2 window. It SHALL support cycling between 2D, 2.5D, and 3D projections via keyboard input. 2D SHALL display the current frame sprite. 2.5D SHALL composite the depth planes with parallax. 3D SHALL display a wireframe or flat-shaded mesh.

#### Scenario: 2D rendering shows sprites
- **WHEN** preview is in 2D mode
- **THEN** the window displays pixel art sprites

#### Scenario: Mode switch with keyboard
- **WHEN** pressing the '2' key
- **THEN** preview switches to 2D rendering mode

### Requirement: Preview SHALL drive the living runtime
The preview SHALL tick the living runtime at a fixed rate (16ms per tick), processing perception, utility AI updates, and transformation triggers. The current form SHALL be displayed as HUD text.

#### Scenario: Runtime ticks advance form
- **WHEN** the preview runs for 100 ticks with a combat scenario fed in
- **THEN** the form may have changed based on utility AI decisions

### Requirement: Preview SHALL display debug overlay
The preview SHALL show an optional debug overlay (toggled by 'D' key) displaying: current form, HP, active statuses, current animation, tick count, and FPS.

#### Scenario: Debug overlay toggles
- **WHEN** pressing the 'D' key in preview
- **THEN** overlay text appears/disappears
