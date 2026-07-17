# Authored channel-map architecture

Channel maps are typed data attached to one canonical visual frame. They are not
interchangeable grayscale images. Every map has stable identity, target frame,
kind, dimensions, color/data semantics, and content-addressed provenance.

Supported authored kinds are:

| Kind | Encoding |
|---|---|
| `material_id` | Integer material ID in R; G/B zero |
| `tangent_normal` | Tangent-space XYZ remapped to RGB; outward, near-unit vector |
| `depth` | Opaque grayscale scalar |
| `emissive` | Opaque sRGB color texture |
| `effects` | Opaque grayscale scalar |
| `collision` | Opaque binary grayscale mask |

All non-color maps use `ColorSpace::data`. Their PNG representation intentionally
omits an sRGB chunk; the visual-set declaration supplies the otherwise missing
semantic meaning. Ordinary artwork still requires explicit sRGB, so an untagged
image cannot silently cross from data into color use. Emissive is currently sRGB
because the RGBA8 pipeline does not yet provide an OCIO-backed linear authoring
transform.

Maps must match their target frame's untrimmed source dimensions. Storage alpha
is always 255 so channel meaning never depends on a renderer's alpha mode.
Tangent normals accept squared lengths within 80–120 percent of unit after
8-bit reconstruction and require nonnegative Z. Collision values are exactly 0
or 255.

Package compilation writes each validated PNG under `assets/channels/`, emits
canonical `channel-maps.json`, and records both source ingestion and encoded
artifact nodes. Each encoded map depends on the seed, source map, and target
frame. A map failure aborts the package staging transaction.

This contract governs authored maps. Learned normal/depth estimation remains a
separate governed-model pass and must preserve model identity, parameters,
validation, uncertainty, and source provenance when introduced.
