## ADDED Requirements

### Requirement: Gene editor – typed contracts
The gene editor SHALL allow users to define typed gene contracts including input parameters, output types, and constraint expressions.

#### Scenario: Define a gene contract
- **WHEN** the user creates a new gene node and sets its input type to `Sprite` and output type to `Sprite`
- **THEN** the gene contract SHALL display typed input and output ports on the node

#### Scenario: Constraint expression editing
- **WHEN** the user edits the constraint field on a gene contract
- **THEN** the editor SHALL validate the constraint expression and highlight syntax errors inline

### Requirement: Gene editor – dependency management
The gene editor SHALL display gene dependency graphs and support adding, removing, and reordering gene dependencies.

#### Scenario: Add a dependency edge
- **WHEN** the user drags from the output port of one gene to the input port of another
- **THEN** a directed edge SHALL be created and the dependency graph SHALL be updated

#### Scenario: Remove a dependency
- **WHEN** the user selects a dependency edge and presses Delete
- **THEN** the edge SHALL be removed and the dependency graph SHALL be revalidated

### Requirement: Gene editor – conflict resolution
The gene editor SHALL detect dependency conflicts (cycles, duplicate edges, type mismatches) and suggest resolutions.

#### Scenario: Detect cycle
- **WHEN** adding a dependency edge would create a cycle in the graph
- **THEN** the editor SHALL display a conflict diagnostic and refuse to create the edge

#### Scenario: Type mismatch on connection
- **WHEN** the user connects an output port of type `Int` to an input port of type `Sprite`
- **THEN** the editor SHALL highlight the connection in red and display a type-mismatch diagnostic

### Requirement: Morphology editor – limb and segment layout
The morphology editor SHALL provide a visual canvas for arranging limbs and segments with drag-and-drop positioning, rotation, and scaling.

#### Scenario: Add a limb
- **WHEN** the user drags a "Limb" element from the palette onto the canvas
- **THEN** a new limb SHALL appear at the drop position with default dimensions and a selectable label

#### Scenario: Resize a segment
- **WHEN** the user drags a corner handle of a selected segment
- **THEN** the segment's width and height SHALL update in real time and the property panel SHALL reflect the new values

### Requirement: Morphology editor – joint configuration
The morphology editor SHALL support configuring joint types (pivot, hinge, ball) with angle limits and attachment points.

#### Scenario: Change joint type
- **WHEN** the user selects a joint and changes its type from "hinge" to "ball" in the property panel
- **THEN** the joint visual SHALL update and the angle limit fields SHALL change to reflect ball-joint parameters

#### Scenario: Set joint angle limit
- **WHEN** the user sets the minimum and maximum angle on a hinge joint
- **THEN** the joint SHALL display an arc visualization indicating the allowed range

### Requirement: Morphology editor – attachment points
The morphology editor SHALL allow defining attachment points on limbs and segments for connecting child elements or equipment.

#### Scenario: Add attachment point
- **WHEN** the user right-clicks a segment and selects "Add Attachment Point"
- **THEN** a draggable attachment marker SHALL appear on the segment surface

#### Scenario: Snap attachment to child
- **WHEN** the user drags a child limb's root near a parent attachment point
- **THEN** the child limb SHALL snap to align with the attachment point

### Requirement: Form editor – color palette
The form editor SHALL provide a color palette with swatches, a color picker, and support for named colors and hex/RGB/HSL input.

#### Scenario: Pick a color
- **WHEN** the user selects a region and clicks a color swatch
- **THEN** the region's fill color SHALL be set to the selected color and the preview SHALL update

#### Scenario: Enter hex value
- **WHEN** the user types "#FF8800" in the hex input field
- **THEN** the color swatch SHALL update and the RGB and HSL fields SHALL reflect the entered value

### Requirement: Form editor – gradients and patterns
The form editor SHALL support linear and radial gradients, tiled patterns, and material assignments with real-time preview.

#### Scenario: Create linear gradient
- **WHEN** the user selects Gradient mode and sets two color stops
- **THEN** the form fill SHALL render as a linear gradient between the stops

#### Scenario: Apply tiled pattern
- **WHEN** the user selects a pattern asset and sets tile mode to "repeat"
- **THEN** the form SHALL fill with the repeating pattern

### Requirement: Form editor – material, emissive, scale, opacity
The form editor SHALL allow setting material properties, emissive color, scale, and opacity on any form region.

#### Scenario: Adjust opacity slider
- **WHEN** the user moves the opacity slider to 50%
- **THEN** the region SHALL render at 50% opacity in the preview

#### Scenario: Set emissive color
- **WHEN** the user sets an emissive color on a region
- **THEN** the region SHALL glow with that color in the preview when the "emissive" layer is enabled

### Requirement: Animation editor – timeline keyframes
The animation editor SHALL provide a timeline with keyframe tracks for each animatable property and support add, move, delete, and interpolate keyframes.

#### Scenario: Add keyframe
- **WHEN** the user clicks the "Add Keyframe" button on a property track at frame 12
- **THEN** a keyframe SHALL be inserted at frame 12 with the current property value

