## ADDED Requirements

### Requirement: Seed parses from key=value format
The system SHALL parse a `.sprite` seed file as line-oriented `key=value` pairs, rejecting duplicate keys and unsupported schema versions.

#### Scenario: Parse valid voltfox seed
- **WHEN** the system reads `examples/voltfox.sprite`
- **THEN** it SHALL produce a `SpriteSeed` with `stable_id = "original.voltfox"`, `rights = RightsClass::ORIGINAL_USER_CREATION`, and `entropy_root = 11072026`

#### Scenario: Reject duplicate field
- **WHEN** a seed file contains two `name=` lines
- **THEN** `parse_seed` SHALL throw `std::runtime_error`

### Requirement: Seed compiles to IR
The system SHALL compile a valid `SpriteSeed` into a `SpriteIr` intermediate representation.

#### Scenario: Compile voltfox seed
- **WHEN** `compile(parse_seed(source))` is called on the voltfox seed
- **THEN** the resulting `SpriteIr` SHALL have `seed_identity` matching the canonical SHA-256 of the canonicalized seed

### Requirement: Seed canonicalizes deterministically
The same `SpriteSeed` SHALL always produce the same canonical form and identity.

#### Scenario: Deterministic identity
- **WHEN** `canonicalize(seed)` is called twice on the same seed
- **THEN** both calls SHALL return identical strings

### Requirement: Seed renders to SVG
The system SHALL render a compiled seed to an SVG string suitable for embedding in a package.

#### Scenario: Render voltfox
- **WHEN** `render_svg(compile(seed))` is called on the voltfox seed
- **THEN** the result SHALL be a non-empty valid SVG string
