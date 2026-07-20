## ADDED Requirements

### Requirement: compile() SHALL produce FormDefinition records
The `compile(SpriteSeed&)` function SHALL convert each `[form.<name>]` section in the seed into a `FormDefinition` struct in `SpriteIr.form_definitions`. Each FormDefinition SHALL contain the form name, a list of valid transformation names, and a reference to the associated morphology.

#### Scenario: Single form compiles to one FormDefinition
- **WHEN** a seed has one `[form.Idle]` section
- **THEN** `compile()` produces `SpriteIr.form_definitions` with one entry whose `name=="Idle"`

#### Scenario: All six Voltfox forms compile
- **WHEN** a seed declares Idle, Running, Jumping, Attack, Special, and Hurt forms
- **THEN** `SpriteIr.form_definitions` has exactly six entries

### Requirement: compile() SHALL produce TransformationDelta records
The `compile()` function SHALL convert each `[transformation.<name>]` section into a `TransformationDelta` struct in `SpriteIr.transformation_deltas`. Each delta SHALL contain from_form, to_form, and trigger conditions.

#### Scenario: Transformation compiles correctly
- **WHEN** a seed has `[transformation.T1]` with `from_form=Idle`, `to_form=Running`
- **THEN** `SpriteIr.transformation_deltas` contains T1 with `from=="Idle"` and `to=="Running"`

### Requirement: compile() SHALL produce MorphologyDefinition records
The `compile()` function SHALL convert `[morphology.<part>]` sections into `MorphologyDefinition` structs containing position (vec3), size (vec3), color (RGBA), and rotation (quaternion) for each of the 11 body parts.

#### Scenario: Morphology compiles with all parts
- **WHEN** a seed has all 11 morphology sections
- **THEN** `SpriteIr.morphology` has 11 entries, each with populated position/size/color/rotation

### Requirement: compile() SHALL produce AnimationIntent records
The `compile()` function SHALL convert the `animation_intents=` field into `AnimationIntent` records mapping behavior state names to animation clip names.

#### Scenario: Animation intents compile
- **WHEN** a seed declares animation intents
- **THEN** `SpriteIr.animation_intents` contains a 1:1 mapping

### Requirement: compile()-time validation SHALL catch dangling transformation references
If a form's `transformations=` field references a transformation name that does not exist, `compile()` SHALL leave a validation error in the seed's error list.

#### Scenario: Dangling transformation reference caught
- **WHEN** `[form.Idle]` has `transformations=NonExistentTransformation` but no corresponding `[transformation.NonExistentTransformation]` exists
- **THEN** `compile()` adds a validation error

### Requirement: compile() SHALL validate form graph connectivity
All six forms SHALL be reachable via transformations from at least one other form. An isolated form with no incoming or outgoing transformations SHALL produce a warning.

#### Scenario: Isolated form warning
- **WHEN** a form Idle exists but no transformation references it and it has no transformations
- **THEN** `compile()` adds a warning about the isolated form
