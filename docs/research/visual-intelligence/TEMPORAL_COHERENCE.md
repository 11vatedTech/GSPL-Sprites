# Temporal Coherence

## Core question

How do persistent visual structures (contours, markings, highlights,
strokes, clusters, landmarks) keep stable identity across frames — and
when is incoherence *deliberate*?

## Findings

### F1. Coherence is structure identity over time
- **Concept:** The same semantic structure (a contour segment, a marking,
  a highlight, a stroke cluster, a landmark) must map to itself from frame
  to frame; independent per-frame generation redefines structures and
  reads as flicker.
- **Source:** Bénard, P., Bousseau, A., Thollot, J. et al., *State of the
  Art in Temporal Coherence for Non-Photorealistic Rendering*.
  Eurographics STAR, 2011 (research, direct).
- **GSPL implication:** every persistent visual structure carries a stable
  semantic ID assigned by the canon/compiler, not by the renderer.

### F2. Surface anchoring
- **Concept:** Features defined in surface/semantic space (UV, region,
  landmark frame) move *with* the surface; coherence follows automatically.
- **Source:** temporal-coherence STAR 2011 (research); Spider-Verse line
  rigging (LINE_ART_AND_CONTOUR_SYSTEMS.md).
- **GSPL implication:** markings/lines/highlights bind to canon structures
  (already true for markings: part_id binding). Highlight and contour
  structures should bind the same way.

### F3. Line and stroke correspondence
- **Concept:** Object-space lines from tracked geometry stay coherent;
  image-space lines need correspondence solving. WYSIWYG stroke styling
  (Kalnins 2002) keeps authored lines attached to the model.
- **Source:** Kalnins et al., *WYSIWYG NPR*, SIGGRAPH 2002 (research);
  Hertzmann & Zorin 2000 (research).
- **GSPL implication:** since rendering is semantic-geometry-driven, line
  identity = region/landmark binding. No image-space correspondence solver
  needed.

### F4. Deliberate incoherence: stepped animation and smears
- **Concept:** Spider-Verse animates characters on twos/fours while the
  camera stays smooth; smear frames intentionally violate coherence to
  signal speed. Incoherence is *authored*, never accidental.
- **Source:** Sony Imageworks craft documentation, 2018 (direct);
  Davignon et al., SIGGRAPH 2023 (research).
- **GSPL implication:** stepped frame cadence and smear requests are typed
  performance/style behaviors, applied deliberately and deterministically.

### F5. Temporal cluster stability in pixel art
- **Concept:** Clusters persist across frames (same shape/position);
  motion transforms clusters rather than regenerating them.
- **Source:** pixel-art animation practice (PIXEL_ART_SEMANTICS.md).
- **GSPL implication:** same TemporalVisualIdentity machinery at cluster
  granularity for future pixel compilation.

### F6. Surface-space features (UV/curve correspondence)
- **Concept:** Markings, highlights and material regions live in
  surface/parameter space so deformation carries them along.
- **Source:** NPR STAR 2011 (research); VFX texture-space practice.
- **GSPL implication:** markings already bind to parts (local frame);
  future surface parameterization extends this to arbitrary
  surface-bound features.

### F7. Flicker sources and mitigation
- **Concept:** Hatching/hatch density, halftones and AA thresholds produce
  flicker when parameters cross quantization boundaries; mitigation is
  hysteresis/anchoring.
- **Source:** NPR temporal-coherence literature (STAR 2011).
- **GSPL implication:** any thresholded style behavior (shadow bands,
  dither, AA) should use deterministic anchored thresholds to avoid
  crossing-boundary flicker.

## GSPL synthesis

Temporal identity is **semantic structure identity**: the canon assigns
stable IDs to persistent structures; binding (to regions/landmarks/
surfaces) provides coherence by construction; and incoherence (stepped
motion, smears) is a typed, deliberate behavior. This is the
TEMPORAL_VISUAL_IDENTITY.md architecture in miniature.
