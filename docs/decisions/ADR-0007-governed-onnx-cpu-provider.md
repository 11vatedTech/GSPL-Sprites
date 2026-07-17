# ADR-0007: Governed ONNX Runtime CPU provider

## Status

Accepted.

## Decision

Use the official SHA-256-pinned ONNX Runtime 1.26.0 Windows x64 CPU distribution
for the portable baseline provider. Require immutable descriptor identity,
license admission, explicit CPU/FP32 capability, model-file hash verification,
and full tensor-contract reconciliation before execution. Bound threads and all
adapter-owned tensor bytes. Return provider and model identity with every result.

## Consequences

GSPL Sprites now performs real local inference without a service or API key.
Runtime bootstrap is offline after the verified dependency archive is cached.
The first adapter is deliberately Windows x64 and CPU/FP32 only; acceleration is
not a silent fallback. Governed trusted models may run in process. Models outside
the immutable registry require a future sandboxed provider host because the
runtime owns internal allocations and native parsing.
