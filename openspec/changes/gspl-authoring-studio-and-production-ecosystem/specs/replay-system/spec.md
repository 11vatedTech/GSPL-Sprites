## ADDED Requirements

### Requirement: Deterministic replay from traces
The replay system SHALL reproduce sprite generation identically from a captured trace given the same compiler version and trace file.

#### Scenario: Replay produces identical output
- **WHEN** the user replays a trace file twice using the same compiler build
- **THEN** both replays produce byte-identical final entity state and rendered assets

#### Scenario: Replay with different compiler version warns
- **WHEN** the user replays a trace captured with compiler version 1.2.0 using compiler version 1.3.0
- **THEN** the system emits a version mismatch warning but proceeds if the trace format is compatible

### Requirement: Frame-by-frame stepping
The replay system SHALL allow the user to advance through a trace one step at a time, forward and backward.

#### Scenario: Step forward through trace
- **WHEN** the user opens a trace and presses step-forward
- **THEN** the system advances one evaluation step and displays the entity state at that point

#### Scenario: Step backward
- **WHEN** the user presses step-backward after advancing three steps
- **THEN** the system restores the entity state from the second step

### Requirement: Full state exposure at each step
The replay system SHALL expose the complete semantic entity state, entropy values, gene evaluation order, and diagnostics at every captured step.

#### Scenario: Inspect full state at a step
- **WHEN** the user selects step index 7 in the replay timeline
- **THEN** the inspector shows the entity fields, per-gene entropy stack, the ordered list of evaluated genes, and any warnings or errors emitted during that step

### Requirement: Configurable capture granularity
The replay system SHALL support three capture granularity levels: full (every instruction), seed-only (RNG seeds per step), and gene-level (state after each gene evaluation).

#### Scenario: Full granularity captures every step
- **WHEN** granularity is set to `full` and a sprite compiles with 350 steps
- **THEN** the trace contains 350 entries each with complete entity state

#### Scenario: Seed-only granularity
- **WHEN** granularity is set to `seed-only`
- **THEN** the trace stores only the RNG seed per step and replays by re-evaluating from seeds

#### Scenario: Gene-level granularity
- **WHEN** granularity is set to `gene-level` and a sprite evaluates 5 genes across 3 steps
- **THEN** the trace contains 3 entries, one after each top-level gene completes

### Requirement: Trace comparison for non-determinism detection
The replay system SHALL compare two traces and report any divergences in evaluation order, entropy values, or final state.

#### Scenario: Compare matching traces passes
- **WHEN** two traces from the same sprite and seed are compared
- **THEN** the comparison reports all steps match

#### Scenario: Compare divergent traces
- **WHEN** traces differ at step 12 due to a non-deterministic entropy draw
- **THEN** the comparison report highlights step 12, showing the differing entropy values and the resulting field differences

### Requirement: Trace metadata
Every trace file SHALL contain metadata including the GSPL compiler version, sprite source hash, initial seed, capture timestamp, granularity level, and total step count.

#### Scenario: Trace metadata is accessible
- **WHEN** the user inspects a trace file header
- **THEN** the metadata displays compiler version, source hash, seed, timestamp, granularity, and step count

### Requirement: Trace file format
Traces SHALL be stored in a self-describing binary format with a magic number, version field, and CRC32 integrity checksum.

#### Scenario: Corrupted trace is rejected
- **WHEN** the user attempts to load a trace file whose CRC32 does not match its content
- **THEN** the system rejects the file and displays a corruption diagnostic

### Requirement: Replay from partial traces
The replay system SHALL replay partial traces that were truncated mid-capture, replaying up to the last valid entry.

#### Scenario: Truncated trace replay
- **WHEN** the user loads a trace that ends abruptly at step 22
- **THEN** the system replays steps 0 through 22 and indicates the trace is incomplete
