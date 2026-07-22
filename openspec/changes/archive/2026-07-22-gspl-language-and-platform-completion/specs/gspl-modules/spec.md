# gspl-modules
## Purpose
Module system, imports, name resolution, source roots
## ADDED Requirements
### Requirement: Module identity and path resolution
Modules SHALL have deterministic identity derived from content hash and SHALL be resolved via ModulePath.
#### Scenario: Module path parses correctly
Given a module path string, When parsed, Then a ModulePath SHALL be produced with the correct components.
### Requirement: Import resolution with cycle detection
The module system SHALL resolve imports and detect cycles.
#### Scenario: Duplicate import rejected
Given two modules with the same identity, When registered, Then a duplicate diagnostic SHALL be emitted.
### Requirement: Safe source roots
Source roots SHALL prevent path traversal attacks.
#### Scenario: Path traversal rejected
Given a path attempting traversal outside a source root, When resolved, Then a diagnostic SHALL be emitted.

