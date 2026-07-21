## MODIFIED Requirements

### Requirement: Seed SHALL support form declarations
The GSPL seed language SHALL allow declaring named forms for a sprite entity using `[form.<name>]` sections. Each form SHALL be validated against ResourceLimits.max_forms (default 16).

#### Scenario: Form declaration parsed successfully
- **WHEN** a seed file contains `[form.Idle]` with fields `transformations=` and `morphology=`
- **THEN** `parse_seed()` returns a SpriteSeed with a forms collection containing the Idle form

#### Scenario: Form count exceeds limit
- **WHEN** a seed file contains more than 16 form sections
- **THEN** `validate()` returns a diagnostic with code SPRITE_FORMS_EXCEEDED

### Requirement: Seed SHALL support transformation definitions
The GSPL seed language SHALL allow defining transformations between forms using `[transformation.<name>]` sections with `from_form=`, `to_form=`, and `triggers=` fields. Transformations SHALL be validated against ResourceLimits.max_transformations (default 32).

#### Scenario: Transformation count exceeds limit
- **WHEN** a seed file contains more than 32 transformation sections
- **THEN** `validate()` returns a diagnostic with code RESOURCE_TRANSFORMATIONS
