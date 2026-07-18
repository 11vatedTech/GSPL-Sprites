# ADR-0016: Fixed-tick 3D animation and structural retargeting

## Status

Accepted.

## Decision

Represent joint and morph animation with bounded integer ticks and canonical
fixed-point transforms. Validate clips against the compiled 3D projection and
require retarget maps to be one-to-one and hierarchy-preserving across
independently valid skeletons.

## Consequences

Animation identities are deterministic and invalid tracks fail before export.
Retargeting cannot silently flatten morphology into name matching. More advanced
retargeting may add semantic joint roles, rest-pose compensation, IK, and
deformation metrics, but must preserve these validation and authority boundaries.
