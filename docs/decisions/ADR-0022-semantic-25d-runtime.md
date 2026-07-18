# ADR-0022: Keep 2.5D presentation separate from semantic authority

- Status: Accepted
- Date: 2026-07-17

## Context

Depth-plane sprites need camera-relative view selection, parallax, deformation, and optional hybrid geometry. Applying these presentation transforms to collision or simulation state would make behavior camera-dependent and undermine deterministic replay.

## Decision

Evaluate 2.5D definitions into an immutable frame using fixed-point camera inputs. Presentation planes and attached hybrid geometry receive camera-relative transforms. Collision volumes retain definition-space coordinates and stable ordering. Fixed-axis and camera-facing definitions contain exactly one view; discrete multi-angle definitions use deterministic nearest-angle selection.

## Consequences

Render adapters can consume a complete operational frame without owning semantic state. Physics and replay remain independent of the active camera. A future renderer may interpolate presentation frames, but interpolation must not feed back into collision, authored definitions, or simulation authority.
