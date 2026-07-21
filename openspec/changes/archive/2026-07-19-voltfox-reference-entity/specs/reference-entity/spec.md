## ADDED Requirements

### Requirement: Voltfox reference entity builds from seed
The system SHALL successfully build a self-verifying sprite package from `examples/voltfox.sprite` using the seed-only build path.

#### Scenario: Build and verify voltfox from seed
- **WHEN** the system builds and verifies a package from the voltfox seed
- **THEN** the verification SHALL pass with `entity_id = "original.voltfox"` and at least 9 declared artifacts

### Requirement: All three representations are valid
The synthesized projections for the voltfox entity SHALL pass their respective validation functions.

#### Scenario: Validate 2D projection
- **WHEN** the synthesized 2D projection for voltfox is passed to `validate_projection2d`
- **THEN** the result SHALL have `ok() == true`

#### Scenario: Validate 2.5D projection
- **WHEN** the synthesized 2.5D projection is passed to `validate_25d`
- **THEN** the result SHALL have `ok() == true`

#### Scenario: Validate 3D projection
- **WHEN** the synthesized 3D projection is passed to `validate_projection3d`
- **THEN** the result SHALL have `ok() == true`

### Requirement: Manifestation programs validate
The manifestation bindings for the voltfox entity SHALL be valid against their associated transformation and combat programs.

#### Scenario: Validate 3D manifestation
- **WHEN** the 3D manifestation program is validated against the voltfox transformation and combat programs
- **THEN** the result SHALL have `ok() == true`

### Requirement: End-to-end pipeline is deterministic
Two consecutive builds from the same voltfox seed SHALL produce identical package identities.

#### Scenario: Deterministic build
- **WHEN** `build_package` is called twice on the same seed
- **THEN** both resulting package identities SHALL be identical
