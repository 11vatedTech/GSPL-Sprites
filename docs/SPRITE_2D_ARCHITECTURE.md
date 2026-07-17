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

Temporal stability is measured between consecutive semantic frames per
animation, direction, and layer. The analyzer forms an overflow-checked union
canvas in pivot-relative coordinates, canonicalizes pixels below the alpha
threshold as transparent, and reports exact changed-pixel and silhouette
intersection-over-union values as integer parts per million. Visual-set policy
can reject excessive RGBA change or insufficient silhouette overlap. Accepted
measurements are embedded in `visual-projection.json`, making the decision
auditable and reproducible without floating-point tolerance drift.

`compile_sprite_sheet` composes trimming, packing, alpha/outline mask generation,
and canonical metadata. The visual package overload publishes its atlas and
masks as PNG plus `atlas.json` inside the same recoverable staging transaction
as the canonical seed and governance artifacts. Every source frame contributes
its ID, dimensions, pivot, duration, color/alpha semantics, and pixels to a
content-addressed source node. Derived visual artifacts depend on those nodes
and the seed node, and carry compiler provenance into the closed package.
The verifier reports seed identity separately from package identity (the
SHA-256 of canonical `manifest.json`), so a visual change preserves authored
semantic identity while producing a distinct compiled package identity.

Frame identifiers are semantic inputs, not inferred filenames. Direction,
animation, layer, and ordinal naming conventions are enforced by
`VISUAL_SET_SPECIFICATION.md`, whose bounded loader backs the public
`build-visual` CLI path. This pass library does not yet
claim segmentation, matting, temporal stability, layered composition,
mesh-deformation, or normal/depth/emissive map generation.
Those remain explicit gates in the master roadmap.