#### Scenario: Move keyframe
- **WHEN** the user drags an existing keyframe from frame 12 to frame 24
- **THEN** the keyframe SHALL be repositioned and the interpolation curve SHALL update

### Requirement: Animation editor – state machine graph
The animation editor SHALL support creating animation state machines with states, transitions, triggers, and blend parameters.

#### Scenario: Add a state
- **WHEN** the user drags a "State" node onto the state machine canvas
- **THEN** a new animation state SHALL be created with a configurable name and default animation clip

#### Scenario: Create a transition
- **WHEN** the user draws a connection from one state to another and sets a trigger condition
- **THEN** a directed transition edge SHALL appear with the trigger label displayed

### Requirement: Animation editor – blending curves
The animation editor SHALL provide a curve editor for defining blend weights, interpolation curves (linear, ease-in, ease-out, custom), and crossfade durations.

#### Scenario: Set blend curve to ease-out
- **WHEN** the user selects a transition and changes its blend curve to "ease-out"
- **THEN** the curve preview SHALL update and the transition SHALL use an ease-out interpolation

#### Scenario: Custom curve editing
- **WHEN** the user drags control points on a custom Bezier curve
- **THEN** the curve SHALL update and the animation blend SHALL follow the modified curve

### Requirement: Behavior editor – condition/action rules
The behavior editor SHALL support defining behavior rules as condition/action pairs with a visual rule builder.

#### Scenario: Add a rule
- **WHEN** the user clicks "Add Rule" and sets a condition "health < 25" and an action "flee()"
- **THEN** a new rule SHALL appear in the rule list with the condition and action displayed

#### Scenario: Rule evaluation order
- **WHEN** the user reorders rules by dragging them in the rule list
- **THEN** the priority order SHALL update and the status bar SHALL show the new evaluation sequence

### Requirement: Behavior editor – priority scheduling
The behavior editor SHALL support assigning numeric priority levels to rules and scheduling the highest-priority matching rule for execution.

#### Scenario: Set rule priority
- **WHEN** the user sets Rule A priority to 100 and Rule B priority to 50
- **THEN** if both rules match, Rule A SHALL execute first due to higher priority

#### Scenario: Priority conflict warning
- **WHEN** two rules have identical priority and overlapping conditions
- **THEN** the editor SHALL display a warning for potential non-deterministic scheduling

### Requirement: Behavior editor – stimulus mapping
The behavior editor SHALL support mapping external stimuli (events, messages, sensory input) to behavior rule triggers.

#### Scenario: Map stimulus to condition
- **WHEN** the user selects a rule's trigger and chooses stimulus "OnDamaged"
- **THEN** the rule SHALL activate only when the "OnDamaged" stimulus is received

#### Scenario: Stimulus parameter binding
- **WHEN** the user binds a stimulus parameter "damageAmount" to a rule condition variable
- **THEN** the condition SHALL use the stimulus parameter value at runtime

### Requirement: Combat editor – ability trees
The combat editor SHALL support creating hierarchical ability trees with unlock prerequisites, cooldown groups, and resource costs.

#### Scenario: Add ability to tree
- **WHEN** the user adds an ability "Fireball" to the ability tree as a child of "BasicMagic"
- **THEN** "Fireball" SHALL appear as a child node with an unlock prerequisite arrow from "BasicMagic"

#### Scenario: Set resource cost
- **WHEN** the user sets the mana cost of an ability to 50
- **THEN** the ability node SHALL display "50 MP" as a cost annotation

### Requirement: Combat editor – cooldown curves
The combat editor SHALL provide cooldown curve editing with level-based scaling, shared cooldown groups, and reduction modifiers.

#### Scenario: Define cooldown curve
- **WHEN** the user sets base cooldown to 10 seconds and a per-level reduction of 0.5 seconds
- **THEN** the curve preview SHALL show cooldown decreasing from 10s at level 1 to 5s at level 11

#### Scenario: Shared cooldown group
- **WHEN** the user assigns two abilities to the same cooldown group "HeavyAttacks"
- **THEN** activating one ability SHALL trigger cooldown on both abilities in the group

### Requirement: Combat editor – damage and hitbox configuration
The combat editor SHALL support configuring damage values, damage types, hitbox shapes, and hitbox attach points per animation frame.

#### Scenario: Set hitbox shape
- **WHEN** the user selects a hitbox and changes its shape from "rectangle" to "circle"
- **THEN** the hitbox visual SHALL update and the radius field SHALL replace width/height fields

#### Scenario: Animate hitbox position
- **WHEN** the user moves a hitbox at keyframe 0 and moves it again at keyframe 10
- **THEN** the hitbox SHALL interpolate its position between the two keyframes during playback

### Requirement: Combat editor – targeting rules
The combat editor SHALL support defining targeting rules including range, line-of-sight, faction filtering, and priority formulas.

#### Scenario: Set targeting range
- **WHEN** the user sets targeting range to 150 units and filters by faction "Enemy"
- **THEN** the ability SHALL only acquire targets within 150 units that belong to the Enemy faction

#### Scenario: Priority formula
- **WHEN** the user enters a priority formula "max(1, 10 - distance / 15)"
- **THEN** the targeting system SHALL evaluate the formula for each candidate and select the highest-priority target
