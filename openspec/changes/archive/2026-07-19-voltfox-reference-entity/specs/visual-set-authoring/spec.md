## ADDED Requirements

### Requirement: Visual-set manifest loads from text file
The system SHALL load an authored visual set from a UTF-8 text manifest file conforming to the `gspl.visual-set/0.1` schema.

#### Scenario: Load visual-set for voltfox
- **WHEN** the system loads a valid visual-set manifest for the voltfox entity
- **THEN** it SHALL produce an `AuthoredVisualSet` with non-empty frames, semantics, channel_maps, and canonical metadata

#### Scenario: Reject missing schema
- **WHEN** the manifest does not start with `schema=gspl.visual-set/0.1`
- **THEN** `load_authored_visual_set` SHALL throw `std::runtime_error`

### Requirement: Visual-set validates temporal stability
The system SHALL enforce pixel-change and silhouette-IoU limits across animation frame sequences.

#### Scenario: Detect excessive pixel change
- **WHEN** consecutive frames exceed `temporal_max_changed_per_million`
- **THEN** `load_authored_visual_set` SHALL throw `std::runtime_error`

### Requirement: Channel maps validate content semantics
Each channel map SHALL be validated for the correct color space, alpha, and data encoding per its `ChannelMapKind`.

#### Scenario: Validate depth channel
- **WHEN** a `ChannelMapKind::depth` map has an sRGB color space
- **THEN** `validate_channel_map` SHALL produce a diagnostic

#### Scenario: Reject non-opaque channel alpha
- **WHEN** a channel map's pixels contain any non-255 alpha
- **THEN** `validate_channel_map` SHALL produce a diagnostic with code `SPRITE_CHANNEL_ALPHA_INVALID`
