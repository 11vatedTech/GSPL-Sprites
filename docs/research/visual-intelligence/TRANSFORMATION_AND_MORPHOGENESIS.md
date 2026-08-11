# Transformation and Morphogenesis

## Core question

How do entities change form while preserving lineage and identity — and
how is that structured as a trajectory rather than a crossfade?

## Findings

### F1. Additive evolutionary design (Pokémon)
- **Concept:** Evolved forms add functional armor/growth/focal features
  while retaining core proportions — visual lineage preserved, not
  replaced.
- **Source:** IGN, *Game Freak Talks Evolution of Pokémon Designs*, 2017
  (direct).
- **GSPL implication:** transformation canon = identity invariants (must
  survive) + additive deltas + proportional shifts; intermediate states
  are first-class.

### F2. Escalation staging (Dragon Ball)
- **Concept:** Power transformations escalate through staged visual
  signals: aura, palette shift, silhouette change, pose language, intensity
  of FX — a directed trajectory, not a morph.
- **Source:** Dragon Ball production analysis
  (HYBRID_2D_3D_PRODUCTION.md); anime practice.
- **GSPL implication:** transformation trajectories carry *escalation
  parameters* (aura intensity, palette shift curve, structure growth)
  evaluated deterministically at each phase.

### F3. Identity anchors under deformation (One Piece)
- **Concept:** Under wild deformation, signature anchors (hats, scars,
  palette, landmark topology) stay rigidly fixed, preserving identity.
- **Source:** One Piece production analysis
  (HYBRID_2D_3D_PRODUCTION.md).
- **GSPL implication:** hard identity boundaries in deformation envelopes
  are *exactly* this practice made typed: per-structure invariants that
  survive any transformation.

### F4. Structured morphing vs image crossfade
- **Concept:** Academic shape morphing interpolates structure (ARAP
  interpolation, correspondence maps); production metamorphosis tracks
  semantic regions through intermediate states.
- **Source:** Alexa, Cohen-Or & Levin, *ARAP Shape Interpolation*,
  SIGGRAPH 2000 (research); Sumner & Popović, *Deformation Transfer*,
  SIGGRAPH 2004 (research).
- **GSPL implication:** intermediate states are generated from canonical
  structure + trajectory parameters (proportion shifts, material/color
  curves, marking evolution) — never by blending rendered images.

### F5. Storm-form semantics (owned reference)
- **Concept:** Voltfox base↔storm is our first owned transformation:
  palette shift (storm_primary_color), eye/emission role escalation,
  silhouette/marking deltas, aura escalation.
- **GSPL implication:** expressed as canon transformation trajectory data,
  not code; any entity gets the same machinery.

## GSPL synthesis

Transformation is a **semantic trajectory**: identity invariants (fixed)
+ additive deltas + proportional/materials/color/marking trajectories +
escalation parameters. Intermediate states are first-class compiled
structures. The renderer never crossfades images; it renders the compiled
state.
