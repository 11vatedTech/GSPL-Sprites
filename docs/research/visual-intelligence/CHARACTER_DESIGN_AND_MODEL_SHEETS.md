# Character Design and Model Sheets

## Core question

How do productions keep a character consistent across poses, artists,
episodes and media — and what computational structure is that consistency?

## Findings

### F1. Model sheets encode relational invariants, not fixed drawings
- **Concept:** Master blueprints (front/3/4/side/back views, expression
  grids, scale comparisons) define a character once, for all time.
- **Practice:** Rather than fixed pixel content, model sheets record
  proportional ratios (head-to-body height, eye-spacing-to-head-width),
  silhouette profiles and anatomical anchor points that must hold in every
  drawing.
- **GSPL implication:** The canonical representation of identity is a set of
  *relational invariants* (proportions, landmarks, silhouette anchors) —
  not a pixel image and not a single pose.
- **Source:** Williams, Richard. *The Animator's Survival Kit*. Faber &
  Faber, 2001. (direct); Blair, Preston. *Cartoon Animation*. Walter Foster,
  1994. (direct); Thomas & Johnston. *Disney Animation: The Illusion of
  Life*. Abbeville, 1981. (direct).

### F2. Construction drawings decompose anatomy into primitives
- **Concept:** Complex organic forms are built from spheres, cylinders,
  boxes and wedges in 3D space; the character is *assembled*, not drawn.
- **GSPL implication:** A hierarchical structural canon of volumetric
  primitives with explicit articulation centers; this is exactly the
  morphology v2 part hierarchy, promoted to first-class structural meaning.
- **Source:** Blair 1994 (direct).

### F3. Landmark / iconic feature anchoring
- **Concept:** Designs carry 3-5 hyper-salient anchors (distinct scar, hat
  profile, asymmetric plate) that survive all deformation.
- **Why it works:** Recognition relies on configural processing — the
  *relationships between* distinctive landmarks, not the whole image.
- **GSPL implication:** A landmark graph with per-landmark deformability;
  landmark *ordering and ratio* relationships are identity invariants.
- **Source:** RMCAD, *The Secrets Behind Character Design*, 2024
  (practitioner synthesis); Gestalt/invariant recognition literature
  (F18, F22).

### F4. Negative space is designed, not accidental
- **Concept:** Gaps between limbs/torso/cape are carved deliberately so
  masses never merge into silhouette sludge at distance or in motion.
- **GSPL implication:** Silhouette canon must carry *negative-space
  features* with measurable minimum separation requirements.
- **Source:** BINUS DKV, *The Power of Silhouette*, 2025 (practitioner);
  silhouette perception findings in SILHOUETTE_AND_SHAPE_LANGUAGE.md.

### F5. Shape contrast and visual hierarchy
- **Concept:** Juxtaposing contrasting forms (blocky torso + tapered limbs;
  round head + angular horns) creates focal hierarchy and recognition.
- **GSPL implication:** Structural canon should record *shape language per
  region* and permit contrast-aware validation (curvature variance across
  adjacent regions).
- **Source:** RMCAD 2024 (practitioner synthesis); 80 Level, *Character
  Design: Shape Language and Readability*, 2018 (practitioner).

### F6. Relational proportion systems (head units)
- **Concept:** Proportions are expressed in modular units (chibi 2-3 heads,
  heroic 8 heads) — a scale-free relational system, not absolute measures.
- **GSPL implication:** Proportion canon stores *ratios between landmark or
  part measures* with preferred value + allowed range + deformation range,
  not pixel coordinates.
- **Source:** Bregier et al., *Human Mesh Modeling for Anny Body*, arXiv
  2511.03589, 2025 (research); classical character-design literature.

### F7. Mascot / icon design: identity at every resolution
- **Concept:** A mascot must remain identifiable from 4K render down to a
  16x16 icon. High-frequency detail vanishes; macro-silhouette and
  two-tone contrast must carry identity.
- **GSPL implication:** Resolution canon: per-feature minimum resolution,
  substitution/merge/omission rules, silhouette features flagged as
  resolution-critical.
- **Source:** 80 Level, *Icon Design Identity Across Resolutions*, 2021
  (practitioner); Pokémon readability practice
  (see CREATURE_DESIGN_SYSTEMS.md).

### F8. Factorized body-part representation
- **Concept:** Global blend/parameter spaces entangle parts (changing foot
  size shifts torso); factorized part models (SUPR) decouple head/torso/
  limbs for compositional control.
- **GSPL implication:** Canon sub-structures should be independently
  addressable and independently deformable; form overrides apply per
  structure.
- **Source:** Osman et al., *SUPR: A Sparse Factorised Model of the Human
  Body*, ECCV 2022 (research).

### F9. Gestalt invariance across transforms
- **Concept:** Recognition extracts invariant topological relationships
  that survive rotation, scale, pose and expression change.
- **GSPL implication:** Identity invariants are defined over *topological
  relationships* (order, adjacency, ratio, connectivity) so they survive
  projection and restyling.
- **Source:** Gestalt literature; IxDF, *What are the Gestalt Principles?*,
  2026 (synthesis); Biederman 1987 (see VISUAL_FIDELITY_EVALUATION.md).

### F10. Asymmetry and counterbalance
- **Concept:** Controlled asymmetry (a heavy side, a lean) reads as life and
  intent; it must remain balanced by optical weight.
- **GSPL implication:** Pose/landmark validation can check center-of-mass
  projection against support contacts, and permit *intentional imbalance*
  as a first-class pose state.
- **Source:** BINUS 2025 (practitioner); POSE_GESTURE_AND_LINE_OF_ACTION.md.

## Synthesis for GSPL

A Visual Canon is a **relational invariant set** (proportions, landmarks,
silhouette anchors, material/color topology) plus **bounded deviation
rules** (deformation envelopes) plus **resolution rules** — not a picture.
This is the single most important conclusion of the corpus.
