## ADDED Requirements

### Requirement: Arbitrary SpriteSeed compilation
The system SHALL accept any valid SpriteSeed (not just Voltfox) and produce a correct SpriteIR with forms, transformations, clips, states, collisions, and projections.

#### Scenario: Non-Voltfox seed with base form
- **WHEN** a SpriteSeed with arbitrary id, form definitions, and transformation rules is compiled
- **THEN** the SpriteIR SHALL contain the correct form definitions, transformation deltas, and entity identity

#### Scenario: Cross-representation synthesis
- **WHEN** a compiled SpriteIr produces a SynthesisResult
- **THEN** the result SHALL contain non-empty 2D, 2.5D, and 3D representations with frame hashes on every FrameSource

### Requirement: Deterministic output
The pipeline SHALL produce identical output for identical inputs (same seed, same entropy_root, same RNG seed).

#### Scenario: Repeated compilation
- **WHEN** the same seed is compiled twice
- **THEN** the resulting SpriteIR SHALL have identical entity_id, form definitions, and transformation deltas

#### Scenario: Repeated synthesis
- **WHEN** the same SpriteIR is synthesized twice
- **THEN** both SynthesisResults SHALL contain identical frame hashes, placement counts, and projection data

### Requirement: Resource-limited pipeline
Every pipeline stage SHALL enforce ResourceLimits before proceeding. The limits SHALL include source bytes, forms, transformations, bones, sockets, clips, frames, planes, vertices, and package size.

#### Scenario: Over-limit seed rejected
- **WHEN** a seed with 17 forms is submitted with max_forms=16
- **THEN** validation SHALL return a diagnostic with code SPRITE_FORMS_EXCEEDED

#### Scenario: Over-limit package rejected
- **WHEN** a package exceeds 256MB after build_package
- **THEN** build_package SHALL throw RESOURCE_PACKAGE_SIZE exceeded
