## ADDED Requirements

### Requirement: Compile-target pair definition
A target adapter SHALL define a mapping between a build profile and a compile target platform.

#### Scenario: Valid profile-target pair
- **WHEN** an adapter defines a profile `release` and target `windows-x64`
- **THEN** the build system SHALL compile using configuration flags associated with that pair

#### Scenario: Unregistered profile error
- **WHEN** an adapter references a profile that does not exist in the build configuration
- **THEN** the system SHALL emit a diagnostic error

### Requirement: Target-specific optimization constraints
Target adapters SHALL specify optimization constraints (e.g., SIMD level, instruction set, alignment) per target.

#### Scenario: SIMD constraint applied
- **WHEN** a target adapter declares `simd: avx2`
- **THEN** the compiler SHALL enable AVX2 code generation for that target

#### Scenario: Incompatible constraint warning
- **WHEN** a constraint conflicts with the host compiler's capabilities
- **THEN** the adapter SHALL log a warning and fall back to a compatible setting

### Requirement: SDK detection and validation
The system SHALL detect installed SDKs and validate them against target adapter requirements.

#### Scenario: SDK found and valid
- **WHEN** a target adapter requires a specific SDK version that is installed
- **THEN** the build SHALL proceed with SDK paths configured automatically

#### Scenario: SDK missing
- **WHEN** a required SDK is not detected on the host
- **THEN** the system SHALL fail with a clear error message naming the missing SDK

#### Scenario: SDK version mismatch
- **WHEN** the detected SDK version does not meet the minimum required version
- **THEN** the system SHALL fail with a version mismatch error

### Requirement: Cross-compilation support
Target adapters SHALL support cross-compilation by specifying a different target triple than the host.

#### Scenario: Cross-compile configured
- **WHEN** the target triple differs from the host triple
- **THEN** the build SHALL invoke the cross-compiler toolchain specified in the adapter

#### Scenario: Missing cross-toolchain
- **WHEN** no cross-toolchain is installed for the requested target triple
- **THEN** the system SHALL fail with a toolchain-not-found error

### Requirement: Adapter manifest format
Every target adapter SHALL include a manifest file declaring identity, dependencies, and constraints.

#### Scenario: Manifest parsed correctly
- **WHEN** the system loads a target adapter
- **THEN** it SHALL parse the manifest and register all declared properties

#### Scenario: Malformed manifest rejection
- **WHEN** the manifest is not valid JSON or YAML as expected
- **THEN** the system SHALL reject the adapter with a parse error

#### Scenario: Missing required fields
- **WHEN** the manifest omits a required field (e.g., `name`, `version`, `target-triple`)
- **THEN** the system SHALL reject the adapter with a validation error

### Requirement: Target listing
The system SHALL enumerate all installed and available target adapters.

#### Scenario: Installed targets listed
- **WHEN** the user lists targets
- **THEN** all registered adapters SHALL be displayed with name, version, and target triple

#### Scenario: Available targets shown
- **WHEN** the user requests available (not yet installed) targets
- **THEN** the system SHALL query a remote index and display installable adapters

### Requirement: Adapter installation
The system SHALL support installing target adapters from a local file or remote registry.

#### Scenario: Install from registry
- **WHEN** the user installs a target adapter by name
- **THEN** the system SHALL download, verify, and register the adapter

#### Scenario: Install from file
- **WHEN** the user installs an adapter from a local `.gspl-target` file
- **THEN** the system SHALL copy it to the adapters directory and register it

#### Scenario: Duplicate installation
- **WHEN** the user attempts to install an adapter that is already installed
- **THEN** the system SHALL prompt for confirmation before overwriting

### Requirement: Adapter version compatibility check
The system SHALL verify that an adapter's declared compatible engine version matches the running engine.

#### Scenario: Compatible version
- **WHEN** the adapter's engine version constraint satisfies the running engine version
- **THEN** the adapter SHALL be loaded without warning

#### Scenario: Incompatible version
- **WHEN** the adapter requires a newer engine version
- **THEN** the system SHALL emit a diagnostic error and refuse to load the adapter
