# GSPL Sprites — Native Visual Core Architecture

**Doctrine:** Pixels are a deterministic manifestation of meaning. The renderer
never rediscovers semantic meaning; the Visual IR already describes what should
exist. Learned systems may later contribute proposals or transforms, but the
GSPL semantic graph remains authoritative.

This document governs the Native Visual Core phase. It covers Visual Morphology
v2, Style Semantics, Material Semantics, Marking/Pattern Semantics, Palette
Semantics, Performance Semantics, the Canonical Visual IR, the native layered
raster compiler, and the deterministic Visual Fidelity Metrics.

## Authority chain

```
CanonicalEntity (semantic authority)
      |
      v
Production SpriteIr (gspl::sprites::SpriteIr — canonical production IR)
      |
      v
Visual Semantic Compiler (compile_visual_ir)
      |
      v
Canonical Visual IR (gspl::sprites::visual::VisualIr)
      |
      v
Native Layered Raster Compiler (render_visual_ir)
      |
      v
ImageRgba8 / semantic channels
```

- `CanonicalEntity` remains the **sole entity truth**. There is no `VisualEntity`,
  `VisualSeed`, `GraphicEntity`, or `RenderEntity`; the visual system is a
  **derived semantic manifestation description**.
- The Visual IR is derived per `(entity, projection context, style context,
  performance context)`. It carries entity identity references only; it never
  re-declares entity semantics.
- The raster compiler consumes only the Visual IR. It contains no
  `if style == "anime"` / `if entity == "voltfox"` / material-name special cases.
  Style/material decisions belong to semantic compilation.

## Visual Morphology v2 (`visual_morphology.hpp/.cpp`)

`VisualMorphologyV2` is an evolved, shape-language-capable morphology. It
supports, per part: semantic part identity, hierarchical attachment, spatial
transform (x/y/z, rotation, size), depth ordering (z_order), semantic role,
primitive shape, surface/region intent, material assignment, color role,
marking binding (marking_ids), silhouette contribution, visibility rules, layer
assignment, articulation/attachment anchors, bone/socket binding, and
projection behavior. No fields are hardcoded around Voltfox anatomy.

- **Compatibility:** v1 `MorphologyPart` morphology is preserved as
  schema-compatible v1. `lower_visual_morphology_v1` is the deterministic
  compatibility path (v1 to v2); the v1 `"root"` parent sentinel normalizes to
  empty, and v1 primitive classes map onto the v2 shape vocabulary.
- **Validation:** `validate_visual_morphology` fails closed on duplicate part
  ids (id must equal its map key), missing parent, cycles, invalid layer refs,
  invalid material/color refs, nonfinite geometry, negative/invalid dimensions,
  and dangling bindings.
- **Canonicalization/identity:** `canonicalize_visual_morphology` and
  `visual_morphology_identity` are deterministic; identity is part of the
  Visual Manifestation Identity taxonomy (see `docs/IDENTITY_TAXONOMY.md`).
## Shape semantics (`visual_geometry.hpp/.cpp`)

The v1 primitive vocabulary is generalized into a native shape system:
`ellipse`, `capsule`, `rounded_rect`, `polygon`, `ring`, `open_path`,
`closed_path` (Bezier paths), with `segmented_curve` lowering to open paths and
`aura_contour` lowering to rings. Semantic shape roles (body-mass, limb,
appendage, facial-feature, eye, energy-aura, marking, outline, ...) are
extensible data, not a fixed taxonomy.

- Deterministic Bezier flattening (`flatten_path`): bounded error tolerance,
  finite-input rejection (nonfinite points fail closed), bounded subdivision
  (max recursion/segments), no unbounded recursion.
- Deterministic polygon filling: even-odd rule, bounded raster work budget,
  safe clipping.
- Analytic anti-aliasing (coverage from signed distance to boundary); the
  `AntiAliasPolicy::none` path forces hard edges for pixel-art compilation.
- High-quality strokes: width, taper (`interpolate_width`), joins/caps via
  polyline distance fields.
- Deterministic geometry helpers: `ellipse_polyline`, `capsule_polyline`,
  `rounded_rect_polyline`, `closest_on_closed_polyline`,
  `distance_point_segment`, `point_in_polygon_even_odd`.

## Style semantics (`style.hpp/.cpp`)

Style is a deterministic description of **visual behavior**, not a label.
Preset labels (`clean-flat`, `inked`, `soft-shaded`, `pixel-constrained`)
exist only as `make_style_preset`. The authoritative structure includes: shape
abstraction, proportion exaggeration, contour behavior, line weight/taper,
outline selection, corner sharpness, curve smoothness, palette policy,
value grouping, saturation behavior, shadow model, highlight model, texture/
detail density, edge softness, anti-aliasing policy, pixel quantization, and
layer compositing policy.

