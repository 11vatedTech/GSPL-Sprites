# ADR-0044: Style program factorization

## Status

Accepted.

## Decision

Style is represented as a StyleProgram of factorized behaviors — Line,
Shading, Motion, Palette, FX and Compositing programs — composed with
explicit precedence (base < project < entity < form < performance) and
lowered deterministically to the existing StyleSemantics. Labels
(clean-flat, inked, soft-shaded, pixel-constrained) exist only as presets.

## Rationale

Production evidence (Guilty Gear, Spider-Verse, Arcane) shows styles are
independently tuned behavior axes, not labels. Factorization gives
independent ownership and reuse; the documented coherence risk (an inked
line program with soft shading may clash) is accepted — the system
composes deterministically; taste remains a human review concern.

## Consequences

Style changes manifestation identity, never entity identity or material
truth. Thresholded behaviors use anchored thresholds for temporal
coherence. New style dimensions are typed additions with canonicalization
and lowering. No franchise-imitation presets.
