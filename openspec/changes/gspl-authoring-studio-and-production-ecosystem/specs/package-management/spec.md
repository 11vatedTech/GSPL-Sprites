## ADDED Requirements

### Requirement: Package creation with manifest
The package management system SHALL support creating a GSPL package from a directory, auto-generating or validating a `package.json` manifest.

#### Scenario: Create package from directory
- **WHEN** the user runs the package create command on a directory containing a valid `package.json` and sprite assets
- **THEN** the system produces a `.gspl-pkg` archive with the manifest and all assets

#### Scenario: Manifest auto-generation
- **WHEN** the user runs package create without a `package.json`
- **THEN** the system interactively prompts for required fields and generates the manifest before packaging

### Requirement: Versioning with semver
All packages SHALL use semantic versioning (MAJOR.MINOR.PATCH) as specified in the manifest.

#### Scenario: Valid semver accepted
- **WHEN** a package manifest declares `"version": "2.1.3"`
- **THEN** the package is accepted and the version is stored in the registry

#### Scenario: Invalid version rejected
- **WHEN** a package manifest declares `"version": "v2.1"`
- **THEN** the system rejects the package with a version format diagnostic

### Requirement: Dependency resolution
The package management system SHALL resolve transitive dependencies and produce a deterministic installation order.

#### Scenario: Resolve transitive dependencies
- **WHEN** package A depends on B at `^1.0` and B depends on C at `~2.0`
- **THEN** the resolver selects the latest compatible version of B and C and returns the ordered list A, B, C

#### Scenario: Version conflict diagnostic
- **WHEN** two packages require different major versions of the same dependency (B@1.0 and B@2.0)
- **THEN** the resolver emits a conflict diagnostic listing the conflicting requirements

### Requirement: Hierarchical package IDs
Packages SHALL use a hierarchical ID format `publisher/package` to avoid naming collisions.

#### Scenario: Unique hierarchical IDs
- **WHEN** two different publishers each create a package named `texture-pack`
- **THEN** they are stored as `alice/texture-pack` and `bob/texture-pack` respectively without collision

#### Scenario: Install by hierarchical ID
- **WHEN** the user runs `gspl package install alice/texture-pack`
- **THEN** the system installs the package published by alice named texture-pack

### Requirement: Local registry
The package management system SHALL maintain a local registry index of all installed packages with their versions and dependency trees.

#### Scenario: List installed packages
- **WHEN** the user runs `gspl package list`
- **THEN** the system displays all locally installed packages with their IDs, versions, and descriptions

#### Scenario: Query a specific package
- **WHEN** the user runs `gspl package list alice/texture-pack`
- **THEN** the system displays the manifest metadata and dependency tree for that specific package

### Requirement: Remote registry pull/push
The package management system SHALL support pulling packages from and pushing packages to a remote registry URL.

#### Scenario: Pull a package from remote
- **WHEN** the user runs `gspl package install bob/texture-pack --registry https://registry.gspl.io`
- **THEN** the system downloads, verifies, and installs the package from the specified remote registry

#### Scenario: Push a package
- **WHEN** the user runs `gspl package publish ./build/texture-pack-1.0.0.gspl-pkg --registry https://registry.gspl.io`
- **THEN** the system uploads the package archive to the remote registry

### Requirement: Integrity verification
Every package operation SHALL verify the package archive's integrity using a SHA-256 hash and, when available, a cryptographic signature.

#### Scenario: Hash verification on install
- **WHEN** the user installs a package whose computed SHA-256 matches the hash in the registry
- **THEN** the installation proceeds

#### Scenario: Tampered package is rejected
- **WHEN** the user attempts to install a package whose payload hash does not match the manifest
- **THEN** the system rejects the installation with an integrity diagnostic

#### Scenario: Signature verification
- **WHEN** a package includes a GPG or Ed25519 signature and the signer's public key is trusted
- **THEN** the system verifies the signature and reports the signing identity

### Requirement: Workspace installation
The package management system SHALL support installing packages into a specific workspace directory rather than globally.

#### Scenario: Install to workspace
- **WHEN** the user runs `gspl package install alice/texture-pack --workspace ./my-project`
- **THEN** the package is installed under `./my-project/gspl-packages/` and scoped to that workspace

### Requirement: Package update
The package management system SHALL support updating installed packages to the latest compatible version per declared semver range.

#### Scenario: Update within range
- **WHEN** the workspace constrains `alice/texture-pack` to `^1.0`, version 1.2.0 is installed, and 1.3.0 is available
- **THEN** `gspl package update` upgrades to 1.3.0

#### Scenario: Major version requires explicit upgrade
- **WHEN** version 2.0.0 is available but the constraint is `^1.0`
- **THEN** `gspl package update` does not upgrade automatically and the user must explicitly request `gspl package install alice/texture-pack@2.0.0`

### Requirement: Package uninstall
The package management system SHALL support uninstalling packages with cascade removal of dependent packages.

#### Scenario: Uninstall with dependents check
- **WHEN** the user runs `gspl package uninstall alice/texture-pack` and no other package depends on it
- **THEN** the package is removed from the registry and its files are deleted

#### Scenario: Uninstall blocked by dependents
- **WHEN** another package depends on `alice/texture-pack`
- **THEN** the system warns about the dependent and refuses uninstall unless `--force` is specified
