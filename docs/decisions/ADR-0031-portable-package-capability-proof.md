# ADR-0031: Derive portable-package capabilities from emitted artifacts

- Status: Accepted
- Date: 2026-07-17

## Context

Authoring target requests describe intent, not proof that a compiled package
contains the corresponding artifacts. A report covered only by hashes can be
consistently forged together with its manifest.

## Decision

Derive normalized portable-package requirements from the compiled payload,
emit a registered-adapter report as content-addressed graph artifacts, and
reconstruct that report during independent verification. Check every required
feature against the closed manifest file set.

## Consequences

Package capability claims now prove concrete artifact presence and remain
separate from authoring intent. Hash-consistent false raster, rig, animation,
collision, or channel-map claims fail closed.
