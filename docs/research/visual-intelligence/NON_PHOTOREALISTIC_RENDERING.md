# Non-Photorealistic Rendering

## Core question

How is stylized (non-photorealistic) rendering computed — contours, cel
shading, hatching, halftones, custom normals — and where do the
temporal-coherence problems live?

## Findings

### F1. Cel/toon shading = quantized lighting
- **Concept:** Lighting is quantized into discrete bands via thresholds
  (e.g., Lake et al. cartoon shader); band count and tints are artistic
  controls.
- **Source:** Lake, Marshall, Harris & Blackstein, *Stylized Rendering
  Techniques for Scalable Real-Time 3D Animation*, NPAR 2000 (research);
  Motomura GDC 2015 (direct, Guilty Gear shadow ramps).
- **GSPL implication:** ShadingProgram carries band count, ramp curve,
  terminator policy — deterministic data.

### F2. Object-space vs image-space contours
- **Concept:** Object-space (geometric silhouette/ridges, stable) vs
  image-space (pixel edges, style-flexible, flicker-prone).
- **Source:** Hertzmann & Zorin, *Illustrating Smooth Surfaces*, SIGGRAPH
  2000 (research); DeCarlo et al. 2003 (research).
- **GSPL implication:** semantic part-boundary contours are the GSPL
  analog of object-space contours — stable by construction.

### F3. Suggestive contours
- **Concept:** Lines at near-silhouette surface regions convey shape with
  fewer lines.
- **Source:** DeCarlo, Finkelstein, Rusinkiewicz & Santella, *Suggestive
  Contours for Conveying Shape*, SIGGRAPH 2003 (research).
- **GSPL implication:** contour selection follows silhouette-canon
  priority (which part boundaries carry lines) — a semantic "suggestive"
  selection.

### F4. WYSIWYG stroke styling
- **Concept:** Stroke styles (width/pattern) authored directly on models;
  authored lines attach to geometry.
- **Source:** Kalnins, Mark, Meier, Kowalski, Lee, Davidson, Webb, Hughes &
  Finkelstein, *WYSIWYG NPR*, SIGGRAPH 2002 (research).
- **GSPL implication:** LineProgram entries authored per region — typed
  per-region line behavior.

### F5. Hatching
- **Concept:** Pen-and-ink tone via strokes; direction/density encode
  tone and form.
- **Source:** Winkenbach & Salesin, *Computer-Generated Pen-and-Ink
  Illustration*, SIGGRAPH 1994 (research).
- **GSPL implication:** deterministic hatch generation is a ShadingProgram
  behavior (density from tone, direction from form/light).

### F6. Halftones (Spider-Verse)
- **Concept:** Comic dots/4-color printing artifacts simulate tone and
  depth-of-field (Davignon et al. 2023); tactile print aesthetic.
- **Source:** Davignon et al., *Non-Photorealistic Compositing of the
  Spider-Verse*, SIGGRAPH 2023 (research).
- **GSPL implication:** halftone pattern is a typed shading/FX behavior.

### F7. Artist-directed normals (Guilty Gear)
- **Concept:** Hand-tuned vertex normals place cel shadow terminators where
  a 2D artist would draw them.
- **Source:** Motomura, GDC 2015 (direct).
- **GSPL implication:** semantic shadow-terminator intent per region —
  future shading programs consume it directly.

### F8. Rim lighting
- **Concept:** Rim/backlight separates subject from background; a stylized
  edge response (already in MaterialSemantics.rim_response and
  LightingSpec.rim).
- **Source:** NPR practice; Gooch et al., *A Non-Photorealistic Lighting
  Model for Automatic Technical Illustration*, SIGGRAPH 1998 (research,
  verify).
- **GSPL implication:** rim is a resolved material+light response, applied
  deterministically.

### F9. Painterly rendering
- **Concept:** Curved brush strokes of multiple sizes compose tone
  (Hertzmann 1998); strokes are the primitive.
- **Source:** Hertzmann, *Painterly Rendering with Curved Brush Strokes of
  Multiple Sizes*, SIGGRAPH 1998 (research).
- **GSPL implication:** stroke-cluster identity (future) needs surface
  anchoring — the temporal-identity design anticipates it.

### F10. Temporal coherence of stylized rendering
- **Concept:** The STAR (Bénard et al. 2011) catalogs flicker sources and
  solutions (object-space lines, surface anchoring, hysteresis).
- **Source:** Bénard et al., Eurographics STAR 2011 (research).
- **GSPL implication:** thresholded behaviors use anchored thresholds;
  structures bind to semantics; coherence is by construction.

### F11. Stylized motion blur / speed abstraction
- **Concept:** Spider-Verse and anime replace physical blur with graphic
  abstraction (smears, ghost multiples, speed lines).
- **Source:** Davignon et al. 2023 (research); Dragon Ball practice
  (HYBRID_2D_3D_PRODUCTION.md).
- **GSPL implication:** motion abstraction is a typed FX behavior gated by
  velocity/impact intents.

## GSPL synthesis

NPR techniques are **deterministic, parameterized behaviors**: quantization
(bands), contours (part-boundary selection), lines (authored per region),
hatch/halftone patterns, custom shadow terminators, rim response and
deliberate motion abstraction. All are style-program data consumed by the
existing layered raster compiler. No image-space edge detection is
required because rendering is semantic-geometry-driven.
