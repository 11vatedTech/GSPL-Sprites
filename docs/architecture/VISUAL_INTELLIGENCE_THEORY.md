# Visual Intelligence Theory

Governing definitions for the GSPL Native Visual Intelligence discipline.
Derived from the research corpus in `docs/research/visual-intelligence/`;
the mapping from evidence to these definitions is in
`RESEARCH_TO_ARCHITECTURE_MAPPING.md`.

## 1. What is visual identity?

**Definition.** The visual identity of a GSPL entity is the maximal set of
visual facts that remain true across every pose, expression, deformation
within envelope, form, transformation, style, resolution and projection
that the entity may legally undergo.

Visual identity is not a picture, not a pose, not a mesh and not a palette.
It is a **relational invariant set**:

- **Proportion relationships** (ratios between landmark/part measures) with
  allowed ranges;
- **Landmark topology** (which landmarks exist, their ordering and
  adjacency, their symmetry partners);
- **Silhouette structure** (dominant masses, anchors, protrusions,
  concavities, negative spaces);
- **Color-role topology** (which roles exist and which regions they bind
  to — not the concrete colors);
- **Material truth** (which surfaces are fur/skin/metal/energy — not how a
  style renders them);
- **Marking topology** (which markings exist and which surfaces they bind
  to — not their exact geometry);
- **Hard identity boundaries** (per-structure deformation limits beyond
  which the entity is no longer itself).

Evidence: model sheets encode ratios and anchors, not drawings (Williams
2001; Blair 1994; Thomas & Johnston 1981); recognition operates on
silhouette structure (Biederman 1987); Pokemon's silhouette test and
palette economy; One Piece identity anchors under deformation.

## 2. What is a visual invariant?

An invariant is a **typed assertion over canon structures** that must hold
under permitted variation. Categories:

| Kind | Example | Evaluated against |
|---|---|---|
| proportion | eye_spacing / cranial_width ∈ [0.9, 1.1] · preferred | landmark-derived measures |
| landmark_ordering | left_eye precedes right_eye along the head axis | landmark graph order |
| landmark_existence | ear_tip.left exists in every form | landmark graph |
| silhouette_anchor | head mass is the topmost silhouette anchor | silhouette analysis |
| attachment | tail attaches to pelvis region | structural graph |
| color_role_topology | eye regions bind to role `eye` in every form | color canon |
| material_truth | muzzle surface is `skin` in every style | material canon |
| marking_topology | cheek stripes exist in base and storm | marking canon |
| hard_boundary | muzzle length deviation ≤ 20% | deformation canon |

Invariants are **canon data** (`IdentityInvariantSet`), validated by
`validate_visual_canon` and enforced by the compiler's landmark/deformation
checks. They are diagnostics first, gates second: a violated invariant
produces a structured diagnostic; invariants marked `hard` fail closed.

## 3. What is permitted variation?

Variation is legal when it is **declared, bounded and intentional**:

- **Expression**: changes within the FacialCanon's expression-control
  envelopes (eyes, brows, mouth, gaze, non-human channels).
- **Squash/stretch**: changes within per-structure deformation envelopes
  (amplitude, direction, volume-preservation request).
- **Pose**: any pose realizable within envelopes and key-pose rules
  (balance intent may be `intentional_imbalance`).
- **Perspective/projection**: projection-dependent interpretation (see
  section 6) may alter rendered proportions without touching invariants.
- **Action deformation**: force-aware envelopes allow larger deviation for
  flagged structures (smears, impact extremes) — bounded, typed.
- **Secondary motion**: follow-through, jiggle — bounded procedural
  variation, never identity-changing.
- **Style-dependent simplification**: the StyleProgram may omit, merge or
  restyle features only per the ResolutionCanon and LineProgram rules.
- **Transformation**: form deltas follow the TransformationCanon
  trajectory; invariants survive, deltas are declared.

**The off-model principle.** Being off-model is not being wrong; it is
being *intentionally, boundedly* outside the canonical condition. The
engine distinguishes three states: `canonical`, `within_envelope`
(valid, diagnostic-free), and `outside_envelope` (rejected or flagged by
severity). Unbounded or undeclared deviation is never silent.

## 4. What is representation-independent truth?

Facts that remain true in every representation (vector 2D, raster 2D,
pixel art, painterly projection, 2.5D, 3D):

- the structural graph (regions/elements/joints/masses/appendages);
- relational proportions and landmark topology;
- silhouette structure classes (masses, anchors, protrusions, negative
  spaces) — as *structure*, not coordinates;
- color-role topology and material truth;
- marking topology and hard identity boundaries;
- expression-control semantics (what an expression means, not its shape).

These are stored in the **VisualCanon** and are what make an entity "the
same entity" across a sprite, a 3D model and an icon.

## 5. What is projection-dependent interpretation?

Facts that change with projection/rendering and are **not** identity:

- concrete pixel/vector geometry (coordinates, sizes, spacing);
- rendered proportions under perspective/orthographic/cel-projection rules
  (Guilty Gear's per-camera proportion morphing is this made explicit);
- concrete palette values (roles stay, values change);
- line style, shading model, material appearance, FX treatment;
- resolution-specific detail (what survives at 16x16 vs 4K);
- frame cadence and motion abstraction.

The authority rule: **rendering is projection; projection never edits
identity.** The renderer consumes resolved VisualIr; the Visual Semantic
Compiler (with the canon) is the only place semantic→visual decisions are
made.

## 6. The GSPL visual-intelligence pipeline

```
CanonicalEntity (semantic authority)
      ↓
Production SpriteIr (compiler/interchange, existing)
      ↓
VisualCanon (identity invariants + structure + envelopes + style data)
   + PerformanceIntent (pose/action/expression/force semantics)
   + StyleProgram (factorized style behaviors)
      ↓
Visual Semantic Compiler
      ↓
VisualIr (resolved manifestation description; existing)
      ↓
Native Layered Raster Compiler (and future backends)
      ↓
ImageRgba8 / layers / channels
      ↓
deterministic quality diagnostics + human review artifacts
```

Identity ownership: CanonicalEntity owns entity semantics; VisualCanon owns
visual-identity semantics (a derived, manifestation-level authority within
the Visual Manifestation Identity taxonomy — never an entity truth).

## 7. Measurability requirement

Every invariant and envelope is measurable:

- ratios from landmark/part measures;
- silhouette metrics from rendered or canonical geometry;
- envelope deviation from compiled state;
- temporal stability from cross-frame ID tracking.

Metrics are enforceable lower bounds (see VISUAL_FIDELITY_VALIDATION.md).
They never replace human artistic review; they make "identity preserved"
and "identity broken" objective.

## 8. Non-goals (explicit)

- No learned model is authoritative. Learned specialists may propose;
  proposals become typed, validated GSPL structures.
- No style is a franchise imitation; styles are behavior vectors.
- No representation (pixels, vertices) becomes truth; semantic state is
  authoritative, rendering is projection.
- No entity-specific compiler behavior. Entities differ by canon data.
