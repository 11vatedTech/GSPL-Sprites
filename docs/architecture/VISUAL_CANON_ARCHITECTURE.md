# Visual Canon Architecture

## Responsibility

The VisualCanon is the authoritative machine-readable description of how
an entity becomes visually recognizable: its structure, proportions,
landmarks, silhouette, surfaces, materials, colors, facial regions,
markings, attachments, deformation limits, resolution behavior and
identity invariants. It is a **derived manifestation-level authority**
(Visual Manifestation Identity taxonomy) — it never replaces
CanonicalEntity.

## Data model

```text
VisualCanon
 ├── StructuralCanon        regions → elements/joints/masses/appendages + relationships
 ├── ProportionCanon        relational ratios with preferred/allowed/deformation ranges
 ├── LandmarkCanon          semantic landmark graph (roles, ownership, frames, symmetry)
 ├── SilhouetteCanon        masses, anchors, protrusions, concavities, negative spaces
 ├── SurfaceCanon           surface regions (semantic areas of structures)
 ├── MaterialCanon          material truth per surface/region (class + response params)
 ├── ColorCanon             color roles per region + palette constraints (economy, separation)
 ├── FacialCanon            ExpressiveRegions + typed ExpressiveFeatures with controls
 ├── MarkingCanon           markings bound to canon structures (kind, region, intent)
 ├── AttachmentCanon        sockets/attachment anchors
 ├── DeformationCanon       per-structure DeformationEnvelope
 ├── ResolutionCanon        VisualFeature priority/min-resolution/substitution/merge/omission
 └── IdentityInvariantSet   typed invariants + hard boundaries
```

### Relationship kinds (StructuralCanon)

parent, child, attachment, articulation, adjacency, symmetry,
correspondence, containment, overlap, surface_ownership. Entities of any
body plan (biped, quadruped, serpentine, insect, bird, fish, blob, plant,
machine, vehicle, weapon, building, abstract) differ by structure data, not
by code.

### Proportion rules (ProportionCanon)

`{ id, landmark_a, landmark_b | part_a | measure, preferred, min, max,
deformation_min, deformation_max, style_range, hard }`. Measures may be
distances between landmarks or part sizes. Deterministic validation:
min ≤ preferred ≤ max; deformation range must contain the allowed range.

### Landmarks (LandmarkCanon)

`{ id, role (extensible), owner (structure), x/y/z (local frame), symmetry
partner, deformability, visibility, silhouette_anchor, constraints }`.
Landmarks exist for any body plan (eye_center.left, wheel_center.front_left,
wing_root.right, tail_tip...).

### Silhouette features (SilhouetteCanon)

`{ id, kind (mass|anchor|protrusion|concavity|negative_space), priority,
structure_ref, min_contribution, min_separation }`. Measurable tests:
occupancy, connected components, anchor alignment, negative-space
preservation, cross-entity distinction.

### Facial canon (FacialCanon)

`ExpressiveRegion { id, structure_ref, features[] }`;
`ExpressiveFeature { id, kind (eye|brow|mouth|visor|antenna|light|panel|...),
control_semantics (gaze, aperture, intensity, rotation, squash), envelope }`.
Non-human expression is first-class: a visor aperture, an antenna and a
mouth are all features.

### Deformation envelopes (DeformationCanon)

`DeformationEnvelope { structure_id, canonical_value, allowed_deviation,
action_scale, expression_scale, style_scale, transformation_scale,
hard_boundary, rigid }`. See SEMANTIC_DEFORMATION_ARCHITECTURE.md.

### Resolution features (ResolutionCanon)

`VisualFeature { id, structure_ref, semantic_priority,
recognition_importance, min_resolution, substitution_rule,
merge_rule, omission_rule }`. Identity must survive resolution loss via
declared substitution/merge/omission, never via accidental downsampling.

### Identity invariants (IdentityInvariantSet)

`IdentityInvariant { id, kind, refs[], tolerance, hard }`. Kinds:
proportion, landmark_ordering, landmark_existence, silhouette_anchor,
attachment, color_role_topology, material_truth, marking_topology,
hard_boundary.

## Invariants

1. Canon graphs are acyclic and well-typed (every ref resolves).
2. Canonicalization is deterministic; identity is stable.
3. No entity-specific behavior: the compiler consumes canon data only.
4. Canon identity is a Visual Manifestation Identity input: it changes
   with style/form/pose inputs but never claims entity truth.
5. Validation fails closed on: duplicate ids, dangling refs, cycles,
   nonfinite values, inverted ranges, unknown enum spellings.

## Ownership

- **Producer:** authoring fixtures and future authoring tools (canon
  definitions are authored data).
- **Consumer:** Visual Semantic Compiler (canon → morphology/landmarks),
  deformation checks, style resolution, fidelity diagnostics.
- **Renderer:** never sees the canon; it consumes resolved VisualIr.

## Interactions

- canon + form → `canon_to_morphology` (constructed VisualMorphologyV2).
- canon + performance → landmark/deformation enforcement.
- canon + style → per-region style scoping + resolution behavior.
- canon → visual_canon_identity (feeds VisualIr identity).

## Determinism and serialization

All canon structs implement `validate_*`, `canonicalize_*` (deterministic
ordering via sorted maps/lists) and identity functions. Malformed-input
and round-trip tests required; forward evolution via schema versioning
(`gspl.visual-canon/0.1`).

## Testing

- valid construction per fixture (fox, humanoid, mech, flyer);
- invalid structural graph (cycle, dangling parent/landmark/proportion);
- invalid proportions (inverted ranges) and invariants;
- identity: permitted deformation passes, invariant-breaking fails;
- determinism: repeated canonicalization byte-identical;
- serialization: round-trip equivalence;
- generalization: all four fixtures compile through one compiler.

## Extension strategy

New body plans = new canon data, not new code. New invariant kinds =
typed additions to the vocabulary with validation. New styles/forms =
canon data + StyleProgram behaviors.
