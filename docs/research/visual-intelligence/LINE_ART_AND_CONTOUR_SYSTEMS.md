# Line Art and Contour Systems

## Core question

How are contours and lines represented, controlled and kept stable over
time in stylized production?

## Findings

### F1. Lines as animatable structural assets (Spider-Verse)
- **Concept:** Imageworks rigs facial/body lines (brows, mouth corners,
  nose bridges) as deformable geometry attached to characters; animators
  pose lines alongside skeletons; ML assists predictable placements.
- **Source:** Sony Imageworks, *Into the Spider-Verse: A New Visual
  Language* (direct); Davignon et al., *Non-Photorealistic Compositing of
  the Spider-Verse*, SIGGRAPH 2023 (research).
- **GSPL implication:** linework belongs to the *semantic layer*: lines
  bind to landmarks/regions and deform with them. This is the strongest
  evidence for a LineProgram inside StyleProgram with surface-bound
  strokes.

### F2. Variable line weight (Guilty Gear)
- **Concept:** Outlines (inverted-hull + post-process thickness) modulate
  width by camera distance, bone deformation and structural importance —
  never uniform.
- **Source:** GG Strive technical-art interviews, Unreal Engine 2021
  (direct).
- **GSPL implication:** LineProgram carries per-region weight policy
  (silhouette vs inner detail vs facial), taper and depth modulation.

### F3. Object-space vs image-space contour extraction
- **Concept:** Object-space contours (silhouette/ridges from geometry
  normals) are stable but require 3D; image-space contours (edges in the
  rendered image) apply to 2D but flicker without temporal handling.
- **Source:** Hertzmann & Zorin, *Illustrating Smooth Surfaces*, SIGGRAPH
  2000 (research); DeCarlo et al., *Suggestive Contours for Conveying
  Shape*, SIGGRAPH 2003 (research).
- **GSPL implication:** since GSPL renders from *semantic geometry*, it can
  produce object-space-style contours from part boundaries — stable by
  construction, no edge detection.

### F4. Suggestive contours
- **Concept:** Lines where the surface is almost but not quite silhouetted
  — convey shape with sparse lines; extend classical contour lines.
- **Source:** DeCarlo et al., SIGGRAPH 2003 (research).
- **GSPL implication:** contour generation can select *which* part
  boundaries carry lines (silhouette canon priority), the semantic analog
  of suggestive selection.

### F5. WYSIWYG NPR stroke styling
- **Concept:** Users draw stroke styles directly on 3D models; styles
  (width, hatching, color) are authored per line, not globally.
- **Source:** Kalnins et al., *WYSIWYG NPR*, SIGGRAPH 2002 (research).
- **GSPL implication:** LineProgram entries are per-region authored
  behaviors — the typed, deterministic analog of WYSIWYG stroke styling.

### F6. Hatching and halftones
- **Concept:** Hatching (Winkenbach & Salesin, 1994) and halftone dots
  (Spider-Verse) encode tone via pattern; both are style-program behaviors.
- **Source:** Winkenbach & Salesin, *Computer-Generated Pen-and-Ink
  Illustration*, SIGGRAPH 1994 (research); Davignon et al. 2023 (research).
- **GSPL implication:** shading programs can emit hatch/halftone patterns
  deterministically from tone values.

### F7. Temporal coherence of lines
- **Concept:** Lines that pop/flicker between frames destroy the drawn
  look; coherent line tracking is an active research area (Kalnins et al.
  2003; Bénard et al., *State of the Art in Temporal Coherence for NPR*,
  Eurographics STAR 2011).
- **GSPL implication:** surface-bound semantic lines are coherent *by
  construction* — the line's identity is the landmark/region it binds to
  (TEMPORAL_COHERENCE.md).

### F8. Painterly rendering
- **Concept:** Hertzmann's brush-based painterly rendering composes
  curved brush strokes of multiple sizes; strokes are the primitive.
- **Source:** Hertzmann, *Painterly Rendering with Curved Brush Strokes of
  Multiple Sizes*, SIGGRAPH 1998 (research).
- **GSPL implication:** stroke-cluster temporal identity (future) builds
  on surface-anchored stroke semantics.

## GSPL synthesis

Lines are **semantic structures bound to regions/landmarks**, with
authored per-region weight/taper/style behavior and coherence guaranteed
by binding rather than by image processing. Contour generation consumes
the silhouette canon's declared boundaries.
