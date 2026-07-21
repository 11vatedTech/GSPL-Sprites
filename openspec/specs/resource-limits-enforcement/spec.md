# resource-limits-enforcement

## Purpose

Every pipeline stage enforces bounded resource consumption through a unified ResourceLimits struct, with clear diagnostics when limits are exceeded.

## Requirements

### Requirement: Source-level resource limits
Before parsing a seed, the system SHALL check source_bytes against ResourceLimits.max_source_bytes, token_count against max_token_count, and token_length against max_token_length.

#### Scenario: Oversized seed source
- **WHEN** a seed source larger than max_source_bytes is submitted
- **THEN** parse_seed SHALL throw RESOURCE_SOURCE_BYTES exceeded

#### Scenario: Excessive tokens
- **WHEN** a seed source produces more tokens than max_token_count
- **THEN** parse_seed SHALL throw RESOURCE_TOKEN_COUNT exceeded

### Requirement: Seed-level resource limits
After parsing, the system SHALL check the SpriteSeed against ResourceLimits for max_forms (16), max_transformations (32), max_bones (64), max_sockets (32), max_clips (32), max_string_length, max_gene_count, max_constraint_count, max_nesting_depth.

#### Scenario: Over-limit forms rejected
- **WHEN** validate encounters a seed with forms > max_forms
- **THEN** the diagnostic SHALL include the limit name, current count, and maximum

### Requirement: Synthesis-level resource limits
After synthesis, the system SHALL check the SynthesisResult against ResourceLimits for max_frames (1024), max_frame_width (2048), max_frame_height (2048), max_25d_planes (32), max_vertices (65535).

#### Scenario: Over-limit frames rejected
- **WHEN** a synthesis produces more frames than max_frames
- **THEN** synthesize_unified_entity SHALL throw RESOURCE_FRAMES

### Requirement: Package-level resource limits
Before atomic rename to the final output path, the system SHALL check total package artifact bytes against ResourceLimits.max_package_bytes (256 MB).

#### Scenario: Oversized package rejected
- **WHEN** a package exceeds 256 MB in staging
- **THEN** build_package SHALL throw RESOURCE_PACKAGE_SIZE before rename
