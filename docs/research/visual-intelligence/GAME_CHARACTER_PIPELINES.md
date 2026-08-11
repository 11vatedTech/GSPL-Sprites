# Game Character Pipelines

## Core question

How do game pipelines represent, skin, animate, retarget and deform
characters at runtime — and which of those layers correspond to GSPL
semantic structures vs. geometric solvers?

## Findings

### F1. Semantic-parameter → mesh realization (SMPL family)
- **Concept:** SMPL: shape β + pose θ → template mesh via LBS; SUPR:
  factorized parts; Anny: interpretable phenotype parameters (age/height/
  weight). Identity, pose and expression are *separate parameter axes*.
- **Source:** Loper et al., SIGGRAPH Asia 2015 (research); Osman et al.,
  ECCV 2022 (research); Bregier et al., arXiv 2511.03589 (research).
- **GSPL implication:** the semantic layer separates identity, pose and
  expression; GSPL generalizes the axes to typed canon structures instead
  of PCA bases.

### F2. Skinning layers: LBS → DQS → corrective shapes
- **Concept:** LBS cheap but collapsing; DQS fixes twist collapse; PSD
  corrective shapes fix volume loss; delta mush smooths procedurally.
  Production rigs stack: bones + blendshapes + correctives.
- **Source:** Kavan et al., I3D 2007 (research); Lewis et al., SIGGRAPH
  2000 (research); Disney hybrid rigging, SIGGRAPH Asia 2010 (research);
  Mancewicz et al., SIGGRAPH 2015 (research, verify).
- **GSPL implication:** skinning is below the semantic line; the
  deformation canon declares joint roles and envelope bounds that future
  solvers honor.

### F3. Animation graphs and blend spaces
- **Concept:** State machines + blend spaces (1D/2D/3D grids, barycentric
  blending) produce continuous locomotion from discrete clips.
- **Source:** Epic Games UE blend-space docs (direct); industry practice.
- **GSPL implication:** runtime action selection is graph/parameter-based;
  the canonical animation ontology's key poses are the compatible semantic
  unit.

### F4. Retargeting between rigs
- **Concept:** Bone maps + local-space rotations + proportional scale
  correction transfer motion across heterogeneous skeletons.
- **Source:** Agata & Igarashi, SIGGRAPH 2025 (research); industry practice.
- **GSPL implication:** key poses defined on landmarks/roles retarget by
  construction across entities sharing a canon structure.

### F5. MetaHuman-style layered rigs
- **Concept:** Multi-layer facial/body rigs (skeleton + blendshape +
  muscle/skin) balance fidelity and runtime cost.
- **Source:** Epic Games MetaHuman technical documentation (direct).
- **GSPL implication:** layered realization is a solver concern; semantic
  controls remain canon data.

### F6. Pose-space deformation in shipping games
- **Concept:** Shipping titles bake pose-space correctives for elbows,
  shoulders and facial extremes; cost is the number of corrective shapes.
- **Source:** production practice; Lewis et al. 2000 (research).
- **GSPL implication:** deformation canon may declare corrective intents;
  the solver stage is future work.

### F7. Motion matching replacing state machines
- **Concept:** Database search over mocap frames minimizes hand-built
  transitions (Clavet, For Honor); learned variants embed the database.
- **Source:** Clavet, GDC 2016 (direct); Holden et al., ACM TOG 2020
  (research).
- **GSPL implication:** future learned specialists propose motion;
  proposals become typed key-pose/intent sequences, validated by the canon.

### F8. Semantic part segmentation drives everything
- **Concept:** Dense part labels enable rigging automation, cloth, style
  transfer and targeted control.
- **Source:** MMHuman3D/OpenMMLab (research infra).
- **GSPL implication:** canon structures carry extensible semantic roles —
  the same principle, from the semantic side.

### F9. Factorized parts (SUPR lesson)
- **Concept:** Global spaces entangle parts; factorized spaces allow
  compositional control (head/torso/limbs mixed independently).
- **Source:** Osman et al., ECCV 2022 (research).
- **GSPL implication:** canon sub-structures are independently addressable;
  form overrides apply per structure; style patches scope per region.

## GSPL synthesis

Game pipelines already separate **semantic control (identity/pose/
expression axes, part semantics, retargetable keys)** from **geometric
realization (LBS/DQS/PSD/blend spaces)**. GSPL's design takes that
separation to its logical end: the semantic layer is fully typed and
deterministic; realization is pluggable below it.
