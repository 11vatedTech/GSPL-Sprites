# Stylized Deformation

## Core question

How far may an entity deviate from its canonical form during action,
expression and transformation — and which computational techniques exist
to realize deformation?

## Findings

### F1. Squash and stretch with volume preservation
- **Concept:** Deforming to emphasize flexibility/weight while conserving
  volume (Sx·Sy·Sz = k); the foundational animation principle.
- **Source:** Thomas & Johnston 1981 (direct); Williams 2001 (direct);
  Terzopoulos et al., *Elastically Deformable Models*, SIGGRAPH 1987
  (research, verify).
- **GSPL implication:** deformation envelopes expose squash/stretch
  amplitude per structure; volume-preservation is a solver constraint the
  semantic layer can request.

### F2. Free-form deformation (FFD) and EFFD
- **Concept:** Embedding an object in a parametric lattice (Bézier/B-spline)
  and moving control points; EFFD extends to arbitrary lattice topologies.
- **Limitation:** global, lacks local precision.
- **Source:** Sederberg & Parry, *Free-Form Deformation of Solid Geometric
  Models*, SIGGRAPH 1986 (research); Coquillart, *Extended FFD*, SIGGRAPH
  1990 (research).
- **GSPL implication:** lattice/FFD is a realization technique for
  region-level deformation envelopes (squash, twist, bend).

### F3. Cage deformation: MVC and Green coordinates
- **Concept:** Cages with generalized barycentric coordinates (mean value)
  or boundary-integral coordinates (Green, using vertex positions *and*
  face normals) deform interior space smoothly; Green coordinates preserve
  local rigidity and volume better than MVC.
- **Source:** Ju, Schaefer & Warren, *Mean Value Coordinates for Closed
  Triangular Meshes*, SIGGRAPH 2005 (research); Lipman, Levin & Cohen-Or,
  *Green Coordinates*, SIGGRAPH 2008 (research).
- **GSPL implication:** cage coordinates are the natural realizer for
  stylized regional deformation (fat/muscle dynamics, cartoon bending).

### F4. As-rigid-as-possible (ARAP) deformation
- **Concept:** Minimizing an energy that penalizes deviation of local
  per-vertex transforms from rigid motion; preserves detail under large
  bending.
- **Source:** Sorkine & Alexa, *As-Rigid-As-Possible Surface Modeling*,
  SGP 2007 (research); Alexa, Cohen-Or & Levin, *As-Rigid-As-Possible
  Shape Interpolation*, SIGGRAPH 2000 (research).
- **GSPL implication:** ARAP-like local rigidity is the principle for
  "allowed deviation preserves identity structure" — deform envelopes can
  declare which regions must remain rigid (hard identity boundary).

### F5. Linear blend skinning artifacts and dual quaternion skinning
- **Concept:** LBS (v' = Σ wi Mi v) is cheap but collapses volume at
  twisting joints (candy-wrapper, collapsing elbow). DQS blends dual
  quaternions, eliminating the collapse but introducing bulging at
  compound joints.
- **Source:** Magnenat-Thalmann et al., *Joint-Dependent Local Structures*,
  Graphics Interface 1988 (research, verify); Kavan, Collins, Žára &
  O'Sullivan, *Skinning with Dual Quaternions*, I3D 2007 (research).
- **GSPL implication:** skinning quality is a solver-level concern;
  the semantic layer declares joint roles and envelope bounds.

### F6. Pose-space deformation and corrective shapes
- **Concept:** PSD parameterizes corrective displacements by joint angles
  (RBF/scatter interpolation over pose space); corrective shapes/delta
  blendshapes fix artifact poses; delta mush procedurally smooths LBS
  artifacts without manual weights.
- **Source:** Lewis, Cordner & Fong, *Pose Space Deformation*, SIGGRAPH
  2000 (research); Mancewicz et al., *Delta Mush*, SIGGRAPH 2015 Talks
  (research, verify).
- **GSPL implication:** the deformation canon can declare per-pose
  corrective intents (what must bulge/fold); the geometric solver stage is
  future work.

### F7. Embedded deformation
- **Concept:** A sparse graph of deformation nodes embedded in dense
  geometry drives smooth localized deformation; bridges sparse control and
  dense surfaces.
- **Source:** Sumner, Schmid & Pauly, *Embedded Deformation*, SIGGRAPH
  2007 (research).
- **GSPL implication:** landmark-anchored deformation (future) can use
  embedded-node graphs for organic bending.

### F8. Smear frames and motion smears
- **Concept:** 2D production stretches geometry along velocity vectors or
  ghosts multi-poses per frame to convey extreme speed without motion
  blur; a *deliberate* temporal incoherence.
- **Source:** Spider-Verse production literature (see
  HYBRID_2D_3D_PRODUCTION.md); animation practice (Dragon Ball speed
  abstraction).
- **GSPL implication:** performance intent can request smear frames as a
  typed effect (impact/velocity thresholds), interpreted by the Style/
  FX program — never applied silently.

### F9. Topology-changing deformation
- **Concept:** Level sets/remeshing allow splits/fusions (liquid
  characters, metamorphosis) at the cost of UV/rigging/semantic tracking.
- **GSPL implication:** transformation morphogenesis may need topology
  change; semantic structures (not vertices) must be tracked through it —
  hence temporal identity (TEMPORAL_COHERENCE.md).
- **Source:** Losasso, Gibou & Fedkiw, *Simulating Water and Smoke*, SIGGRAPH
  2004 (research, verify topic match); metamorphosis literature.

### F10. Non-rigid secondary dynamics
- **Concept:** Elastic rods/shells/mass-spring simulate secondary motion
  (hair, tails, cloth, jiggle) as physical systems.
- **Source:** Bergou, Wardetzky, Robinson, Audoly & Grinspun, *Discrete
  Elastic Rods*, SIGGRAPH 2008 (research).
- **GSPL implication:** secondary-motion intents feed deterministic
  procedural dynamics; ownership is explicit (per-part, bounded).

## GSPL synthesis (architecture-relevant)

1. **Decouple semantic intent from geometric solver.** Envelopes state
   *what* (amplitude, bounds, rigidity), solvers realize *how* (FFD, cage,
   ARAP, DQS, PSD).
2. **Rigidity is the identity boundary.** Deformation is bounded per
   structure; rigid regions protect identity invariants.
3. **Volume preservation is a requested constraint**, not an accident.
4. **Smear/ghosting is deliberate effect**, gated by typed performance
   intent.
5. **Topology change must be tracked semantically**, not geometrically.
