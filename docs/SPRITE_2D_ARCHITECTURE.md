# Deterministic 2D compilation architecture

The foundational 2D pass library operates only on bounded RGBA8 images with an
explicit color space and straight or opaque alpha. Its current compiler-grade
operations are transparent trimming with pivot correction, integer
nearest-neighbor scaling, canonical palette extraction, fixed-palette remapping,
binary alpha and four-neighbor outline masks, pixel-art policy validation,
deterministic atlas packing, and canonical atlas metadata.

Determinism is structural and artifact-level. Palette frequency ties resolve by
packed RGBA value. Nearest-neighbor scaling performs no interpolation. Fully
transparent RGB is canonicalized during palette remapping. Atlas metadata sorts
by stable frame identifier and validates every placement with overflow-safe
bounds. Pixel-art validation admits at most 256 colors, checks declared grid
alignment, and can forbid fractional alpha without allocating per-pixel
diagnostics.

Frame identifiers are semantic inputs, not inferred filenames. Direction,
animation, and ordinal naming conventions will be enforced by the authored
visual-set schema in the next integration layer. This pass library does not yet
claim segmentation, matting, temporal stability, layered composition,
mesh-deformation, normal/depth/emissive map generation, or package integration.
Those remain explicit gates in the master roadmap.
