# headless-evidence-contract

## Purpose

Defines the versioned JSON serialization format for the headless evidence trace, including schema identification, complete event serialization, and committed oracle verification.

## Requirements

### Requirement: Versioned JSON output
write_trace_json SHALL produce JSON with a top-level _schema_version field set to "gspl_evidence_v1".

#### Scenario: Schema version present
- **WHEN** write_trace_json is called
- **THEN** the output JSON SHALL contain "_schema_version": "gspl_evidence_v1"

### Requirement: Complete event serialization
Each event in the JSON output SHALL include tick, kind, form_before, form_after, health, status_id, and ability_name.

#### Scenario: Status and ability fields
- **WHEN** an event has non-empty status_id and ability_name
- **THEN** both fields SHALL appear in the JSON output

### Requirement: Committed oracle
The tests/evidence_oracle.json file SHALL contain the expected deterministic output for the headless evidence scenario (seed=42). The headless_evidence test SHALL verify that current output matches the oracle byte-for-byte.

#### Scenario: Oracle match
- **WHEN** run_headless_evidence is called with the headless.test SpriteIr
- **THEN** write_trace_json output SHALL exactly match the committed evidence_oracle.json

#### Scenario: Deterministic output
- **WHEN** run_headless_evidence is called twice with the same SpriteIr
- **THEN** both traces SHALL have identical event counts, tick values, and event kinds
