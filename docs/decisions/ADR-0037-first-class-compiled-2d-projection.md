# ADR-0037: First-class compiled 2D projections

## Status

Accepted.

## Decision

Treat the complete compiled 2D representation—pixels, atlas layout, animation,
channels, rig, and collision—as one validated, content-identified projection
contract.

## Rationale

An atlas path or metadata document cannot prove that pixels, animation
semantics, auxiliary maps, and gameplay geometry agree. A cohesive artifact
permits fail-closed validation and gives form manifestation the same evidence
quality already available to 2.5D and 3D projections.

## Consequences

2D representation identity changes for any meaningful visual or gameplay
artifact change. Memory ownership is explicit and validation can be expensive;
production compilation should validate once and pass verified evidence to
consumers rather than repeatedly hashing large atlases per presentation frame.
