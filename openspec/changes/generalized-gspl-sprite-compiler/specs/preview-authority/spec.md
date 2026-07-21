## ADDED Requirements

### Requirement: Input routed through transformation state
Every keyboard input in the SDL preview SHALL flow through the authoritative transformation state machine rather than directly modifying display fields.

#### Scenario: Ascend key
- **WHEN** the user presses 'A' while current_form is "Voltfox" and no transformation is active
- **THEN** begin_transformation SHALL be called with transformation_id="ascend"

#### Scenario: Descend key
- **WHEN** the user presses 'S' while current_form is "Storm" and no transformation is active
- **THEN** begin_transformation SHALL be called with transformation_id="descend"

### Requirement: Display driven by authoritative state
The rendering functions SHALL read current_form and active transformation from TransformationState, not from a parallel PreviewState.

#### Scenario: Form display
- **WHEN** tstate.current_form is "Storm"
- **THEN** render_2d SHALL use storm color scheme (0x80, 0x40, 0xFF)

#### Scenario: Ascend animation
- **WHEN** tstate.active is set (transformation in progress)
- **THEN** render_2d SHALL show the flash animation based on progress
