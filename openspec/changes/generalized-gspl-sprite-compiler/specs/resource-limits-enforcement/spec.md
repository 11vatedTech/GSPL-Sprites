## ADDED Requirements

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
