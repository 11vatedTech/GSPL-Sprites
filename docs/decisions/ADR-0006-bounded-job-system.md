# ADR-0006: Bounded dependency-aware synthesis jobs

Status: accepted — 2026-07-17

## Decision

Use an explicitly owned bounded worker pool with immutable job IDs, existing-
dependency-only DAG submission, cooperative cancellation, start deadlines,
bounded retries, resource admission, deterministic single-worker mode, and
terminal failure propagation.

## Consequences

There are no detached or uncontrolled background threads. CPU/GPU work cannot
overcommit declared budgets, dependents never run after an upstream failure,
shutdown is join-safe, and operational scheduling data cannot perturb build
identity. Non-cooperative risky providers require process isolation.

