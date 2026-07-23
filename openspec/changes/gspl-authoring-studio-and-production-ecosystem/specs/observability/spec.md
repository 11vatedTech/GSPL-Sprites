## ADDED Requirements

### Requirement: Structured logging
All components SHALL emit structured log records with severity level, component tag, timestamp, and message.

#### Scenario: Log record emitted
- **WHEN** a component processes an event
- **THEN** a structured log record SHALL be written with level, component, ISO-8601 timestamp, and message fields

#### Scenario: Severity filtering
- **WHEN** the log level is set to WARN
- **THEN** DEBUG and INFO messages SHALL be suppressed while WARN, ERROR, and FATAL SHALL pass through

#### Scenario: Component-tagged output
- **WHEN** a log record is written
- **THEN** the record SHALL include the originating component name (e.g., `compiler`, `provider`, `preview`)

### Requirement: Telemetry bus
The studio SHALL provide an in-process telemetry bus for components to publish and subscribe to instrumentation events without direct coupling.

#### Scenario: Event published
- **WHEN** a component publishes a telemetry event
- **THEN** all subscribed consumers SHALL receive the event within 100 milliseconds

#### Scenario: Backpressure handling
- **WHEN** a consumer is slower than the event producer
- **THEN** the bus SHALL buffer events up to a configurable limit and then drop oldest events with a logged warning

### Requirement: System health dashboard
The studio SHALL expose a real-time dashboard showing component status, resource usage, and recent errors.

#### Scenario: Dashboard displays live status
- **WHEN** the health dashboard is open
- **THEN** it SHALL update component status indicators every 500 milliseconds

#### Scenario: Error highlighted
- **WHEN** a component reports an error state
- **THEN** the dashboard SHALL highlight that component in red and display the last error message

### Requirement: Diagnostics export
The user SHALL be able to export a diagnostic bundle containing logs, configuration, and system information.

#### Scenario: Bundle exported
- **WHEN** the user triggers diagnostics export
- **THEN** a compressed archive SHALL be created containing logs, current config, and system info

#### Scenario: Sensitive data redacted
- **WHEN** the diagnostics bundle is being assembled
- **THEN** any known secrets, tokens, or paths containing user names SHALL be redacted

### Requirement: Crash reporter
The studio SHALL capture unhandled exceptions and crashes and present a report dialog to the user.

#### Scenario: Crash caught
- **WHEN** an unhandled exception occurs
- **THEN** a crash report dialog SHALL appear with a stack trace, component identity, and option to save a report

#### Scenario: Crash report saved
- **WHEN** the user chooses to save a crash report
- **THEN** a file SHALL be written to the diagnostics directory with all available context

### Requirement: Log viewer
The studio SHALL include a built-in log viewer for browsing and filtering log records in real time.

#### Scenario: Logs displayed
- **WHEN** the log viewer is opened
- **THEN** log records SHALL be displayed in a scrollable list with severity coloring and component filtering

#### Scenario: Log search
- **WHEN** the user enters a search term in the log viewer
- **THEN** only matching log records SHALL be shown, with matches highlighted

#### Scenario: Real-time tailing
- **WHEN** new log records are written
- **THEN** the log viewer SHALL append them to the display in real time

### Requirement: Metric collection
The observability system SHALL collect runtime metrics including memory usage, CPU utilization, event counts, and operation latencies.

#### Scenario: Metrics recorded periodically
- **WHEN** the studio is running
- **THEN** memory and CPU metrics SHALL be sampled every 5 seconds and stored in a ring buffer

#### Scenario: Counter metrics
- **WHEN** an operation occurs (e.g., compile, preview frame)
- **THEN** the corresponding counter SHALL be incremented and exposed via the telemetry bus

#### Scenario: Latency histogram
- **WHEN** an operation with latency tracking completes
- **THEN** its duration SHALL be recorded in a histogram with p50, p95, and p99 buckets

### Requirement: Log retention policy
The system SHALL enforce a configurable log retention policy to prevent unbounded disk usage.

#### Scenario: Log rotation
- **WHEN** a log file reaches the configured maximum size (default 10 MB)
- **THEN** the system SHALL rotate the log file and compress the previous file

#### Scenario: Age-based cleanup
- **WHEN** a log file is older than the configured retention period (default 30 days)
- **THEN** the system SHALL delete the file

### Requirement: Health check endpoint
The main process SHALL expose a health check endpoint for external monitoring.

#### Scenario: Healthy response
- **WHEN** an external monitor requests the health endpoint
- **THEN** the system SHALL respond with HTTP 200 and a JSON body listing component statuses

#### Scenario: Degraded response
- **WHEN** one or more components are in an error state
- **THEN** the system SHALL respond with HTTP 503 and a JSON body detailing the degraded components
