## ADDED Requirements

### Requirement: Version bump on publish
The build SHALL increment the version number according to semver rules before publishing.

#### Scenario: Major version bump
- **WHEN** the manifest specifies a breaking change
- **THEN** the major version SHALL be incremented and minor/patch reset to zero

#### Scenario: Minor version bump
- **WHEN** the manifest specifies a backward-compatible feature addition
- **THEN** the minor version SHALL be incremented and patch reset to zero

### Requirement: Automatic changelog generation
The publishing workflow SHALL generate a changelog entry from commit messages since the last published version.

#### Scenario: Changelog created from commits
- **WHEN** a publishing operation begins
- **THEN** a changelog entry SHALL be compiled from all commits between the previous and current version tag

#### Scenario: Empty changelog warning
- **WHEN** no commit messages exist since the last tag
- **THEN** the workflow SHALL warn the user and abort unless overridden

### Requirement: Artifact signing
Every published artifact SHALL be signed with a configured code-signing certificate.

#### Scenario: Valid signature applied
- **WHEN** publishing completes successfully
- **THEN** each artifact SHALL carry an Authenticode or GPG signature verifiable by the consumer

#### Scenario: Missing certificate failure
- **WHEN** no signing certificate is configured
- **THEN** the publish operation SHALL fail with a diagnostic error

### Requirement: Upload channels
The workflow SHALL support stable, beta, and nightly release channels with independent version tracks.

#### Scenario: Stable channel isolation
- **WHEN** publishing to the stable channel
- **THEN** only releases with a non-prerelease semver tag SHALL be accepted

#### Scenario: Nightly auto-tagging
- **WHEN** publishing to the nightly channel
- **THEN** the build SHALL append a prerelease suffix with the current UTC date

### Requirement: Target resolution
The publisher SHALL resolve publish targets to local registries or GitHub Releases based on configuration.

#### Scenario: Local registry publish
- **WHEN** the target is a local registry path
- **THEN** artifacts SHALL be copied to that path and a registry index updated

#### Scenario: GitHub Releases publish
- **WHEN** the target is GitHub Releases
- **THEN** the workflow SHALL create or update a release and upload all assets via the GitHub API

### Requirement: Pre-publish validation
Before uploading, the workflow SHALL validate artifact integrity, license metadata, and package consistency.

#### Scenario: Integrity check
- **WHEN** an artifact hash does not match the computed checksum
- **THEN** the publish SHALL fail and report the mismatch

#### Scenario: License metadata validation
- **WHEN** the package metadata lacks a recognized SPDX license identifier
- **THEN** the publish SHALL fail with a license validation error

#### Scenario: Package consistency check
- **WHEN** a required file listed in the manifest is missing from the build output
- **THEN** the publish SHALL fail and list the missing files

### Requirement: Publishing history
The publisher SHALL maintain an append-only history log of all publish operations.

#### Scenario: History entry created
- **WHEN** a publish operation completes
- **THEN** a timestamped entry SHALL be appended to the publishing history with version, channel, target, and artifact hashes

#### Scenario: History query
- **WHEN** the user requests the publication history
- **THEN** the history SHALL be displayed in reverse chronological order

### Requirement: Rollback
The publisher SHALL support rolling back a published version by restoring the previous version's artifacts.

#### Scenario: Rollback succeeds
- **WHEN** the user requests a rollback to a prior version
- **THEN** the previous version's artifacts SHALL be re-uploaded and the channel pointer reverted

#### Scenario: Rollback with no prior version
- **WHEN** no prior version exists in the target channel
- **THEN** the rollback SHALL fail with a diagnostic error

### Requirement: Concurrent publish prevention
The workflow SHALL prevent concurrent publishes to the same channel.

#### Scenario: Lock acquired
- **WHEN** a publish to a channel is in progress
- **THEN** a second publish to the same channel SHALL be rejected with a conflict error
