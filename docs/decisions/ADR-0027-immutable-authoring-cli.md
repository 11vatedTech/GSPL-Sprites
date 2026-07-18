# ADR-0027: Immutable authoring CLI operations

- Status: Accepted
- Date: 2026-07-17

## Context

Persisted projects are not usable without process-level workflows. A CLI that edits files in place would weaken revision ancestry, conflict detection, and recovery guarantees.

## Decision

Expose inspect, revise, lower, and build commands. Revisions require the caller's expected identity and a distinct output path. Lowered seeds and packages publish transactionally and reject existing destinations. All commands reuse the production authoring, seed, package, and verification contracts.

## Consequences

Headless users and future authoring applications share the same domain behavior. Revision history remains explicit and recoverable. Ability edits and new project creation require subsequent typed commands rather than an unbounded generic patch language.
