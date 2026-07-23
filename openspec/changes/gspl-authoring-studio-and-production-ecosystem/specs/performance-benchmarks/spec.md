## ADDED Requirements

### Requirement: Configurable benchmarking framework
The benchmarking framework SHALL accept workload definitions specifying source files, input sprites, and measurement parameters.

#### Scenario: Workload loaded from config
- **WHEN** the user provides a benchmark configuration file
- **THEN** the framework SHALL load and execute each defined workload, recording metrics per workload

#### Scenario: Invalid workload rejected
- **WHEN** a workload definition references a nonexistent source file
- **THEN** the framework SHALL emit a diagnostic error and skip that workload

### Requirement: Compilation throughput measurement
The framework SHALL measure compilation throughput in source bytes per second.

#### Scenario: Throughput recorded
- **WHEN** a compilation workload completes
- **THEN** the benchmark SHALL report bytes compiled divided by wall-clock compile time

#### Scenario: Multi-file throughput
- **WHEN** a workload contains multiple source files
- **THEN** throughput SHALL be reported as aggregate bytes across all files divided by total compile time

### Requirement: Preview FPS tracking
The framework SHALL measure preview rendering frame rate for sprite animations.

#### Scenario: FPS recorded
- **WHEN** a preview workload runs
- **THEN** the benchmark SHALL report average, minimum, and maximum frames per second over the measurement window

#### Scenario: FPS under load
- **WHEN** the workload includes complex animations with multiple layers
- **THEN** the benchmark SHALL still record FPS without frame dropping in the measurement

### Requirement: Provider inference latency measurement
The framework SHALL measure the end-to-end latency of provider inference calls.

#### Scenario: Latency recorded
- **WHEN** a provider inference workload executes
- **THEN** the benchmark SHALL report p50, p95, and p99 latency in milliseconds

#### Scenario: Latency with warm-up
- **WHEN** inference latency is measured
- **THEN** the framework SHALL perform a warm-up iteration before recording measurements

### Requirement: Memory usage tracking
The framework SHALL track peak and average memory usage during each benchmark workload.

#### Scenario: Peak memory recorded
- **WHEN** a workload completes
- **THEN** the benchmark SHALL report the peak working set in megabytes

#### Scenario: Memory leak detection
- **WHEN** memory usage increases monotonically across repeated iterations
- **THEN** the framework SHALL flag a potential memory leak in the report

### Requirement: Artifact size tracking
The framework SHALL record the size of all build artifacts produced by a workload.

#### Scenario: Artifact size reported
- **WHEN** a compilation workload finishes
- **THEN** the benchmark SHALL list each output artifact and its size in bytes

#### Scenario: Size regression flag
- **WHEN** an artifact is more than 10% larger than the baseline
- **THEN** the framework SHALL flag it as a regression

### Requirement: Regression detection
The framework SHALL compare current benchmark results against a stored baseline and flag regressions.

#### Scenario: Regression detected
- **WHEN** a metric degrades beyond a configurable threshold relative to baseline
- **THEN** the framework SHALL mark that metric as a regression and exit with a non-zero code

#### Scenario: No baseline available
- **WHEN** no baseline file exists for comparison
- **THEN** the framework SHALL record results as the new baseline without regression checking

### Requirement: Baseline comparison
The framework SHALL support storing and loading baseline snapshots for comparison across runs.

#### Scenario: Baseline loaded from file
- **WHEN** a baseline file is specified
- **THEN** the framework SHALL load it and compare each metric against the current run

#### Scenario: Baseline updated
- **WHEN** the user approves the current run as the new baseline
- **THEN** the framework SHALL overwrite the baseline file with current results

### Requirement: Benchmark report export
The framework SHALL export results to JSON, CSV, and HTML report formats.

#### Scenario: JSON export
- **WHEN** the user specifies JSON output format
- **THEN** the framework SHALL write a JSON file with all metrics, workloads, and regression flags

#### Scenario: HTML report generated
- **WHEN** the user specifies HTML output format
- **THEN** the framework SHALL generate a self-contained HTML page with tables and charts

#### Scenario: Unsupported format rejected
- **WHEN** the user specifies an unsupported export format
- **THEN** the framework SHALL emit a diagnostic error listing supported formats
