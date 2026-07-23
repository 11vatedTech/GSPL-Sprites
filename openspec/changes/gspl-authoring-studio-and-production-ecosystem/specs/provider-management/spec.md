## ADDED Requirements

### Requirement: Provider registry UI
The provider management UI SHALL display all registered providers with their name, version, capability set, and enabled/disabled state.

#### Scenario: View provider list
- **WHEN** the user opens the provider management panel
- **THEN** a table lists every registered provider showing name, version, capabilities, and status toggle

### Requirement: Install providers
The provider management system SHALL support installing new providers from a provider package file (`.gspl-provider`).

#### Scenario: Install from file
- **WHEN** the user selects a `.gspl-provider` file and confirms installation
- **THEN** the provider is copied into the provider directory, registered in the registry, and appears in the UI

#### Scenario: Install duplicate version
- **WHEN** the user attempts to install a provider that already exists at the same version
- **THEN** the system rejects the installation with a duplicate diagnostic

### Requirement: Configure providers
The provider management system SHALL expose each provider's configuration schema and allow the user to edit configuration values.

#### Scenario: Edit provider configuration
- **WHEN** the user opens the configuration editor for a provider with fields `api_key`, `endpoint`, and `timeout`
- **THEN** the editor renders each field with appropriate input controls and persists changes on save

#### Scenario: Invalid configuration is rejected
- **WHEN** the user enters a non-numeric value in the `timeout` field which expects an integer
- **THEN** the editor shows a validation error and does not save

### Requirement: Enable/disable providers
The provider management system SHALL allow toggling any provider between enabled and disabled states without uninstalling it.

#### Scenario: Disable a provider
- **WHEN** the user flips the enabled toggle off for a provider
- **THEN** the provider is deactivated and no pipeline step uses it until re-enabled

#### Scenario: Re-enable a provider
- **WHEN** the user flips the toggle on for a disabled provider
- **THEN** the provider is activated and available for pipeline steps

### Requirement: Provider-side preview rendering
The provider management system SHALL support rendering a preview of a provider's output within the studio UI.

#### Scenario: Request provider preview
- **WHEN** the user selects a provider and clicks preview
- **THEN** the provider renders its current output and displays it in the preview panel

### Requirement: Monitoring dashboard
The provider management system SHALL display real-time and historical monitoring data for each provider including inference latency, throughput, and error rates.

#### Scenario: View provider metrics
- **WHEN** the user selects a provider and opens the monitoring tab
- **THEN** charts display average latency, requests per minute, and error rate over the last hour

#### Scenario: Error rate alert threshold
- **WHEN** a provider's error rate exceeds 10% in a one-minute window
- **THEN** the monitoring dashboard highlights the provider with a warning badge

### Requirement: Provider isolation via worker process
Each provider SHALL execute in its own worker process isolated from the main compiler and from other providers.

#### Scenario: Provider crash does not affect compiler
- **WHEN** a provider's worker process crashes
- **THEN** the compiler or studio remains operational and the provider is marked as failed with a restart option

### Requirement: Provider configuration validation
The provider management system SHALL validate provider configuration against the provider's declared schema before activation.

#### Scenario: Valid configuration activates
- **WHEN** the provider declares a required `endpoint` string field and the user provides a valid URL
- **THEN** activation succeeds and the provider enters enabled state

#### Scenario: Missing required field prevents activation
- **WHEN** the provider declares `api_key` as required but the configuration omits it
- **THEN** activation fails with a diagnostic listing the missing required field

### Requirement: Log viewer
The provider management system SHALL display real-time and historical log output from each provider in a filterable log viewer.

#### Scenario: View provider logs
- **WHEN** the user selects a provider and opens its log viewer
- **THEN** log entries with timestamps, severity levels, and messages are displayed, filterable by severity
