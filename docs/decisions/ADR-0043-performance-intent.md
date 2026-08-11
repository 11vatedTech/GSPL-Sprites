# ADR-0043: Performance intent

## Status

Accepted.

## Decision

Introduce PerformanceIntent (action, phase, commitment, force, emotion,
balance, gaze, timing cadence) and KeyPose (line of action, center of
mass, support contacts, silhouette goal, gaze, force, expression,
deformation requests) as the semantic layer above raw animation frames.
Intents lower deterministically to the existing PerformanceState consumed
by the Visual Semantic Compiler.

## Rationale

Poses carry meaning (line of action, balance, intent) that joint angles
cannot express; motion is sparse semantic keys plus timing (Thomas &
Johnston, Williams). One intent must drive pose, deformation, expression
and FX simultaneously rather than per-system ad-hoc state.

## Consequences

Key poses are retargetable (role/landmark-defined). Motion abstraction
(smear/stepped) is declared by intent, never automatic. Deterministic
tick→frame sequences remain guaranteed.
