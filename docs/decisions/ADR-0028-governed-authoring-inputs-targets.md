# ADR-0028: Govern reference use and target fidelity in authoring state

- Status: Accepted
- Date: 2026-07-17

## Context

Reference-guided creation and multi-target output are foundational product requirements. Bare paths lose content identity and rights intent, while free-form target names permit silent semantic loss.

## Decision

Represent references with content hashes, rights, typed use, and required/optional status. Gate required references during lowering according to their use. Represent target requests with registered adapter IDs and typed feature requirements evaluated by the shared fail-closed target contract. Include both collections in revision identity and persistence.

## Consequences

Mutable URLs cannot silently change an authoring revision, and commercial asset use cannot inherit research-only permission. Target incompatibility becomes visible before synthesis. Compiled authoring packages preserve these records and compatibility decisions as mandatory content-addressed artifacts linked from the canonical seed in the asset graph. Cryptographic authorship and full semantic closure verification remain separate release gates.
