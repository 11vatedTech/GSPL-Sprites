# ADR-0023: Fail-closed target feature preservation

- Status: Accepted
- Date: 2026-07-17

## Context

Portable artifacts and glTF can carry substantial entity data, but neither a file format nor an engine name proves preservation of living runtime, collision, 2.5D, LOD selection, or other GSPL semantics. Silent partial import would create plausible-looking but behaviorally incorrect entities.

## Decision

Every adapter declares evidence-backed capabilities. Compatibility evaluation resolves each requested feature to native, adapter-emulated, or unsupported. Missing declarations are unsupported, required unsupported features fail, duplicates are invalid, and reports use canonical ordering.

## Consequences

Adapters cannot claim engine fidelity before their translation and validation exist. The contract adds deliberate work when a feature is introduced, but makes semantic loss observable and machine-testable.
