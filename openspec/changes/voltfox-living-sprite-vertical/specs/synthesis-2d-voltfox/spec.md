## ADDED Requirements

### Requirement: 2D synthesis SHALL produce SpriteFrame buffers from morphology
`synthesize_projection2d_voltfox()` SHALL read the `MorphologyDefinition` and `SynthesisPalette` from SpriteIr and produce a `SpriteProjection2D` containing per-form `SpriteFrame` pixel buffers (RGBA, 128x128 default resolution).

#### Scenario: Idle form produces recognisable fox shape
- **WHEN** synthesizing the Idle form of Voltfox
- **THEN** the resulting pixel buffer has non-zero alpha in regions corresponding to torso, head, ears, eyes, muzzle, tail, and limbs

### Requirement: 2D synthesis SHALL render each morphology part as a distinct filled shape
Each of the 11 morphology parts SHALL be rendered as a specific 2D primitive:
- Torso: filled ellipse
- Head: filled ellipse
- Ears: filled triangles (pointing up)
- Eyes: small filled circles with color
- Muzzle: filled ellipse
- Tail: filled extended ellipse
- Limbs: filled small ellipses
- Aura: semi-transparent circle behind the sprite

#### Scenario: Each part maps to correct primitive
- **WHEN** rendering each body part independently
- **THEN** the pixel output matches the expected primitive shape

### Requirement: 2D synthesis SHALL support six forms with distinct visuals
Each form (Idle, Running, Jumping, Attack, Special, Hurt) SHALL produce a recognisably different frame:
- Idle: upright, symmetrical, neutral tail position
- Running: slight forward lean, legs spread, tail extended back
- Jumping: legs tucked, tail up, body rotated up
- Attack: one ear flattened, lightning projection, lean forward
- Special: both ears back, mouth open, aura intensified, lightning burst
- Hurt: tilted, one eye closed, tail down

#### Scenario: Idle vs Running frames differ
- **WHEN** comparing the Idle and Running frame pixel buffers
- **THEN** they differ by more than 10% of pixels (at 128x128 resolution)

### Requirement: 2D synthesis SHALL include collision shape data
Each `SpriteFrame` SHALL include a `collision_shape` field with axis-aligned bounding boxes derived from the morphology part positions and sizes for that form.

#### Scenario: Idle collision bounds include torso and head
- **WHEN** computing the collision AABB for the Idle frame
- **THEN** the AABB encloses the torso and head morphology positions

### Requirement: 2D frames SHALL include animation timing metadata
Each `SpriteFrame` SHALL carry an `ability_timing` field indicating frame index, duration (in ticks), and associated ability name (empty for non-ability frames). Attack and Special forms SHALL have an associated ability.

#### Scenario: Attack frame has ability timing
- **WHEN** synthesizing the Attack form
- **THEN** the frame's `ability_timing` has a non-empty ability name matching a defined combat ability
