# gspl-providers
## Purpose
Provider abstraction, build profiles, cross-platform
## ADDED Requirements
### Requirement: Provider interface
Synthesis and inference providers SHALL implement a pure virtual interface; core code SHALL never import ONNX headers.
#### Scenario: Provider registered and looked up
Given a registered provider, When looked up by name, Then the provider SHALL be returned.
### Requirement: Build profiles
The build system SHALL support GSPL_CORE_ONLY, WITH_ONNX (default), and WITH_PREVIEW profiles.
#### Scenario: CORE_ONLY builds without ONNX
Given GSPL_CORE_ONLY=ON, When configuring CMake, Then the build SHALL succeed without downloading ONNX Runtime.

