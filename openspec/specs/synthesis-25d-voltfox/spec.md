# synthesis-25d-voltfox

## Purpose

Generate 2.5D multi-plane images with depth layers, morphological z-ordering, parallax metadata, and form-specific plane arrangements for the Voltfox sprite.

## Requirements

### Requirement: 2.5D synthesis SHALL generate multi-plane images with depth
`synthesize_projection25d_voltfox()` SHALL produce a `SpriteProjection25D` containing a series of RGBA plane images, one per depth layer, for each form. Each plane SHALL have an associated depth value.

#### Scenario: Idle form produces front/back layers
- **WHEN** synthesizing the 2.5D Idle form
- **THEN** the result has at least two plane images at different depth values

### Requirement: 2.5D planes SHALL separate morphological layers
The 11 morphology parts SHALL be sorted by z-order (back to front): tail → back_limbs → torso → head → ears → eyes → muzzle → front_limbs → aura → lightning_overlay. Each group at a distinct depth produces a separate plane.

#### Scenario: Tail appears in backmost plane
- **WHEN** checking the depth of the tail-rendered plane
- **THEN** its depth value is greater (farther back) than the torso plane

### Requirement: 2.5D synthesis SHALL produce parallax-compatible metadata
Each plane image SHALL include a `parallax_factor` field (float) in the plane metadata for multi-layer parallax scrolling effect.

#### Scenario: Parallax factors increase for nearer planes
- **WHEN** comparing parallax factors of consecutive planes from back to front
- **THEN** each nearer plane has a larger parallax factor than the one behind it

### Requirement: 2.5D synthesis SHALL support all six forms
Each of the six Voltfox forms SHALL produce a distinct 2.5D projection with different plane arrangements reflecting form-specific morphology adjustments.

#### Scenario: Attack form aura is in a unique front plane
- **WHEN** synthesizing the Attack form
- **THEN** an additional aura/lightning plane appears in front of all other planes
