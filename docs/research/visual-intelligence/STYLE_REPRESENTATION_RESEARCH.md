# Style Representation Research

## Core question

Can style be represented as typed, composable *behaviors* (not labels),
and what does the evidence say about factorizing it?

## Findings

### F1. Style labels are not style
- **Concept:** "Anime", "pixel art", "realistic" name clusters of
  behaviors; they cannot drive a compiler. The behaviors are: line
  representation, shading quantization, palette policy, motion cadence,
  edge treatment, compositing.
- **Evidence:** every production studied separates these axes and tunes
  them independently (Guilty Gear shadow ramps vs outlines; Spider-Verse
  stepping vs linework; Arcane materials vs light).
- **GSPL implication:** StyleSemantics (already implemented) is a vector of
  behaviors; labels exist only as presets (make_style_preset). The
  research confirms this design.

### F2. Factorized style programs
- **Concept:** Decompose style into LineProgram, ShadingProgram,
  MotionProgram, PaletteProgram, FxProgram, CompositingProgram.
- **Evidence for:** independent tunability (each production tuned axes
  separately); reuse (a shading ramp from one, line weights from another);
  clarity of ownership.
- **Evidence against / risk:** incoherent combinations — an inked line
  program with a soft-shaded lighting program may clash. Professional
  taste still filters combinations.
- **Source:** production case studies (HYBRID_2D_3D_PRODUCTION.md);
  NPR STAR (Bénard et al. 2011); NPR literature.
- **GSPL implication:** implement factorization *as a container of
  programs that lowers to the existing effective StyleSemantics*; document
  the coherence risk; precedence remains explicit (base < project <
  entity < form < performance).

### F3. Style transfer is about behaviors, not textures
- **Concept:** Academic style transfer operates on statistics/textures;
  production style is about *structural behaviors* (how lines, tones and
  motion are built).
- **Source:** style-transfer literature (verify specific papers);
  production evidence (F1).
- **GSPL implication:** the StyleProgram is behavior-first; a future
  learned specialist proposes behavior parameter patches, validated and
  compiled deterministically.

### F4. Per-region style
- **Concept:** WYSIWYG NPR and production both apply style *per region*
  (facial lines vs armor outlines), not globally.
- **Source:** Kalnins et al., WYSIWYG NPR, SIGGRAPH 2002 (research);
  Guilty Gear outline importance weighting (direct).
- **GSPL implication:** style patches can scope to regions; the canon's
  structures are the scoping unit.

### F5. Style must preserve identity invariants
- **Concept:** Restyling changes permitted visual properties but must not
  alter semantic identity (relational invariants, landmark topology,
  material truth).
- **Source:** corpus-wide (model sheets, silhouette, mascot/icon).
- **GSPL implication:** style changes manifestation identity but never
  entity identity — already the existing VisualIr identity contract.

### F6. Determinism and temporality of style
- **Concept:** Thresholded style behaviors must be temporally stable
  (anchored thresholds, hysteresis) to avoid flicker.
- **Source:** NPR temporal-coherence STAR 2011 (research).
- **GSPL implication:** any new thresholded behavior uses anchored
  thresholds.

## GSPL synthesis

Style = **typed, scoped, composable behaviors with explicit precedence**,
lowered deterministically to an effective style consumed by the renderer.
Factorization is adopted with documented coherence risk; labels remain
presets; identity invariants are never style-visible.
