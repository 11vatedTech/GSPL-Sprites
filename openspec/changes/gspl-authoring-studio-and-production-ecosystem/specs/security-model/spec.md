## ADDED Requirements

### Requirement: Process isolation
The compiler, provider, preview, and plugin workers SHALL run in separate OS-level processes with restricted capabilities.

#### Scenario: Compiler isolation
- **WHEN** a sprite is compiled
- **THEN** the compiler SHALL execute in a dedicated process that cannot access the provider or preview processes

#### Scenario: Worker crash isolation
- **WHEN** a worker process crashes
- **THEN** the crash SHALL NOT terminate the main studio process or other workers

### Requirement: Workspace-scoped authority model
Every operation SHALL be scoped to the workspace directory and denied access outside it.

#### Scenario: Workspace-bound file read
- **WHEN** a plugin reads a file
- **THEN** the read SHALL be allowed only if the path resolves within the workspace root

#### Scenario: Out-of-scope access denied
- **WHEN** a component attempts to read a file outside the workspace
- **THEN** the access SHALL be denied and a security event logged

#### Scenario: Symlink escape prevented
- **WHEN** a path contains a symlink pointing outside the workspace
- **THEN** the system SHALL resolve the real path and deny access if it falls outside the workspace

### Requirement: Package signature verification
All published packages SHALL have their signatures verified before installation or loading.

#### Scenario: Signature valid
- **WHEN** a package's signature matches its content and the signer's certificate is trusted
- **THEN** the package SHALL be accepted for installation

#### Scenario: Signature invalid
- **WHEN** a package's signature does not match its content
- **THEN** the package SHALL be rejected and the user alerted

#### Scenario: Untrusted signer
- **WHEN** the signer's certificate is not in the trusted store
- **THEN** the package SHALL be rejected with an untrusted-signer error

### Requirement: No network access in core
The compiler and runtime core SHALL NOT make any network connections.

#### Scenario: Network call blocked
- **WHEN** an internal component attempts to open a socket or make an HTTP request
- **THEN** the attempt SHALL fail with a security denial error

#### Scenario: Studio-level networking permitted
- **WHEN** the studio UI or package manager makes a controlled network request
- **THEN** the request SHALL be allowed through a designated networking component, not from core

### Requirement: Input validation on all external data paths
Every external input (file read, IPC message, command-line argument, environment variable) SHALL be validated before use.

#### Scenario: File path validated
- **WHEN** a file path is received from an external source
- **THEN** the system SHALL validate length, character set, and reject path traversal patterns

#### Scenario: IPC message schema validated
- **WHEN** an IPC message is received from a worker
- **THEN** the message SHALL be validated against a schema before processing

#### Scenario: Bound-checked numeric input
- **WHEN** a numeric value is parsed from external input
- **THEN** the system SHALL clamp or reject values outside the acceptable range

### Requirement: Crash recovery
The system SHALL recover from worker crashes without data loss and with a clear user notification.

#### Scenario: Worker restarted
- **WHEN** a worker process terminates unexpectedly
- **THEN** the main process SHALL restart the worker and log the incident

#### Scenario: Unsaved work preserved
- **WHEN** a preview worker crashes
- **THEN** the editor document buffer SHALL NOT be affected and SHALL remain open for saving

### Requirement: Privilege separation
The studio SHALL run with the minimum necessary OS privileges and SHALL NOT request elevation.

#### Scenario: No elevation requested
- **WHEN** the studio starts
- **THEN** it SHALL NOT prompt for administrator/root elevation

#### Scenario: Privileged operation delegated
- **WHEN** an operation requires elevated privileges (e.g., installing a system-wide SDK)
- **THEN** the studio SHALL delegate to a separate helper process that requests elevation only for that operation

### Requirement: Audit logging
All security-relevant events SHALL be logged to a tamper-evident audit trail.

#### Scenario: Access denial logged
- **WHEN** a file access is denied due to workspace scope rules
- **THEN** the event SHALL be recorded with timestamp, requesting component, path, and denial reason

#### Scenario: Audit log integrity
- **WHEN** the audit log is read
- **THEN** the system SHALL verify a hash chain to detect tampering

### Requirement: Secure IPC channel
Communication between the main process and workers SHALL use authenticated, encrypted IPC channels.

#### Scenario: IPC message authenticated
- **WHEN** a worker sends a message to the main process
- **THEN** the message SHALL include a per-session authentication token

#### Scenario: Unauthenticated message rejected
- **WHEN** a message arrives without a valid authentication token
- **THEN** the message SHALL be discarded and a security event logged
