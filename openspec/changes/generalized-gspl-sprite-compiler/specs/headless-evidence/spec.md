## ADDED Requirements

### Requirement: Evidence mode SHALL include _schema_version in JSON output
The `write_trace_json()` output SHALL include a top-level `_schema_version` field set to `"gspl_evidence_v1"`.

#### Scenario: JSON contains schema version
- **WHEN** `write_trace_json()` is called on any EvidenceTrace
- **THEN** the output JSON matches `"_schema_version": "gspl_evidence_v1"`

### Requirement: Event JSON SHALL include status_id and ability_name
Each event object in the JSON output SHALL include `status_id` and `ability_name` fields alongside existing tick, kind, form_before, form_after, and health fields.

#### Scenario: Event with non-empty ability name
- **WHEN** an event has ability_name="take_damage"
- **THEN** the JSON object SHALL contain `"ability_name": "take_damage"`

#### Scenario: Event with non-empty status_id
- **WHEN** an event has status_id="stunned"
- **THEN** the JSON object SHALL contain `"status_id": "stunned"`

## REMOVED Requirements

### Requirement: Evidence mode SHALL write a JSON trace file
**Reason**: Replaced by more specific requirements for format and content
**Migration**: Use write_trace_json() which now returns a string; callers control file I/O
