# synthesis-3d-voltfox

## Purpose

Generate a 3D mesh with skeleton, skinning, animation clips, and glTF export for the Voltfox sprite, supporting 8 primitive types, 13 bones, and linear-blend-skin animation.

## Requirements

### Requirement: 3D synthesis SHALL produce a mesh with 8 primitives
`synthesize_projection3d_voltfox()` SHALL produce a `SpriteProjection3D` containing a single indexed triangle mesh assembled from 8 primitives mapping to the 11 morphology parts:
- Torso: box
- Head: box
- Left ear, Right ear: triangular prisms (caps)
- Left eye, Right eye: spheres
- Muzzle: cone
- Tail: capsule
- Left front leg, Right front leg: cylinders
- Aura: transparent expanded sphere

#### Scenario: Mesh has at least 8 primitive groups
- **WHEN** examining the 3D mesh primitive groups
- **THEN** there are at least 8 distinct groups (some parts share primitives)

#### Scenario: Mesh has non-zero vertex count
- **WHEN** examining the 3D mesh vertex buffer
- **THEN** vertex count >= 64

### Requirement: 3D synthesis SHALL produce a bone skeleton with 13 bones
The skeleton SHALL have 13 bones in a parent-child hierarchy: root → spine → neck → head → left_ear, right_ear, tail, left_shoulder → left_arm, right_shoulder → right_arm. Each bone SHALL have a bind-pose local transform.

#### Scenario: Skeleton has correct bone count
- **WHEN** creating the Voltfox skeleton
- **THEN** there are exactly 13 bones

#### Scenario: Head bone is child of neck
- **WHEN** examining bone hierarchy
- **THEN** the head bone's parent is the neck bone

### Requirement: 3D synthesis SHALL produce per-vertex skinning weights
Each mesh vertex SHALL have 0 to 4 bone influences with weights summing to 1.0 (within floating-point tolerance). Vertices near bone joints SHALL have weights distributed across adjacent bones.

#### Scenario: Joint vertices have multiple influences
- **WHEN** examining vertices near the neck joint
- **THEN** those vertices have weights on both spine and neck bones

### Requirement: 3D synthesis SHALL produce linear-blend-skin animation clips
The 3D projection SHALL contain at least one `AnimationClip` (IdleAnim) with keyframes for each bone, using translation+rotation+spline channels suitable for linear-blend skinning.

#### Scenario: IdleAnim has non-trivial keyframes
- **WHEN** exporting the IdleAnim clip
- **THEN** it has keyframes (positions or rotations) for at least the root and spine bones

### Requirement: 3D synthesis SHALL export to glTF with skin and animations
The 3D projection SHALL support glTF 2.0 export via `export_to_gltf()` producing a valid glTF file containing: mesh node, skin, skeleton, animation clips, and vertex accessors for POSITION, NORMAL, JOINTS_0, WEIGHTS_0. The output SHALL pass glTF validation (schema + semantic checks).

#### Scenario: Exported glTF validates
- **WHEN** exporting 3D Voltfox to glTF and running glTF validator
- **THEN** no errors or warnings
