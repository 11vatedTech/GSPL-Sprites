# Research → Architecture Mapping

For every major GSPL architectural decision: the observed production/
research problem, the evidence, the generalized principle, the naive
approach rejected, the chosen abstraction, and the validation strategy.

## Visual Canon (docs/architecture/VISUAL_CANON_ARCHITECTURE.md)

| Decision | Evidence | Naive approach rejected | Chosen abstraction | Validation |
|---|---|---|---|---|
| Identity = relational invariant set | Model sheets encode ratios/anchors, not drawings (Williams 2001; Blair 1994; Thomas & Johnston 1981) | Freeze one canonical drawing/pose | VisualCanon with ProportionCanon (ratios + ranges), LandmarkCanon, IdentityInvariantSet | invariant ratio/ordering tests; permitted-vs-breaking deformation tests |
| Silhouette is first-class declared structure | Solid-black-fill test (Pokemon); pre-attentive silhouette recognition (Biederman 1987) | Silhouette as by-product of part placement | SilhouetteCanon: masses, anchors, protrusions, concavities, negative spaces | occupancy/components/anchor/negative-space metrics; cross-entity distinction |
| Structure = general typed graph, not humanoid template | Comparative anatomy/homology (Ellenberger 1956); Whitlatch biomechanics; kinematic DAGs | Hardcode humanoid/quadruped parts | StructuralCanon: regions/elements/joints/masses/appendages with relationship kinds | invalid-graph/attachment/cycle tests; generalization fixtures (humanoid, mech, flyer, fox) |
| Deformation intentional and bounded | One Piece identity anchors; Dragon Ball force deformation; PSD corrective practice | Freeze all proportions (kills expression) or allow unbounded deformation | DeformationCanon: per-structure envelopes with canonical value, deviation, hard identity boundary | hard-invariant checks pass; envelope-violation fails; action/style variation allowed |
| Expression = semantic channels over structure | FACS AUs (Ekman 1978); stylized systems; visor/antenna/light channels | Blendshape vertex lists as truth; humanoid-only faces | FacialCanon: ExpressiveRegion/ExpressiveFeature with typed controls; PerformanceIntent expression | expression controls validated; non-human expression channels tested |
| Palette economy and color roles | Pokemon 3-hue rule; role topology survives restyle | Literal colors everywhere | ColorCanon: role→region bindings, palette caps, separation/hue constraints | palette validation; role topology stable across styles/forms |
| Materials are truth; style interprets | Arcane painterly vs Guilty Gear stepped ramps vs PBR | Switch statements on material names | MaterialCanon/StyleProgram split (existing MaterialSemantics + new StyleProgram) | same material under 4 styles remains material-identical |
| Markings bind to semantic surfaces | surface-space features; temporal coherence STAR | Bake markings into pixels | MarkingCanon binds to canon structures (existing Marking.part_id) | marking-part binding validated; markings move with surface |
| Resolution identity survives | mascot/icon tests; Pokemon sprite readability | One LOD for all sizes; nearest-neighbor shrink | ResolutionCanon: VisualFeature priority/min_resolution/substitution/merge/omission | same canon at multiple sizes → stable silhouette metrics |
| Transformations are trajectories | Pokemon additive evolution; Dragon Ball escalation | Crossfade two rendered images | TransformationCanon: invariants + additive deltas + trajectories | intermediate states compiled; invariants preserved; identity lineage proven |

## Performance (docs/architecture/VISUAL_PERFORMANCE_ARCHITECTURE.md)

| Decision | Evidence | Naive approach rejected | Chosen abstraction | Validation |
|---|---|---|---|---|
| Pose = semantic state (line of action, balance, force, gaze) | Gesture/line-of-action practice; staging; balance practice | Joint-angle vectors as pose truth | KeyPose with line of action, CoM, support contacts, silhouette goal, gaze, force, balance intent | pose validation; balance diagnostics; deterministic key generation |
| Motion = sparse keys + phases + timing | Timing charts, anticipation, follow-through (Williams; Thomas & Johnston) | Undifferentiated interpolation | PerformanceIntent phases (anticipation→settle) with commitment/timing; deterministic inbetween derivation | phase validation; deterministic tick→frame sequences |
| Intent drives many systems at once | Action intent informs pose, deformation, FX, camera (Dragon Ball, Spider-Verse) | Per-system ad-hoc state | PerformanceIntent typed fields consumed by compiler/deformation/FX | intent→pose→deformation integration tests |
| Motion abstraction is deliberate | Smear frames, ghosting, stepped animation (Spider-Verse; anime) | Automatic motion blur | typed smear/stepped behaviors gated by velocity/impact | gated effect tests; no silent application |

