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
frame=idle|south|body|0|frames/idle-south-0.png|32|58|6
frame=idle|south|body|1|frames/idle-south-1.png|32|58|6
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
