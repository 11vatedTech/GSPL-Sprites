# ADR-0036: Derived transformation manifestations

## Status

Accepted.

## Decision

Derive form animation and visual projection exclusively from validated
transformation state. Require total bindings from forms to animation states and
3D projections, and from transformations to transition clips. Include the
authoritative state identity in every manifestation frame.

## Rationale

Allowing a renderer or animation controller to own current form creates split
authority and makes replay divergence invisible. Total, validated bindings make
missing production artifacts a compilation failure. State identity lets preview
and target consumers prove which semantic state they manifested.

## Consequences

Form and transition presentation is deterministic, complete, and traceable to
semantic authority. Bindings target validated 2D, 2.5D, and 3D projections with
the same authoritative state identity. Presentation interpolation may use
progress but cannot feed state back into simulation.
