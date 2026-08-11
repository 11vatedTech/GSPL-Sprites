# ADR-0042: Deformation envelope

## Status

Accepted.

## Decision

Every important structural/visual property may declare a DeformationEnvelope:
canonical value, allowed deviation, action/expression/style/transformation
scales, hard identity boundary and rigidity flag. Effective deviation is
the context-scaled value clamped by the hard boundary. Off-model is valid
when intentional and bounded; unbounded deviation is rejected.

## Rationale

Production evidence (One Piece anchors, Dragon Ball force deformation,
PSD corrective practice) shows expressive work requires controlled
deviation while identity requires hard anchors. Freezing all proportions
kills expression; unbounded deformation destroys identity. The envelope is
the typed middle path.

## Consequences

Intent-driven deformation is compiler-enforced. Rigid structures and hard
boundaries fail closed on violation. Deformation intent is decoupled from
geometric solvers (FFD/cage/ARAP/DQS/PSD remain below the semantic line as
future realizers).