- **Composition/precedence** (`compose_style`): base style grammar < project
  < entity < form < performance. No global mutable style state.
- **Renderer wiring** (`render_visual_ir`): the raster consumes the resolved
  style — outline selection/line weight drive the silhouette outline pass;
  `highlight_model` drives a light-axis highlight term; `saturation_behavior`
  is a luminance-preserving saturation scale; `value_grouping` posterizes
  channels; `palette_policy == quantize` posterizes final RGB via
  `quantize_color` (post-shading, the correct pixel-art behavior);
  `shadow_model` modulates drop-shadow alpha.
- **Validation/canonicalization:** `validate_style`, `canonicalize_style`,
  `canonicalize_style_patch`, `style_identity`.

## Palette semantics (`palette.hpp/.cpp`)

The single color authority. Colors are packed `0xRRGGBBAA`. `PaletteDefinition`
maps semantic color roles (primary, secondary, accent, eye, emission, shadow,
highlight, outline, effect, ...) to concrete colors; explicit entity hex colors
remain compatible through the `custom` role. Deterministic generation:
`make_default_palette`, `make_transformed_palette`; `parse_hex_color`
(single-source hex parser), `palette_color`, `mix_colors`, `scale_rgb`,
`quantize_color`, `color_luminance`, `with_alpha_value`. `validate_palette`,
`canonicalize_palette`, `palette_identity`.

## Material semantics (`material.hpp/.cpp`)

`MaterialSemantics` encodes **what the surface visually is**, renderer-
independent: material class, base color role, roughness-like response,
reflectivity, metallicity, translucency, emission, surface texture scale,
anisotropy intent, edge response, rim-light response, subsurface intent.
Material classes (`skin`, `glass`, `electricity`, `metal`, `cloth`, `fur`, ...)
are data (`make_material`), resolved via `resolve_material(material, style)`.
The raster's `shade_pixel` produces deterministic base/shadow/highlight/rim
responses per material and a deterministic 2D lighting model (`LightingSpec`:
light direction, ambient/key/fill/rim, drop shadow). This is a baseline
material-response stage — not photorealism, and not a switch on entity names.

## Marking / pattern semantics (`markings.hpp/.cpp`)

`Marking` generalizes Voltfox's proven markings into a data-driven system:
`MarkingKind` (electrical, stripe, spot, symbol, circuit, ...), color role,
part binding, geometry, z-order, opacity. A marking binds to a semantic
surface/part (`part_id` + `marking_ids`) and is masked by the part geometry at
render time; it is never baked into arbitrary pixels. `validate_marking`,
`canonicalize_marking`.

## Performance semantics (`performance.hpp/.cpp`)

The foundation for realizing a pose/performance state visually: `PartMotion`
(per-part dx/dy/rotation/scale/opacity), `PerformanceState` (motion_phase,
action_phase, velocity intent, facing, expression intent, impact/emphasis,
secondary-motion state). The compiler bakes motions into part transforms and
facing into `projection.mirror_x`. `validate_performance`,
`canonicalize_performance`, `performance_identity`. This is not yet the full
Canonical Animation Ontology; it is the visual-realization subset.
## Canonical Visual IR (`visual_ir.hpp/.cpp`)

`VisualIr` is the explicit, immutable, resolved graphics description: entity
identity reference, projection/canvas specification, effective morphology v2,
resolved shapes, visual layers with deterministic order, resolved style,
resolved palette, materials, markings, performance/pose state, lighting spec,
visibility, channel requests. The raster never accesses `CanonicalEntity`.

- **Validation** (`validate_visual_ir`): duplicate node IDs, missing parent,
  cycles, invalid layer/material/palette/shape refs, nonfinite geometry,
  invalid opacity, invalid z relationships, dangling bindings — fail closed.
- **Canonicalization/identity:** `canonicalize_visual_ir`,
  `visual_ir_identity`. Visual IR identity belongs to the existing **Visual
  Manifestation Identity** taxonomy; no eighth top-level identity is invented.
- Schema `gspl.visual-ir/0.1`. Visual IR is currently internal (not yet a
  package artifact); it still has deterministic versioned canonicalization.
- Future-compatible by design: depth, normal, and surface-orientation data can
  be added without schema destruction.

## Visual Semantic Compiler (`visual_compiler.hpp/.cpp`)

`compile_visual_ir(SpriteIr, VisualCompileOptions)` is the only production
entry point from the semantic spine into the visual core:

```
Production SpriteIr + form + effective style + performance state
  -> Visual IR
```