## Deformation (docs/architecture/SEMANTIC_DEFORMATION_ARCHITECTURE.md)

| Decision | Evidence | Naive approach rejected | Chosen abstraction | Validation |
|---|---|---|---|---|
| Semantic intent decoupled from geometric solver | FFD/cage/ARAP/DQS/PSD literature | Bake deformation into renderer | DeformationEnvelope (canon value + deviation + boundary) consumed by compiler; solvers below the line | envelope validation; canonical-vs-deformed determinism |
| Rigidity protects identity | ARAP local rigidity; One Piece anchors | Global uniform deformation | per-structure hard boundaries + rigid regions | boundary-violation rejection |

## StyleProgram (docs/architecture/STYLE_PROGRAM_ARCHITECTURE.md)

| Decision | Evidence | Naive approach rejected | Chosen abstraction | Validation |
|---|---|---|---|---|
| Style = typed behaviors, not labels | every production tunes axes independently | "anime"/"pixel art" strings as architecture | StyleSemantics behaviors + presets (existing); StyleProgram factorization | style preset tests; behavioral delta tests |
| Factorization with coherence risk documented | independent tunability vs incoherent combinations | Uncontrolled combinations | Line/Shading/Motion/Palette/FX/Compositing programs lowering to effective style; precedence explicit | precedence tests; canonicalization; identity-stable across styles |
| Lines bind to semantics | Spider-Verse rigged lines; WYSIWYG NPR; GG variable weight | Image-space edge detection | LineProgram entries scoped to canon structures | line-region binding; temporal stability by construction |

## Temporal Visual Identity (docs/architecture/TEMPORAL_VISUAL_IDENTITY.md)

| Decision | Evidence | Naive approach rejected | Chosen abstraction | Validation |
|---|---|---|---|---|
| Coherence = structure identity over time | NPR temporal coherence STAR (Benard 2011); surface anchoring | Per-frame regeneration | stable IDs for persistent structures (markings, lines, highlights) bound to canon structures | ID stability across frames; deliberate incoherence only via typed behaviors |
| Anchored thresholds | flicker from threshold crossing (STAR 2011) | raw thresholding | deterministic anchored/hysteresis policy where thresholds exist | determinism A/B tests |

## Fidelity (docs/architecture/VISUAL_FIDELITY_VALIDATION.md)

| Decision | Evidence | Naive approach rejected | Chosen abstraction | Validation |
|---|---|---|---|---|
| Metrics are enforceable lower bounds | Biederman 1987; SSIM (Wang 2004); production review practice | "looks better" or metric-as-aesthetic | deterministic diagnostics (existing + visual_quality) + human review artifacts | good-fixture vs degraded-fixture metric separation |
| Regression ≠ byte hashes alone | perceptual metrics literature | byte hash only | deterministic hashes + semantic geometry/perceptual regression classes | regression infra with explicit golden updates |

## Identity taxonomy (docs/IDENTITY_TAXONOMY.md, unchanged)

| Decision | Evidence | Chosen abstraction |
|---|---|---|
| Canon identity is a Visual Manifestation Identity input | identity must change with style/form/pose but never entity truth | visual_canon_identity feeds VisualIr identity under the existing Manifestation Identity taxonomy |

## Corpus-level rejected approaches (documented for the record)

- Nearest-neighbor downsampling as "pixel art" — rejected (PIXEL_ART_SEMANTICS).
- Image-space edge detection as the line authority — rejected (LINE_ART).
- Blendshape vertex lists as expression truth — rejected (FACIAL).
- Hardcoded body plans / part names — rejected (ANATOMY).
- Style as franchise-imitation presets — rejected (copyright + STYLE).
- Renderer-level entity switches — rejected (corpus-wide).
- Crossfading rendered images for transformations — rejected (TRANSFORMATION).
- Metrics pretending to be an aesthetic judge — rejected (FIDELITY).
