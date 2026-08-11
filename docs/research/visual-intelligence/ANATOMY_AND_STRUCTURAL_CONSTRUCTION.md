# Anatomy and Structural Construction

## Core question

What is the structural grammar beneath a design — the hierarchy, joints,
proportions and functional logic that make an entity constructible,
animatable and recognizable?

## Findings

### F1. Construction drawings = primitive decomposition
- **Concept:** Figures are built from volumetric primitives (spheres,
  cylinders, boxes, wedges) arranged in 3D space; construction drawings
  show the *assembly*, guaranteeing volume and depth under projection.
- **Source:** Blair, Preston. *Cartoon Animation*. Walter Foster, 1994
  (direct).
- **GSPL implication:** structural canon is a hierarchical assembly of
  primitives with explicit local frames and articulation centers — the
  morphology v2 part hierarchy given semantic meaning (regions, elements,
  joints, masses, appendages).

### F2. Comparative anatomy and skeletal homology
- **Concept:** Human and animal skeletons share homologous structures
  (scapula, femur, digits) with functional divergence; artists use
  homologous templates to construct believable creatures of any body plan.
- **Source:** Ellenberger, Dittrich & Baum. *Atlas of Animal Anatomy for
  Artists*. Dover, 1956 (direct).
- **GSPL implication:** the structural canon is a *general kinematic graph*,
  not a humanoid template; body-plan is expressed by which nodes exist and
  how they connect (biped, quadruped, serpentine, volant, insectoid,
  abstract), not by hardcoded part names.

### F3. Biomechanical plausibility anchors believability (Whitlatch)
- **Concept:** Fictional creatures must obey functional logic: gravity,
  muscle attachment, weight distribution. Extra limbs attach to valid
  pectoral/pelvic girdle nodes; joints follow mechanical roles.
- **Source:** Whitlatch, Terryl. *Animals Real and Imagined*. Design Studio
  Press, 2011 (direct).
- **GSPL implication:** canon attachment rules — appendages declare
  attachment regions; validation rejects appendages with no declared
  articulation/attachment parent; joint roles are typed
  (hinge, ball, gliding, fixed).

### F4. Parametric body models (SMPL family)
- **Concept:** SMPL represents body shape/pose as low-dimensional functions
  of shape params (beta) and pose params (theta) over a template mesh via
  linear blend skinning; SUPR factorizes parts; Anny provides interpretable
  phenotype parameters.
- **Source:** Loper et al., *SMPL*, ACM TOG 34(6), 2015; Osman et al.,
  *SUPR*, ECCV 2022; Bregier et al., *Anny Body*, arXiv 2511.03589, 2025
  (research).
- **GSPL implication (and limits):** these are *statistical shape models for
  humans* — their key architectural lesson is separation of
  **identity/body-plan parameters from pose parameters from expression
  parameters**. GSPL generalizes the lesson to arbitrary creatures with
  semantic structures instead of PCA bases.

### F5. Semantic part segmentation
- **Concept:** Dense semantic labels per region (head, cranium, upper_arm,
  digit_1) enable rigging, cloth attachment, style transfer and targeted
  control.
- **Source:** MMHuman3D/OpenMMLab 2021 (research infra); game pipeline
  practice.
- **GSPL implication:** every canon structure carries a semantic role from
  an extensible vocabulary; materials, markings, deformations and sockets
  bind to semantic roles, never to pixel areas.

### F6. Skeletal hierarchies and landmark graphs
- **Concept:** Anatomy is a kinematic DAG; nodes are joints/landmarks,
  edges are bones with typed DOF. Motion propagates parent-to-child;
  landmark graphs give artists a sparse semantic handle on dense geometry.
- **Source:** kinematic-graph practice (SKEL: Osman et al., SIGGRAPH Asia
  2023); procedural character literature.
- **GSPL implication:** two complementary graphs: the **structural graph**
  (regions/elements/appendages with relationship kinds) and the
  **landmark graph** (sparse semantic points with constraints). Landmarks
  are first-class identity anchors.

### F7. Pose-space corrections for volume loss
- **Concept:** Linear skinning collapses volume at joints ("candy wrapper",
  collapsing elbow); pose-space deformation (PSD) adds artist-sculpted
  corrective shapes driven by joint angles.
- **Source:** Lewis, Cordner & Fong, *Pose Space Deformation*, SIGGRAPH
  2000 (research); SMPL 2015 (research).
- **GSPL implication:** deformation canon can declare per-joint corrective
  envelopes — semantic "what may bulge/fold when" rules that a future
  geometric solver consumes. (Details in STYLIZED_DEFORMATION.md.)

### F8. Shape grammars and procedural anatomy
- **Concept:** Formal rewriting rules iteratively subdivide/append primitives
  into complex structures — a compact generative description of anatomy.
- **Source:** Merrell, *Procedural Modeling Using Graph Grammars* (verify:
  Paul Merrell / Stanford materials).
- **GSPL implication:** the structural canon is intentionally *grammar-
  compatible*: structures declare how they may be elaborated (e.g., limb →
  upper/lower/digits) so future procedural variation composes without new
  code.

### F9. Symmetry and correspondence
- **Concept:** Bilateral symmetry is the default expectation for organisms;
  correspondence (left↔right, base↔form) anchors retargeting, deformation
  transfer and transformation.
- **Source:** Sumner & Popović, *Deformation Transfer for Triangle Meshes*,
  SIGGRAPH 2004 (research); anatomy practice.
- **GSPL implication:** canon relationships include `symmetry` (mirror
  pairs) and `correspondence` (map between forms/states) so identity and
  deformation rules can be stated once per pair.

## GSPL synthesis

The Structural Canon is a general, typed semantic hierarchy: regions →
elements/joints/masses/appendages, with relationship kinds (parent, child,
attachment, articulation, adjacency, symmetry, correspondence,
containment, overlap, surface ownership) and body-plan-agnostic roles. It
is the substrate every other canon (proportion, landmark, silhouette,
facial, material, marking, deformation) binds to.
