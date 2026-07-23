## ADDED Requirements

### Requirement: Tree view of compiled artifact graph
The artifact explorer SHALL display compiled artifacts in a hierarchical tree view organized by content-addressed key.

#### Scenario: Explore artifact graph
- **WHEN** the user opens the artifact explorer after compiling a sprite
- **THEN** the tree view shows the root artifact with children for each dependency, grouped by content-addressed hash

#### Scenario: Expand artifact children
- **WHEN** the user expands a parent artifact node
- **THEN** the tree displays its immediate child artifacts with their hashes, types, and sizes

### Requirement: Inspect command
The artifact explorer SHALL provide an inspect command that displays full artifact metadata and contents in a detail panel.

#### Scenario: Inspect an artifact
- **WHEN** the user selects an artifact and invokes inspect
- **THEN** the detail panel shows the artifact's hash, size, type, creation time, compiler version, and a hex or formatted preview of its payload

### Requirement: Inspect-diff command
The artifact explorer SHALL support comparing two artifacts by hash and displaying their differences.

#### Scenario: Diff two artifacts
- **WHEN** the user selects two artifacts and invokes inspect-diff
- **THEN** the detail panel shows a side-by-side or unified diff of the payload bytes, highlighting differing regions

### Requirement: Validate command
The artifact explorer SHALL validate an artifact's integrity by recomputing its hash and confirming it matches the content-addressed key.

#### Scenario: Valid artifact passes
- **WHEN** the user selects an uncorrupted artifact and invokes validate
- **THEN** the explorer reports the artifact is valid and displays the recomputed hash

#### Scenario: Corrupted artifact fails validation
- **WHEN** the user selects an artifact whose payload has been tampered with and invokes validate
- **THEN** the explorer reports the mismatch and flags the artifact as corrupt

### Requirement: Manual invalidation
The artifact explorer SHALL allow the user to manually mark one or more artifacts as invalid, triggering recompilation on next use.

#### Scenario: Invalidate a single artifact
- **WHEN** the user right-clicks an artifact and selects invalidate
- **THEN** the artifact is marked stale and the next pipeline build recomputes it

#### Scenario: Invalidate subtree
- **WHEN** the user invalidates a parent artifact
- **THEN** all its descendants are also marked stale

### Requirement: Cache pruning
The artifact explorer SHALL provide a prune command that removes artifacts not accessed within a configurable retention period.

#### Scenario: Prune old artifacts
- **WHEN** the user sets retention to 30 days and invokes prune
- **THEN** all artifacts with last-access time older than 30 days are removed from the cache

#### Scenario: Dry-run prune
- **WHEN** the user invokes prune with the --dry-run flag
- **THEN** the explorer lists artifacts that would be removed without actually deleting them

### Requirement: Integrity verification
The artifact explorer SHALL verify every artifact in the cache on startup, reporting any corrupted entries.

#### Scenario: Start-up integrity check
- **WHEN** the explorer loads and finds a cached artifact whose stored hash does not match its content
- **THEN** the artifact is flagged with a warning icon and the user is prompted to recompile

### Requirement: Artifact metadata display
The artifact explorer SHALL display size in bytes, content hash in hex, creation timestamp, compiler version, and artifact type for every artifact.

#### Scenario: Metadata panel
- **WHEN** the user hovers over any artifact node in the tree
- **THEN** a tooltip shows size, hash, creation time, compiler version, and type

### Requirement: Search by hash
The artifact explorer SHALL support searching the artifact cache by full or partial content hash.

#### Scenario: Search by partial hash
- **WHEN** the user types `a3f2` into the search bar
- **THEN** the tree filters to show only artifacts whose hash contains `a3f2`
