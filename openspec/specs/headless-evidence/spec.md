# headless-evidence

## Purpose

Run a deterministic headless Voltfox lifetime scenario for test reproducibility, trace recording, and oracle-based regression detection, with no graphical dependencies.

## Requirements

### Requirement: Headless evidence mode SHALL run a fixed Voltfox lifetime scenario
A `run_headless_evidence()` function SHALL execute a deterministic scenario: Voltfox spawns → senses idle → transforms to Running → takes damage → transforms to Hurt → recovers → transforms to Attack → uses directional lightning → transforms to Special → returns to Idle → despawns. The scenario SHALL run at a fixed 16ms tick rate with a seeded RNG (seed=42).

#### Scenario: Scenario completes without error
- **WHEN** running the headless evidence scenario
- **THEN** it completes all lifecycle steps without throwing an exception

### Requirement: Evidence mode SHALL record a deterministic event trace
Each step of the scenario SHALL emit a timestamped `EvidenceEvent` record containing: tick number, event type, form before, form after, HP, statuses, and triggered ability name. All events SHALL be collected into an ordered `EvidenceTrace`.

#### Scenario: Trace contains at least 6 form-change events
- **WHEN** running the full scenario
- **THEN** the trace contains >= 6 form-change events (Idle→Running, Running→Hurt, etc.)

### Requirement: Evidence mode SHALL write a JSON trace file
The `EvidenceTrace` SHALL be serializable to JSON via `write_trace_json()`, producing a file at a specified path. The JSON SHALL contain all event fields as an array of objects.

#### Scenario: JSON trace file is valid
- **WHEN** writing the trace to a JSON file
- **THEN** the file parses as valid JSON and contains the expected number of events

### Requirement: A committed oracle SHALL verify trace determinism
A reference JSON oracle file SHALL be committed to the repository. A test SHALL run the headless evidence scenario and compare the produced trace against the oracle. Any difference SHALL fail the test.

#### Scenario: Trace matches oracle
- **WHEN** running the headless scenario and comparing to the committed oracle
- **THEN** all events match exactly (same count, same field values)

### Requirement: Headless mode SHALL produce no graphical output
The headless evidence scenario SHALL run without creating any windows or requiring a display server. This SHALL be enforced by not linking against any graphics library in the headless target.

#### Scenario: Headless test runs without display
- **WHEN** running the headless evidence test on a system without a display
- **THEN** the test passes (no graphics dependencies required)