It resolves form morphology (data-driven via `form_id`, no fixed names),
lowers v1 to v2, selects the form palette (base vs transformed, data-driven),
composes the effective style, bakes performance state, and validates.

## Native layered raster compiler (`visual_raster.hpp/.cpp`)

`render_visual_ir(VisualIr, RasterLimits) -> RasterResult` is the first
GSPL-native rendering backend. Output: `ImageRgba8` (0xRRGGBBAA, straight
alpha) plus semantic channels.

- Deterministic draw order: layer, then z_order, then part id (stable sort).
- Ordered layers, fills, strokes, opacity, markings (masked by part geometry),
  palette, materials at baseline fidelity, emission/glow, drop shadow,
  silhouette outline pass, analytic AA.
- **Channels** are derived from the Visual IR (emissive, depth, material_id,
  effects coverage), not reverse-engineered from final RGBA. Channel pixel
  buffers are owned by the channel `ImageRgba8`; `RasterCtx::fb` is a raw
  pointer **rebound from the live image** before every channel draw and the
  channel vector is capacity-reserved, so no vector reallocation can dangle it
  (MSVC Debug STL vector moves are not noexcept; see inline note in
  `render_visual_ir`).
- **Resource bounds** (`RasterLimits`): max canvas dimensions, max flattened
  vertices, max raster work (per render and per channel), max markings/
  channels. Overruns fail closed with diagnostics.
- **Determinism:** identical (entity, style, pose, projection, renderer
  version) implies byte-identical Visual IR, RGBA, channels, identity.
  Verified by A/B determinism tests.

## Visual fidelity metrics (`visual_metrics.hpp/.cpp`)

Deterministic, structured metrics — not a learned critic yet:
`measure_frame_fidelity` (silhouette area/connected components/extrema,
part visibility, palette validity, color-role adherence, alpha-edge quality,
shape overlap, part containment, marking presence, loop closure notes) and
`frame_similarity`/`frame_distinction` (background-invariant: averaged only
over pixels where either frame has content, so real differences are not
diluted by transparent canvas). These are inputs to the future Fidelity Critic.

## Versioning

| Form | Schema | Status |
|---|---|---|
| Morphology v1 | package-compatible v1 | stable (compatibility) |
| Visual Morphology v2 | `gspl.visual-morphology/0.1` | internal, evolving |
| Style | `gspl.style/0.1` | internal, evolving |
| Palette | `gspl.palette/0.1` | internal, evolving |
| Performance | `gspl.performance/0.1` | internal, evolving |
| Visual IR | `gspl.visual-ir/0.1` | internal, evolving |

Nothing in this phase is frozen into the Living Visual Package; the existing
package provenance contract is unchanged.

## Determinism

- Canonical serialization and identities are byte-deterministic.
- Palette/color math is pure; map ordering uses `std::less<>`; draw order is a
  stable sort on (layer, z_order, id).
- All geometry subdivision is bounded and finite-input validated.
- No shared global renderer state; `RasterCtx` is an explicit per-render
  context. (Thread safety: contexts are independent; caches, if added later,
  must have clear ownership semantics.)

## Resource limits

`RasterLimits` bounds canvas dimensions, flattened vertex counts, per-render
and per-channel raster work, and channel/marking counts. `VisualLimits`
bounds morphology part/shape/layer/material/marking/canvas counts at compile
time. Adversarial inputs fail closed with diagnostics; nothing silently
discards unsupported semantics.

## Extension rules

1. New visual semantics extend the **Visual IR** (via the compiler), never a
   parallel authority and never renderer special cases.
2. New shapes/materials/styles/markings are data-driven enums/records with
   deterministic canonicalization; add presets via `make_style_preset`, not
   renderer branches.
3. Renderer changes must be gated by measurable fidelity evidence (metrics +
   viewer inspection), not "looks better".
4. Any new persisted/canonical form gets an explicit schema/version and a
   validation + determinism test before it may enter the package contract.
5. The renderer stays projection-agnostic in semantics: Visual Morphology v2
   separates semantic morphology from projection from render geometry so the
   core can later lift into 2.5D/3D.

## Visual evidence

- Legacy renderer evidence (contact sheet, transformation strip, base vs
  storm, acceptance report): `gsplc --evidence <source.gspl>` -> `evidence/`.
- Native Visual Core evidence: `gspl_sprites_visual_core_tests --evidence
  <dir>` -> `<dir>/visual-core/` (Voltfox base/action/transformation/storm,
  four style grammars, humanoid/mech/flyer generalization fixtures).
- Human inspection of native output: open the PNGs with the Living Visual
  Package viewer or any PNG viewer. The graphical `gspl_sprites_preview`
  remains the package-driven observation instrument for the packaged
  artifacts.
