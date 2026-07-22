# gspl-sdk
## Purpose
Stable C++ SDK, self-contained headers
## ADDED Requirements
### Requirement: Public API surface
The SDK SHALL expose a stable public API through include/gspl/*.hpp with no internal implementation leakage.
#### Scenario: SDK header includes compile independently
Given a compilation unit, When it includes any include/gspl/*.hpp header, Then it SHALL compile without errors.
### Requirement: RAII context
The SDK SHALL provide a GsplContext RAII class for explicit ownership and lifecycle management.
#### Scenario: GsplContext compiles and runs
Given a GsplContext instance, When constructed and destroyed, Then no errors SHALL occur.
### Requirement: API stability policy
The SDK SHALL document its stability policy in STABILITY.md.
#### Scenario: STABILITY.md exists
Given the include/gspl/STABILITY.md file, When read, Then it SHALL contain versioning and deprecation policies.

