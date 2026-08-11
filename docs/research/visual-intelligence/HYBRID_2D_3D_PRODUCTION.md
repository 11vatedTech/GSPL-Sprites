# Hybrid 2D/3D Production

## Core question

How do studios blend 2D aesthetics with 3D production, and what are the
generalizable technical principles (not the styles)?

## Findings

### F1. Guilty Gear Xrd / Strive — cel shading with artist-directed normals
- **Decision:** Vertex normals are hand-tuned so cel shadow terminators
  land where a 2D animator would draw them; lighting geometry is decoupled
  from physical normals.
- **Principle:** Treat normals/shadow placement as artistic control
  vectors; shadow ramps use 1-3 band textures, eliminating smooth
  Lambert gradients.
- **GSPL implication:** shadow-terminator intent is semantic per region;
  band count/tint is data (ShadingProgram).
- **Source:** Motomura, *Guilty Gear Xrd's Art Style*, GDC 2015 (direct);
  GG Strive technical-art interviews, Unreal Engine 2021 (direct).

### F2. Guilty Gear — limited/controlled 3D posing
- **Decision:** During key poses, the 3D model dynamically morphs limb
  lengths and proportions to match 2D orthogonal proportions from the
  active camera.
- **Principle:** Camera-space visual fidelity can override rig anatomical
  consistency; 2D drawing weight is preserved per viewpoint.
- **GSPL implication:** projection-dependent interpretation is a declared
  concept (VISUAL_INTELLIGENCE_THEORY.md): identity (relational) survives,
  projection changes proportions deterministically.
- **Source:** Motomura GDC 2015 (direct); Game Developer coverage (direct).

### F3. Guilty Gear — outline control
- **Decision:** Inverted-hull outlines + post-process thickness scaling;
  width modulates by distance, deformation and line importance.
- **Principle:** Variable, importance-weighted line weight (never uniform).
- **GSPL implication:** LineProgram per-region weight policy.
- **Source:** GG Strive UE interviews 2021 (direct).

### F4. Spider-Verse — lines as rigged assets
- **Decision:** Facial/body lines drawn on geometry, rigged and animated
  with the skeleton.
- **Principle:** Linework is animatable structure, not post-process.
- **GSPL implication:** line binding to landmarks/regions
  (LINE_ART_AND_CONTOUR_SYSTEMS.md).
- **Source:** Sony Imageworks, *Into the Spider-Verse: A New Visual
  Language* (direct).

### F5. Spider-Verse — stepped animation vs smooth camera
- **Decision:** Characters animated on twos/fours; camera stays 24fps
  smooth.
- **Principle:** Decouple character temporal frequency from camera;
  illustrative cadence without motion sickness.
- **GSPL implication:** frame cadence is a typed performance/style
  behavior; runtime tick model already supports it.
- **Source:** Imageworks craft docs 2018 (direct).

### F6. Spider-Verse — smears, ghosting, speed abstraction
- **Decision:** Hand-drawn smear geometry, multi-limb ghosting and speed
  lines replace photographic motion blur.
- **Principle:** Graphic abstraction preserves silhouette clarity under
  speed; physical blur reads wrong in a graphic style.
- **GSPL implication:** motion abstraction is a typed FX behavior gated by
  velocity/impact intents.
- **Source:** Davignon et al., *Non-Photorealistic Compositing of the
  Spider-Verse*, SIGGRAPH 2023 (research).

### F7. Spider-Verse — halftones and print artifacts
- **Decision:** Halftone dots, chromatic mis-registration and offset
  printing artifacts simulate depth-of-field and texture.
- **Principle:** Analog print flaws become procedural shader parameters.
- **GSPL implication:** halftone/registration are typed shading/FX
  behaviors.
- **Source:** Davignon et al. 2023 (research).

### F8. Spider-Verse — hand-drawn FX in 3D space
- **Decision:** A reusable library of 2D hand-drawn explosions/fire/smoke
  composited in 3D space.
- **Principle:** Hybridize hand-drawn frame-by-frame FX with procedural
  data.
- **GSPL implication:** FX semantics (fx_semantics) carry phenomenon facts;
  style programs interpret them as hand-drawn or procedural.
- **Source:** Imageworks FX documentation 2018 (direct).

### F9. Arcane — painterly materials and non-physical lighting
- **Decision:** Hand-painted texture workflows (visible brushstrokes,
  coarse gradients, stylized speculars) plus localized non-realistic
  light rigs with heavy rim and color-bleed.
- **Principle:** Materials carry tactile canvas quality; light paints
  hierarchy, not physics.
- **GSPL implication:** material truth separated from artistic response;
  LightingSpec is semantic intent.
- **Source:** RedShark technical feature 2024 (direct); Fortiche
  interviews 2021-2022 (direct).

### F10. Arcane — bidirectional 2D/3D pipeline
- **Decision:** 3D layout/blockout validated by 2D paint-overs; final
  composite blends 3D with matte paintings.
- **Principle:** Technical 3D never overrides artistic 2D vision.
- **GSPL implication:** the canonical semantic layer is the authority;
  renderers are projections (matches GSPL's core principle).
- **Source:** Cartoon Brew, *The Secret to Fortiche's Success*, 2022
  (direct).

### F11. Dragon Ball — action staging and anticipation
- **Decision:** Extreme compressed anticipation frames (squash + muscle
  contraction) launch instantaneous multi-frame skips; diagonal graphic
  staging; single-frame impact abstracts (starbursts, inverted palettes).
- **Principle:** Force is expressed through deformation + timing; impact
  frames deliberately break temporal continuity.
- **GSPL implication:** force-aware deformation envelopes; typed impact
  frames; phase model includes extreme compression.
- **Source:** animation-principles analyses (verify); anime production
  analysis literature (research interpretation).

### F12. Dragon Ball — aura and power escalation
- **Decision:** Procedural aura particle systems deform geometrically with
  emotional escalation; speed abstraction via streak lines and ghost
  trails.
- **Principle:** Power state is encoded into FX geometry; escalation is a
  directed trajectory.
- **GSPL implication:** transformation trajectories carry escalation
  parameters (aura intensity, palette shift, structure growth).
- **Source:** production analysis (research interpretation).

### F13. One Piece — diversity, elasticity, identity anchors
- **Decision:** Extreme proportional variation across the cast; elastic
  rubber-hose deformation without volume conservation in gags; signature
  anchors (hats, scars, colors) stay rigidly fixed under wild deformation.
- **Principle:** Identity survives deformation when anchors persist;
  silhouette contrast differentiates the cast.
- **GSPL implication:** hard identity boundaries (deformation envelopes)
  are exactly the anchor practice made typed.
- **Source:** production analyses 2023-2025 (research interpretation);
  ScreenRant/Full Frontal analyses (interpretation).

### F14. Pokémon — readability, silhouette, palette economy
- **Decision:** Solid-black silhouette test; ~3-hue palettes; additive
  evolution; readability at sprite scale.
- **Principle:** silhouette and landmark topology carry identity at every
  resolution.
- **GSPL implication:** silhouette canon + resolution canon + palette caps.
- **Source:** Game Informer 2017 (direct); IGN 2017 (direct); Sugimori
  interviews (direct).

## GSPL synthesis

The productions studied converge on the same architecture: **a semantic
authority above rendering**; style as a set of parameterized behaviors;
identity anchored in invariants; deformation bounded by anchors; motion
abstraction deliberate; projection separated from truth. GSPL's existing
authority chain (CanonicalEntity → SpriteIr → VisualIr → raster) is
already the correct skeleton; the research fills in the semantic layers
above the raster.
