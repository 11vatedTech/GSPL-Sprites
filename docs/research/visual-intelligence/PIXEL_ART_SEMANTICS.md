# Pixel Art Semantics

## Core question

What is real pixel-art construction (as opposed to nearest-neighbor
downsampling), and what typed rules would a computational pixel-art
compiler need?

## Findings

### F1. Pixel clusters are the primitive unit
- **Concept:** Pixel art is built from connected pixel clusters, not
  individual pixels; cluster economy = minimum clusters for maximum
  readability. Good clusters read as form; scattered pixels read as noise.
- **Source:** Arne, *How to Make Pixel Art* tutorial series
  (arne.graphics) (direct); Lospec pixel-art guides (direct); Michael
  Azzi, *Pixel Logic* (book, direct).
- **GSPL implication:** a true pixel compiler renders *cluster-level
  semantics*: meaningful connected regions with explicit edges, not
  quantized high-res renders.

### F2. Banding is an artifact to avoid
- **Concept:** Parallel stair-steps ("banding") at diagonal/curved edges
  read as noise; artists break banding by stepping offsets and AA.
- **Source:** Lospec/Arne tutorials (direct).
- **GSPL implication:** a pixel compiler needs banding diagnostics
  (parallel-edge detection) as a quality gate.

### F3. Jaggies and stair-stepping control
- **Concept:** Edges are stair-cased; the stair pattern itself is an
  aesthetic — smooth, controlled steps vs random jitter. Edge flow must be
  intentional.
- **Source:** pixel-art practice (direct, multiple).
- **GSPL implication:** edge-quantization policy is a typed Style/Line
  behavior (pixel stair alignment), not an accident of rounding.

### F4. Palette ramps and hue shifting
- **Concept:** Each hue needs a dark→light value ramp; ramps are built by
  hue-shifting (toward warm in light, cool in shadow) not just
  darkening/lightening.
- **Source:** Arne, *Palette Tutorial* (direct); Lospec palette guides
  (direct).
- **GSPL implication:** palette generation derives deterministic ramps
  with hue-shift curves per material/role.

### F5. Dithering
- **Concept:** Ordered (checker/bayer) or noise dithering creates
  intermediate tones/ramps between palette entries; used deliberately for
  gradients or texture, avoided on large flat surfaces where possible.
- **Source:** Lospec dithering guides (direct).
- **GSPL implication:** dithering is a deterministic ShadingProgram
  behavior with explicit pattern type and thresholds.

### F6. Selective/partial outlining
- **Concept:** Full black outlines flatten and shrink designs; selective
  outlines (outer contour + some internal separation, colored or partial)
  keep readability without heaviness.
- **Source:** pixel-art practice (direct, multiple).
- **GSPL implication:** OutlineSelection already exists in style semantics;
  pixel styles can apply selective outlines from silhouette canon
  priorities.

### F7. Anti-aliasing in pixel art
- **Concept:** Manual AA (intermediate colors at edges) is a craft; AA
  against transparent backgrounds (halo avoidance) is a distinct problem.
  Some styles forbid AA entirely.
- **Source:** Arne/Lospec AA tutorials (direct).
- **GSPL implication:** AA policy is per-style (already in
  AntiAliasPolicy); pixel styles may forbid it; when allowed, AA colors
  must derive from palette ramps.

### F8. Readability at scale (16x16 vs 32x32)
- **Concept:** Different features survive at different resolutions; design
  must know which features are load-bearing at each scale.
- **Source:** Pokémon sprite practice (CREATURE_DESIGN_SYSTEMS.md);
  pixel-art practice (direct).
- **GSPL implication:** ResolutionCanon feature priority feeds the pixel
  compiler: what merges/omits/substitutes at each resolution.

### F9. Sprite animation: subpixel motion
- **Concept:** Moving 1px per frame is the finest quantized step, but
  smooth motion at fractional speeds uses *subpixel accumulation*
  (carrying remainder offsets) so the sprite itself stays on integer
  pixels while motion is smooth.
- **Source:** pixel-art animation tutorials (e.g., Michael Azzi; Lospec
  animation guides) (direct).
- **GSPL implication:** subpixel motion is a deterministic accumulation
  policy at the pixel-realizer layer.

### F10. Temporal cluster stability
- **Concept:** Clusters must persist (same shape/position relation) across
  frames to avoid shimmer/flicker; motion is expressed by cluster
  transformation, not cluster noise.
- **Source:** pixel-art animation practice (direct); temporal coherence
  literature (TEMPORAL_COHERENCE.md).
- **GSPL implication:** temporal cluster identity is the pixel analog of
  surface-bound marking identity; the same TemporalVisualIdentity system
  applies at cluster granularity.

### F11. Downsampling ≠ pixel art
- **Concept:** Nearest-neighbor downsampling of high-res art produces
  arbitrary aliasing with no intentional clusters, ramps or stair control;
  it is the antithesis of constructed pixel art.
- **Source:** practitioner consensus (Arne, Lospec, Pixel Joint) (direct).
- **GSPL implication:** the current pixel-constrained backend is
  *quantization-level* — honestly documented as such. A true pixel
  compiler (cluster-semantic) is future work, but the architecture
  (semantic shapes → typed style behaviors → deterministic quantized
  realization) already permits it without redesign.

### F12. Scaling algorithms (xBR, hq2x) are upscaling conveniences
- **Concept:** These interpolate existing pixel art to higher resolutions
  with edge-aware sharpness; they are not construction tools.
- **Source:** tech literature (verify specifics); practitioner usage.
- **GSPL implication:** scaling policy is presentation, not identity.

## GSPL synthesis

Real pixel art is **cluster construction with ramps, controlled edges,
deliberate dither/AA policy and temporal cluster stability** — all
semantic rules a compiler can enforce. The current backend is honestly a
quantization level; the architecture must not close the door to a
cluster-semantic pixel compiler (integer grid, palette limits, cluster
connectivity, temporal persistence).
