# ADR-0026: Canonical, transactional authoring persistence

- Status: Accepted
- Date: 2026-07-17

## Context

In-memory authoring state is not a usable project workflow. A durable format must preserve unresolved choices and revisions without delimiter ambiguity, identity aliases, unsafe reads, or partial publication.

## Decision

Use a bounded canonical text profile with strict uppercase percent encoding. Reject any byte sequence that differs from deterministic serialization. Persist new files through a verified sibling staging file and atomic rename, never overwriting an existing project.

## Consequences

Projects round-trip exactly across processes and retain the same semantic revision identity. Immutable revision files are safe to archive and compare. Updating an existing path remains intentionally unsupported; higher-level project history will publish a new revision and advance a separately governed head pointer.
