# ADR-0005: Provider-independent governed model registry

Status: accepted — 2026-07-17

## Decision

Keep model identity, license, task/tensor contracts, device support, resource
requirements, and determinism in immutable descriptors. Select models through a
deterministic request policy rather than references embedded in genes, compiler
passes, or UI state.

## Consequences

Paid services are not mandatory, models can be replaced without changing
semantic identity, incompatible licenses and devices fail closed, and every
generated artifact can cite the exact descriptor and file hash that produced it.

