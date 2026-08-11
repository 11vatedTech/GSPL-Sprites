# Facial Expression and Acting

## Core question

How is facial expression encoded in production, and how can the encoding
generalize beyond humanoid faces to animals, monsters, machines and
abstract entities?

## Findings

### F1. FACS decomposes expression into orthogonal Action Units
- **Concept:** FACS (Ekman & Friesen) classifies facial movement into
  anatomically independent Action Units (AU1 inner brow raiser, AU12 lip
  corner puller...), driven by named muscles.
- **GSPL implication:** expression is a set of *orthogonal semantic
  channels* over a structure, not a pose or a texture. The functional
  decomposition generalizes: ear pinning, whisker compression, snout
  wrinkle are the non-human AUs.
- **Source:** Ekman & Friesen, *FACS*, Consulting Psychologists Press,
  1978 (direct).

### F2. Blendshape banks and the blendshape STAR
- **Concept:** Linear/nonlinear blendshape models dominate production;
  the state of the art covers construction, additive vs subtractive
  shapes, and combinatorial explosion control.
- **GSPL implication:** the facial canon declares *expression controls*
  (semantic sliders); a future geometric solver may realize them via
  blendshape-like bases — but the canon itself is semantic, not vertex
  lists.
- **Source:** Lewis, Pettit, Taylor, Parke, Lyons, Omlor, Rusinkiewicz.
  *Practice and Theory of Blendshape Facial Models*. Eurographics STAR,
  2014 (research).

### F3. Example-based facial rigging (EBFR)
- **Concept:** EBFR auto-derives optimal blendshape bases from sparse
  artist examples; mesh-agnostic, works for creatures with consistent
  connectivity.
- **GSPL implication:** supports the eventual learned-specialist boundary:
  examples/proposals become typed expression data, validated by the canon,
  never silent truth.
- **Source:** Li, Luo, Vlasic, Peers, Popović, Pauly. *Example-Based
  Facial Rigging*. SIGGRAPH 2010 (research).

### F4. Pose-space corrections for extreme expressions
- **Concept:** Hybrid rigs (bone + blendshape + corrective PSD) prevent
  collapse at extreme jaw/brow angles; essential for crocodile/dragon/
  robot jaws.
- **GSPL implication:** deformation canon per expressive structure declares
  allowed deviation; PSD-style correctives are a solver concern below the
  semantic layer.
- **Source:** Komorowski, Melapudi, Mortillaro, Lee (Disney). *A Hybrid
  Approach to Facial Rigging*. SIGGRAPH Asia 2010 (research).

### F5. Stylized anime/game facial systems
- **Concept:** Large eyes, floating brows, detached highlights,
  multi-quadrant mouth shapes (M-shape smile) — anatomy subordinated to
  readability and appeal.
- **GSPL implication:** the facial canon is *style-aware by design*:
  eye/mouth/brow structures declare alternative render interpretations
  (aperture vs mesh vs line); the StyleProgram selects.
- **Source:** Epic/Cygames GDC presentations on stylized facial animation
  2021 (verify); Guilty Gear production literature (HYBRID_2D_3D_PRODUCTION.md).

### F6. Non-human expression channels (machines and animals)
- **Concept:** Robots emote via visor apertures, antenna orientation, panel
  deformation, light intensity; animals via ear angle, tail position, pupil
  shape.
- **GSPL implication:** the canon's ExpressiveRegion/Feature framework is
  explicitly non-anatomical: a visor aperture, an antenna, a light channel
  and a mouth are all `ExpressiveFeature`s with typed control semantics.
- **Source:** robotics/creature practice (RSS expressive robotics work —
  verify; Pixar creature rigging course notes — verify).

### F7. Gaze and pupil control
- **Concept:** Gaze direction (saccades, pursuit, vergence) and pupil
  dilation are decoupled rig channels; eyelid follows eyeball.
- **GSPL implication:** gaze is a first-class performance intent feeding
  eye landmarks and expression channels.
- **Source:** Sony Imageworks technical presentations on eye animation
  (verify); industry rigging practice.

### F8. Semantic expression spaces
- **Concept:** High-dimensional vertex displacements are mapped to
  low-dimensional semantic axes (valence, arousal, dominance, stylized
  emotion) for intuitive control.
- **GSPL implication:** expression intent is semantic (e.g.,
  `focused_aggression`), lowered to typed controls by the canon's facial
  region rules; axes may be re-anchored per species.
- **Source:** Popa et al., *Semantically-aware blendshape rigs from facial
  performance measurements*. CGF, 2016 (research).

### F9. Perceptually-based blendshape importance
- **Concept:** Perceptual models rank which facial channels viewers attend
  to, allowing rig pruning without perceived loss.
- **GSPL implication:** fidelity diagnostics can weight facial-landmark
  drift by channel importance; a future Fidelity Critic consumes the same
  data.
- **Source:** Dahyot et al., *Investigating perceptually based models to
  predict importance of facial blendshapes*. MIG 2020 (research).

### F10. Procedural life: noise and micro-expression
- **Concept:** Subtle procedural noise (breathing, resting tension,
  ear twitch, mechanical jitter) prevents "dead" characters.
- **GSPL implication:** secondary-motion intents in PerformanceState
  (already present) are the semantic hook; deterministic generation is
  required.
- **Source:** SIGGRAPH Talks on procedural life in facial pipelines 2019
  (verify).

## GSPL synthesis

Facial expression is a **semantic control problem**. The FacialCanon
declares ExpressiveRegions and typed ExpressiveFeatures with control
semantics; PerformanceIntent carries expression intent; the StyleProgram
decides the visual interpretation (mesh, aperture, line, light). No
humanoid assumption is embedded anywhere — a fox, a robot and a flower all
express through the same framework.
