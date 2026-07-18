# ADR-0024: Transactional, import-validated Godot targets

- Status: Accepted
- Date: 2026-07-17

## Context

A GLB that conforms to the Khronos specification is not by itself a usable engine package. Godot needs a project and scene boundary, while GSPL requires explicit feature preservation, integrity, reproducibility, and failure behavior.

## Decision

Generate a minimal runnable Godot 4.7 project around the validated GLB. Derive target requirements from the payload, fail unsupported required semantics before writing, publish through a sibling staging directory, and verify a canonical hash manifest before atomic rename. Validate fixtures with the official Godot editor importer. Treat LOD as adapter-emulated until Godot-side selection is generated and validated.

## Consequences

The output can be opened and imported directly by Godot without a repository dependency or plugin. The adapter truthfully covers the 3D asset boundary but not the living runtime or other projections. Adding those features requires their actual translation, capability evidence, tests, and engine validation.
