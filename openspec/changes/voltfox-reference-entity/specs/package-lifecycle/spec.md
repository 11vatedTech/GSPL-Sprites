## ADDED Requirements

### Requirement: Package builds from seed
The system SHALL build a complete sprite package at a filesystem path given a `SpriteSeed`.

#### Scenario: Build voltfox package from seed-only
- **WHEN** `build_package(seed, output_path)` is called with the voltfox seed
- **THEN** the output directory SHALL contain `manifest.json`, `seed.canonical.json`, `rights.json`, `provenance.json`, `assets/entity.svg`, `asset-graph.json`, `package-target-report.json`, and `package-target-requirements.json`

### Requirement: Package builds from seed with visual set
The system SHALL build a package that includes 2D raster artifacts when an `AuthoredVisualSet` is provided.

#### Scenario: Build voltfox package with visual set
- **WHEN** `build_package(seed, visual_set, output_path)` is called with a valid voltfox visual set
- **THEN** the output SHALL additionally contain `assets/sprite-atlas.png`, `assets/sprite-alpha-mask.png`, and `atlas.json`

### Requirement: Package self-verifies
The system SHALL verify a built package and report its identity, artifact count, and total bytes.

#### Scenario: Verify voltfox package
- **WHEN** the built voltfox package directory is passed to `verify_package`
- **THEN** the result SHALL have `ok() == true` and non-empty `entity_id`, `seed_identity`, and `package_identity`
