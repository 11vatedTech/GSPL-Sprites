## ADDED Requirements

### Requirement: Synthesis generates 2D projection from palette and rig
The system SHALL synthesize a `Projection2dDefinition` from an entity ID, form ID, palette, and rig.

#### Scenario: Synthesize voltfox base form 2D
- **WHEN** `synthesize_projection2d("original.voltfox", "base", palette, rig)` is called
- **THEN** the result SHALL have `id = "original.voltfox.base.2d"`, non-empty `source_frames`, and non-empty `animations`

### Requirement: Synthesis generates 2.5D projection
The system SHALL synthesize a `Projection25dDefinition` with billboard mode and view planes.

#### Scenario: Synthesize voltfox storm form 2.5D
- **WHEN** `synthesize_projection25d("original.voltfox", "storm", palette, rig)` is called
- **THEN** the result SHALL have `billboard = BillboardMode::camera_facing` and at least one plane

### Requirement: Synthesis generates 3D projection
The system SHALL synthesize a `Projection3dDefinition` with materials, meshes, and optional skeleton.

#### Scenario: Synthesize voltfox base form 3D
- **WHEN** `synthesize_projection3d("original.voltfox", "base", palette, rig)` is called
- **THEN** the result SHALL have at least one material and one mesh

### Requirement: Synthesis generates manifestation bindings
The system SHALL generate `TransformationManifestationProgram` bindings for 2D, 2.5D, and 3D.

#### Scenario: Generate 3D manifestation for voltfox
- **WHEN** `make_manifestation3d("original.voltfox", rig)` is called
- **THEN** the result SHALL have `forms` containing both "base" and "storm" form bindings

### Requirement: Unified synthesis produces all representations
The system SHALL produce all six projections (2D base, 2D storm, 2.5D base, 2.5D storm, 3D base, 3D storm) plus three manifestation programs from one call.

#### Scenario: Full entity synthesis
- **WHEN** `synthesize_unified_entity("original.voltfox", base_palette, storm_palette)` is called
- **THEN** the result SHALL contain all six projections and three manifestation programs with non-empty IDs
