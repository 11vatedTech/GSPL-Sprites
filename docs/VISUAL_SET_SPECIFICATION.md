# Authored visual-set specification 0.1

An authored visual set is the strict user-facing boundary between source PNG
frames and deterministic 2D compilation. Invoke it with:

```text
gspl-sprites build-visual <seed.sprite> <visual-set.txt> <output-directory>
```

The manifest is UTF-8-compatible ASCII `key=value` text. Blank lines and lines
whose first non-whitespace character is `#` are ignored. Scalar fields occur
exactly once; `frame` is repeatable.

```text
schema=gspl.visual-set/0.1
rights=ORIGINAL_USER_CREATION
max_width=2048
max_height=2048
padding=2
trim=true
alpha_threshold=0
pixel_art=true
grid_width=1
grid_height=1
maximum_colors=16
binary_alpha=true
temporal_max_changed_per_million=350000
temporal_min_silhouette_iou_per_million=700000
layer=shadow
layer=body
layer=effects
frame=idle|south|shadow|0|frames/idle-south-shadow-0.png|32|58|6
frame=idle|south|body|0|frames/idle-south-body-0.png|32|58|6
frame=idle|south|effects|0|frames/idle-south-effects-0.png|32|58|6
map=collision|idle|south|body|0|maps/idle-south-body-0-collision.png
```

Frame fields are:

```text
animation|direction|layer|ordinal|relative_png_path|pivot_x|pivot_y|duration_ticks
```

Animation and layer identifiers use lowercase ASCII letters, digits, `_`, and
`-`. Directions are `none`, the four cardinal directions, or the four diagonal
forms such as `north_east`. Ordinals are `0..9999`. The compiler derives a
canonical identifier such as `idle.south.body.0000`; duplicate semantic
identities are rejected.

`layer` declarations define back-to-front ordering and are repeatable. Every
animation/direction/ordinal tuple must contain each declared layer exactly once.
Layers in that tuple must share source canvas dimensions, pivot, and duration;
ordinals for each animation/direction sequence must be contiguous from zero.
The package records this typed structure in canonical `visual-projection.json`.

When `pixel_art=true`, every source must satisfy the declared grid, palette, and
binary-alpha policy. The union of colors across all layers, directions, and
frames must also fit `maximum_colors`, preventing individually valid images from
silently violating the set's fixed palette.

Temporal analysis compares consecutive ordinals independently for each
animation, direction, and layer. Pixels are aligned in pivot-relative space, so
trimmed dimensions do not create false motion. `temporal_max_changed_per_million`
bounds exact RGBA changes; `temporal_min_silhouette_iou_per_million` bounds
foreground silhouette overlap. Both use integer parts-per-million values in
`0..1000000`. Every transition's measured values are preserved in
`visual-projection.json` even when thresholds are permissive.

Optional channel maps use:

```text
map=kind|animation|direction|layer|ordinal|relative_png_path
```

Kinds are `material_id`, `tangent_normal`, `depth`, `emissive`, `effects`, and
`collision`. Every map targets an existing semantic frame and must share its
dimensions. Material IDs store an integer in R with G/B zero. Tangent normals
must face outward and have near-unit length. Depth and effects are grayscale;
collision is binary grayscale. All use opaque storage alpha. Emissive maps are
explicit sRGB color textures. Every other kind is an explicitly declared data
texture and its PNG must omit the sRGB chunk. Valid maps are packaged under
`assets/channels/` with canonical `channel-maps.json` and provenance.

Paths are relative to the manifest directory. Absolute paths, drive syntax,
backslashes, empty/dot/parent components, control/non-ASCII bytes, missing
files, non-regular files, and symlink traversal are rejected. PNG decoding uses
the bounded production adapter and requires an explicit sRGB chunk.

The loader bounds manifest bytes, lines, frame count, each encoded image, each
decoded image, aggregate encoded bytes, aggregate decoded bytes, and final atlas
resources. The entire sheet is compiled once during admission so an invalid set
fails before package staging begins.

Version 0.1 intentionally admits only `ORIGINAL_USER_CREATION`. This prevents a
single top-level declaration from laundering mixed reference rights. Per-source
rights and license records will be introduced with the multi-source reference
manifest; unsupported classifications fail rather than being silently widened.
