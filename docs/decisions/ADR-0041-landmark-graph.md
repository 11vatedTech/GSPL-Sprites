# ADR-0041: Landmark graph

## Status

Accepted.

## Decision

Introduce a first-class semantic landmark graph: named landmarks with
roles, owner structures, local frames, symmetry partners, deformability,
visibility, silhouette-anchor flags and relational constraints. Landmarks
are not limited to humanoid anatomy (eye_center.left, wheel_center.
front_left, wing_root.right, tail_tip are all legal).

## Rationale

Recognition relies on configural landmark relationships (Biederman 1987;
production practice). Landmarks give the compiler and future solvers a
sparse semantic handle on identity, expression, gaze, contact and
retargeting, independent of representation.

## Consequences

Proportions, invariants, gaze and silhouette goals reference landmarks.
Validation rejects dangling refs, ordering violations and missing required
landmarks. Key poses defined on landmarks retarget across entities by
construction.
