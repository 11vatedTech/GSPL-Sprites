## ADDED Requirements

### Requirement: Seed SHALL support form declarations
The GSPL seed language SHALL allow declaring named forms for a sprite entity using `[form.<name>]` sections.

#### Scenario: Form declaration parsed successfully
- **WHEN** a seed file contains `[form.Idle]` with fields `transformations=` and `morphology=`
- **THEN** `parse_seed()` returns a SpriteSeed with a forms collection containing the Idle form

#### Scenario: Form declaration with invalid name rejected
- **WHEN** a seed file contains `[form. ]` with an empty form name
- **THEN** `validate()` returns a validation error

### Requirement: Seed SHALL support transformation definitions
The GSPL seed language SHALL allow defining transformations between forms using `[transformation.<name>]` sections with `from_form=`, `to_form=`, and `triggers=` fields.

#### Scenario: Transformation parsed successfully
- **WHEN** a seed file contains `[transformation.IdleToRunning]` with `from_form=Idle`, `to_form=Running`
- **THEN** `parse_seed()` returns a SpriteSeed with a transformations collection containing that mapping

#### Scenario: Transformation with missing from_form rejected
- **WHEN** a transformation section omits the `from_form` field
- **THEN** `validate()` returns a validation error

### Requirement: Seed SHALL support morphology declarations
The seed language SHALL allow declaring 11 body part sub-sections under `[morphology.<part>]` for torso, head, left_ear, right_ear, left_eye, right_eye, muzzle, tail, left_front_leg, right_front_leg, aura. Each part SHALL have `position=`, `size=`, `color=`, and `rotation=` fields.

#### Scenario: Full morphology parsed successfully
- **WHEN** a seed file contains all 11 `[morphology.<part>]` sections with valid fields
- **THEN** `parse_seed()` returns a SpriteSeed with a full morphology map

#### Scenario: Missing required morphology field rejected
- **WHEN** a `[morphology.torso]` section omits the `color=` field
- **THEN** `validate()` returns a validation error

### Requirement: Seed SHALL support runtime behavior attributes
The seed language SHALL support a `[runtime]` section with fields: `aggession=`, `curiosity=`, `energy=`, `loyalty=` (integer 0-100) and `animation_intents=` (comma-separated list).

#### Scenario: Runtime section parsed successfully
- **WHEN** a seed file contains `[runtime]` with `aggression=60`, `curiosity=80`
- **THEN** `parse_seed()` returns a SpriteSeed with runtime attributes

### Requirement: Seed SHALL support animation intents
The `[runtime]` section SHALL accept an `animation_intents=` field mapping behavior states to animation clip names.

#### Scenario: Animation intents parsed
- **WHEN** `animation_intents=idle:IdleAnim,running:RunAnim,attacking:AttackAnim`
- **THEN** the parsed SpriteSeed contains a map from behavior state to animation clip
