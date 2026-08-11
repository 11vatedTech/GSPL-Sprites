# Visual Fidelity Evaluation

## Core question

How is visual quality measured objectively — as enforceable lower bounds —
without claiming to replace artistic judgment?

## Findings

### F1. Recognition from silhouettes (recognition-by-components)
- **Concept:** Objects are recognized by decomposing silhouette features
  into volumetric primitives (geons) and their spatial relations.
- **Source:** Biederman, *Recognition-by-Components*, Psychological Review
  94(2), 1987 (research).
- **GSPL implication:** silhouette metrics (occupancy, connected
  components, anchor alignment, negative-space preservation) are
  *recognition-relevant*, not arbitrary.
- **Directly implemented in:** visual_metrics (SilhouetteStats).

### F2. Gestalt grouping as diagnostics
- **Concept:** Proximity/similarity/closure/continuation govern grouping;
  violations (accidental splits, merged limbs) are measurable.
- **GSPL implication:** connected-component analysis, color-grouping
  consistency and contour-continuity checks are valid diagnostics.
- **Source:** Gestalt literature; IxDF synthesis.

### F3. Shape descriptors for matching
- **Concept:** Shape contexts and similar descriptors match shapes by
  landmark correspondence; they quantify shape distance.
- **Source:** Belongie, Malik & Puzicha, *Shape Matching and Object
  Recognition Using Shape Contexts*, IEEE TPAMI 2002 (research, verify).
- **GSPL implication:** cross-form/cross-entity shape distance is a
  measurable diagnostic (frame similarity/distinction already exist).

### F4. Image quality assessment (SSIM family)
- **Concept:** SSIM/MS-SSIM measure structural similarity perceptually
  better than PSNR; modern learned metrics improve further.
- **Source:** Wang, Bovik, Sheikh & Simoncelli, *Image Quality Assessment:
  From Error Visibility to Structural Similarity*, IEEE TIP 13(4), 2004
  (research).
- **GSPL implication:** deterministic structural similarity (existing
  frame_similarity) is the right class of metric for regression testing —
  byte hashes alone catch nothing semantic.

### F5. Perceptual simplification and multiscale representation
- **Concept:** Vision builds multiscale descriptions (Marr's primal
  sketch); identity survives scale when signature structure survives.
- **Source:** Marr, *Vision*, 1982 (research).
- **GSPL implication:** resolution canon is perception-grounded: feature
  importance + minimum resolution + substitution rules.

### F6. Visual saliency
- **Concept:** Saliency models (Itti-Koch) predict attentional priority;
  salient features carry recognition weight.
- **Source:** Itti, Koch & Niebur, *A Model of Saliency-Based Visual
  Attention*, IEEE TPAMI 20(11), 1998 (research).
- **GSPL implication:** landmark/feature priority (recognition_importance)
  is the semantic analog; fidelity diagnostics weight landmark drift by
  importance.

### F7. Iconicity
- **Concept:** Simplified/iconic representations retain recognition when
  signature structure survives.
- **GSPL implication:** LOD substitution rules preserve signature structure
  (silhouette anchors, landmark topology, two-tone contrast).
- **Source:** mascot/icon practice (CHARACTER_DESIGN_AND_MODEL_SHEETS.md).

### F8. Metrics are lower bounds, not excellence
- **Concept (research interpretation + production evidence):** no metric
  proves artistic quality; metrics *enforce* minimal structural
  conditions (silhouette validity, palette validity, landmark stability,
  temporal stability).
- **GSPL implication:** FidelityReport + canon-level diagnostics are gates;
  human inspection remains the quality authority. This is the honest
  position the corpus demands.

### F9. Character recognizability at small scale
- **Concept:** thumbnails/16x16 legibility is the standard production
  test; only macro structure survives.
- **GSPL implication:** resolution tests render the same canon at multiple
  sizes and require stable silhouette metrics.
- **Source:** Pokemon/icon practice (CREATURE_DESIGN_SYSTEMS.md).

## GSPL synthesis

Fidelity evaluation = **deterministic structural diagnostics over
semantic inputs** (canon validity, landmark ratio drift, silhouette
stability, palette adherence, temporal stability, frame distinction) +
**human review artifacts**. Metrics enforce lower bounds; they never
pretend to be an aesthetic judge. (Directly implemented in visual_metrics;
extended by the new visual_quality diagnostics.)
