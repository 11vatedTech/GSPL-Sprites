# frame-content-identity

## Purpose

Every FrameSource carries a deterministic, schema-bound content hash for identity verification, distinctness validation, and corruption detection.

## Requirements

### Requirement: Schema-bound frame hash
Every FrameSource SHALL carry a frame_hash computed as SHA-256 of a preimage containing "GSPL_FRAME_V1" + width (4B LE) + height (4B LE) + "RGBA8" + "RGBA" + canonical packed RGBA bytes.

#### Scenario: Identical frames hash identically
- **WHEN** two ImageRgba8 with identical width, height, and RGBA pixels are hashed
- **THEN** their frame hashes SHALL be identical

#### Scenario: Dimension change alters hash
- **WHEN** two ImageRgba8 have identical pixel bytes but different dimensions
- **THEN** their frame hashes SHALL differ

#### Scenario: One-pixel change alters hash
- **WHEN** a single RGBA byte differs between two otherwise identical images
- **THEN** their frame hashes SHALL differ

### Requirement: Frame distinctness validation
After synthesis, the system SHALL validate that no AnimationClip contains frames with duplicate content hashes.

#### Scenario: Duplicate content in clip
- **WHEN** an AnimationClip references two frames with identical pixel hashes
- **THEN** validate_frame_distinctness SHALL emit SPRITE_ANIMATION_FRAME_DUPLICATE_CONTENT

### Requirement: PixelAABB computation
The system SHALL compute the tight axis-aligned bounding box of non-fully-transparent pixels (alpha > 0) for any ImageRgba8.

#### Scenario: Tight bounds
- **WHEN** a 4x4 image has a 2x2 opaque region at (1,1)
- **THEN** compute_pixel_aabb SHALL return min_x=1, min_y=1, max_x=2, max_y=2, empty=false

#### Scenario: Fully transparent
- **WHEN** all pixels have alpha=0
- **THEN** compute_pixel_aabb SHALL return empty=true
