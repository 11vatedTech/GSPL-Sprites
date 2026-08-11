# Color, Light and Materials

## Core question

What is color/material/lighting *truth* vs. *artistic response*, and how
are both represented so styles can reinterpret without destroying
identity?

## Findings

### F1. Palette economy (Pokémon)
- **Concept:** ~3 primary hues + neutrals per creature; strong contrast;
  visual fatigue avoided; identity aided.
- **Source:** Pokémon developer interviews (Sugimori), Shmuplations
  (direct/translation).
- **GSPL implication:** ColorCanon caps palette size, enforces minimum
  separation and hue relationships; palette validation is a canon gate.

### F2. Color roles vs literal colors
- **Concept:** Designs assign semantic roles (primary, accent, eye,
  emission, shadow, outline, damage) that *survive* palette shifts; a
  transformation changes the palette, not the topology of roles.
- **GSPL implication:** already embodied in ColorRole and PaletteDefinition;
  the canon extends it: role→region bindings + value/contrast constraints.

### F3. Hue shifting in ramps
- **Concept:** Pixel/illustration practice builds value ramps by shifting
  hue toward warm/cool, not just darkening — richer ramps with stable
  identity.
- **Source:** pixel-art practice (PIXEL_ART_SEMANTICS.md); illustration
  practice.
- **GSPL implication:** palette generation can derive ramps deterministically
  with hue-shift curves (a PaletteProgram behavior).

### F4. Material truth vs artistic response (core separation)
- **Concept:** A surface *is* fur/steel/glass/energy semantically; the
  style decides how that reads (cel bands, hatching, palette ramps, PBR).
- **Source:** Arcane's hand-painted materials + Guilty Gear's stepped
  ramps (HYBRID_2D_3D_PRODUCTION.md); NPR literature.
- **GSPL implication:** MaterialSemantics already encodes surface response
  vocabulary; StyleProgram selects interpretation. Material identity stays
  constant across styles.

### F5. Artist-directed normals (Guilty Gear)
- **Concept:** Vertex normals are hand-tuned so cel shadow terminators land
  where a 2D artist would draw them — decoupling lighting geometry from
  physical normals.
- **Source:** Motomura, *Guilty Gear Xrd's Art Style: The X Factor Between
  2D and 3D*, GDC 2015 (direct).
- **GSPL implication:** future shading programs can carry per-region
  shadow-curve intent ("shadow terminator here"), a semantic form of
  artist-directed normals.

### F6. Stepped shadow ramps and shadow control
- **Concept:** 1-3 band ramps via custom UV/dot-product thresholds replace
  smooth Lambert; number and tint of steps is an artistic control.
- **Source:** Motomura GDC 2015 (direct); GG Strive UE interviews
  (HYBRID_2D_3D_PRODUCTION.md).
- **GSPL implication:** ShadingProgram carries band count, ramp curve and
  terminator policy — data, not shader code.

### F7. Stylized/non-physical lighting (Arcane)
- **Concept:** Localized non-realistic light rigs paint light onto
  characters regardless of physics; heavy rim/color-bleed for separation
  and hierarchy.
- **Source:** Fortiche interviews; RedShark technical feature, 2024
  (direct).
- **GSPL implication:** LightingSpec (already in VisualIr) is semantic
  intent (key/fill/rim/ambient), not a light simulator; styles interpret.

### F8. Rim and emissive separation
- **Concept:** Rim light separates subject from background; emission
  carries energy state. Both are *role channels*, not decoration.
- **GSPL implication:** existing MaterialSemantics rim_response/emission
  and emissive channel map; canon declares which regions may emit.

### F9. Value grouping and saturation behavior
- **Concept:** Stylization groups values (compresses luminance range) and
  modulates saturation per material (skin stays desaturated, energy
  saturated).
- **GSPL implication:** StyleSemantics value_grouping/saturation_behavior
  already exist; canon may constrain per-material saturation policy.

## GSPL synthesis

Color = roles + palette + constraints (economy, separation, hue
relationships). Light = semantic intents (ambient/key/fill/rim). Materials
= surface truth vocabulary. All three are *facts*; style programs are
*interpretations*. The identity contract is that role topology and material
truth never change under restyling.
