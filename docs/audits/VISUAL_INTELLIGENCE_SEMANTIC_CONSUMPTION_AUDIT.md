# Visual Intelligence — Semantic Consumption Audit

Status: **current as of commit `abdf9f8` + the Functional Authority Closure working tree (uncommitted)**
Branch: `feature/first-living-sprite-2d`

This audit classifies every semantic field in the Visual Intelligence type
surface by authority level. A field is `production-authoritative` only when
it is **causally consumed** by the compiler or raster, **validated**,
**represented in the correct canonical identity** (when identity-affecting),
**round-trip safe** (when serialized), and **covered by a causal or hostile
test**. Fields that merely serialize or validate are never described as
implemented behavior.

## Authority classification legend

| Class | Meaning |
|---|---|
| `production-authoritative` | Causally consumed in production compilation/rendering; changing it changes output or validation outcome. |
| `validation-only` | Consumed by validation paths; does not alter construction/rendering. |
| `serialization-only` | Serialized/parsed/round-trip tested; no behavioral effect. |
| `identity-only` | Participates in canonical identity (manifestation identity); may not directly alter pixels. |
| `quality-only` | Consumed by quality diagnostics (visual_quality / fidelity metrics). |
| `future-reserved` | Architecture reserved for a later phase; explicitly not claimed as implemented behavior. |
| `dead/unconsumed` | Not consumed anywhere; must be removed or upgraded before any completion claim. |

---

## 1. `VisualCanon` (visual_canon.hpp)

| Field | Class | Consumption |
|---|---|---|
| `schema` | `serialization-only` | Version tag; validated on parse. |
| `entity_id`, `name` | `identity-only` | Participate in `visual_canon_identity`; not geometry. |
| `rights_class`, `provenance` | `serialization-only` | Provenance readability; excluded from identity by policy (documented). |
| `forms` | `production-authoritative` | Form selection drives `canon_to_morphology(form_id)` and per-form palettes. |
| `structures` | `production-authoritative` | Primary construction input; solved by `solve_construction_constraints`, lowered by `canon_to_morphology`. |
| `construction` | `production-authoritative` | Relational Construction System solver derives geometry (anchor/size_ratio/symmetry/orientation/chain). Causal tests mutate constraints and assert geometry change. |
| `proportions` | `production-authoritative` (generative) / `validation-only` (measurement) | `derived` size rules construct geometry (`target = denominator * preferred`); distance-based rules are validation/identity-only. |
| `landmarks` | `production-authoritative` | Resolved by `resolve_landmark_position`; consumed by pose/gaze/support/identity logic. |
| `silhouette_features` | `quality-only` + `identity-only` | Declared masses/anchors/protrusions validated against realized silhouette; silhouette-anchor invariants fail closed on violation. |
| `surfaces` | `validation-only` | Declared surface roles; referenced by material/feature bindings. |
| `materials` | `production-authoritative` | Material class/id bindings flow into `MaterialSemantics` and raster material response. |
| `color_regions` | `production-authoritative` | Color-role topology consumed by palette assembly and raster fill. |
| `facial_regions` | `production-authoritative` | `expression_motions` resolves features to parts; drives eye aperture/gaze/rotation/emission as typed motions BEFORE the deformation gate. |
| `markings` | `production-authoritative` | Lowered into IR markings; rendered with style-interpreted drawing. |
| `attachments` | `validation-only` | Declared socket/attachment points; validated for dangling refs. |
| `deformation_envelopes` | `production-authoritative` | `enforce_deformation_envelopes` clamps/composes aggregate deviation per typed axis (translation/rotation/scale); hard boundaries fail closed. |
| `resolution_features` | `production-authoritative` (Semantic LOD) | `apply_semantic_lod` executes authored priority/importance/min_resolution/substitution/merge/omission rules at target resolutions. |
| `identity_invariants` | `production-authoritative` | `check_identity_invariants` enforces typed severity (advisory/soft/hard) against realized morphology; hard violations fail the compile. |
| `gaze_driver` | `production-authoritative` | Validated (referenced structure must exist), canonicalized, parsed, included in `visual_canon_identity`; consumed by pose solver gaze orientation. |
| `attention_driver` | `production-authoritative` | Same closed validation + identity participation; drives attention/orientation channel. |
| `support_policy` | `production-authoritative` | Typed closed vocabulary (`auto_/grounded/flight/buoyant/free`); unknown values fail closed; consumed by `analyze_balance`. |
| `emotion_responses` | `production-authoritative` | Data-driven emotion → expressive channel overrides; consumed by `expression_motions` for both organism and mechanical fixtures. |
| `base_palette`, `form_palettes` | `production-authoritative` | Deterministic palette assembly; color roles never vanish. |

## 2. `ConstructionConstraint` (visual_canon.hpp)

| Field | Class | Consumption |
|---|---|---|
| `id` | `identity-only` | Deterministic iteration order (constraint id + target id). |
| `kind` | `production-authoritative` | Anchor/size_ratio/symmetry/orientation/chain each solve deterministically. |
| `structure`, `reference` | `production-authoritative` | Target and reference ids; validated to exist. |
| `axis` | `production-authoritative` | Typed `ConstructionAxis` (x/y/z) — **distinct from property domain**; position vs size constraints on the same axis do not conflict (typed property domains in solver). |
| `reference_axis` | `production-authoritative` | Optional distinct reference measure axis; enables cross-axis ratios (target size_x from reference size_y). |
| `factor`, `offset` | `production-authoritative` | Deterministic derived geometry. |
| `hard` | `production-authoritative` | Hard contradictions fail closed with precise diagnostics; soft resolve via deterministic precedence. |
| `scope` | `production-authoritative` | Form-restricted constraints apply only to the selected form. |

## 3. `ProportionRule` (visual_canon.hpp)

| Field | Class | Consumption |
|---|---|---|
| `id` | `identity-only` | Deterministic ordering + invariant references. |
| `numerator`, `denominator` | `production-authoritative` (generative) / `validation-only` (measurement) | Measure strings resolve through `measure_value`. |
| `preferred` | `production-authoritative` | Generative rules derive target = denominator × preferred; mutation of preferred changes constructed morphology (causal tests). |
| `min`, `max` | `validation-only` | Range validation of measured ratios. |
| `deformation_min`, `deformation_max` | `validation-only` | Deformation-range validation. |
| `hard` | `production-authoritative` | Contradiction/range failure severity. |
| `derived` | `production-authoritative` | `size:<part>:<axis>` target driven by the rule; axis grammar strictly parsed (no silent Z fallback). |
| `derived_hard` | `production-authoritative` | Marks derived value identity-critical. |
| `scope` | `validation-only` | Form/style restriction. |

## 4. `IdentityInvariant` (visual_canon.hpp)

| Field | Class | Consumption |
|---|---|---|
| `id` | `identity-only` | Diagnostic reference. |
| `kind` | `production-authoritative` | proportion/landmark_ordering/landmark_existence/silhouette_anchor/attachment/color_role_topology/material_truth/marking_topology/hard_boundary each enforced by `check_identity_invariants`. |
| `refs` | `production-authoritative` | Target ids; dangling refs fail validation. |
| `tolerance` | `production-authoritative` | Deviation tolerance for soft/advisory checks. |
| `severity` | `production-authoritative` | **Typed `InvariantSeverity` (advisory/soft/hard) — single authority.** `hard` field removed from the schema; `ValidationResult::ok()` derives from `DiagnosticSeverity::error`, never from string parsing. Advisory/soft never fail; hard fails closed. |
| `scope` | `validation-only` | Form restriction. |

## 5. `DeformationEnvelope` (visual_canon.hpp)

| Field | Class | Consumption |
|---|---|---|
| `structure_id` | `production-authoritative` | Envelope lookup key. |
| `allowed_deviation` | `production-authoritative` | Legacy scalar default for all axes when typed axis values are 0. |
| `allowed_translation` / `allowed_rotation` / `allowed_scale` | `production-authoritative` | **Typed axes with correct units**: world units, degrees, ratio. Aggregate effective deviation is composed first, then enforced per axis. |
| `action_scale` / `expression_scale` / `style_scale` / `transformation_scale` | `production-authoritative` | Context multipliers applied to typed channels in the composition stage. |
| `hard_translation` / `hard_rotation` / `hard_scale` | `production-authoritative` | Hard boundaries; exceeding fails closed. |
| `rigid` | `production-authoritative` | Identity-critical: never exceeds boundary. |
| `volume_preserving` | `production-authoritative` | Volume-conservation request honored during deformation. |

## 6. `PerformanceIntent` (performance_intent.hpp)

| Field | Class | Consumption |
|---|---|---|
| `schema` | `serialization-only` | Version tag. |
| `action` | `production-authoritative` | Drives phase/motion/action fields in `pose_to_performance_state`; motion-phase timing. |
| `phase` | `production-authoritative` | Anticipation/extension/etc. changes derived motion scaling and acting (causal test: phase change → anticipation/extension relationship changes). |
| `commitment` | `production-authoritative` | Scales derived motion amplitude (causal test: commitment change → solved pose changes predictably). |
| `force_direction` | `production-authoritative` | Normalized on use; drives derived lean/strike direction and facing. |
| `force_magnitude` | `production-authoritative` | Scales derived motion amplitude. **Does NOT invent FX phenomenon.** |
| `emotion` | `production-authoritative` | Resolved through canon `emotion_responses` → typed expressive channel overrides (organism + mechanical, same machinery). |
| `balance` | `production-authoritative` | Drives `analyze_balance` support expectation + derived CoM shift. |
| `gaze_target` | `production-authoritative` | Resolved through **declared gaze driver** (canon `gaze_driver`), never smallest-feature heuristics. |
| `gaze_direction` | `production-authoritative` | World gaze direction; resolved into solved head/gaze orientation. |
| `attention` | `production-authoritative` | Consumed via declared `attention_driver` orientation channel. |
| `timing_cadence` | `production-authoritative` | Spacing/timing multiplier flowing into performance timing fields. |

## 7. `KeyPose` (performance_intent.hpp)

| Field | Class | Consumption |
|---|---|---|
| `id`, `action`, `phase` | `production-authoritative` | Deterministic pose selection/identity. |
| `commitment` | `production-authoritative` | Motion amplitude override. |
| `line_of_action_direction` / `line_of_action_curvature` | `production-authoritative` | Consumed by the acting solver (gesture axis, curvature). |
| `center_of_mass` | `production-authoritative` | Consumed by `analyze_balance` when authored. |
| `support_contacts` | `production-authoritative` | **Explicit active support contacts**; support polygon derives from actual contacts + declared `support_capable` structures. Ears/tails/antennae are never inferred as support. |
| `balance` | `production-authoritative` | Balance expectation. |
| `gaze_direction` | `production-authoritative` | Gaze resolution override. |
| `force_direction`, `force_magnitude` | `production-authoritative` | Derived motion inputs. |
| `emotion` | `production-authoritative` | Same canon emotion-resolution path as intent emotion. |
| `silhouette_goal` | `production-authoritative` | Priority landmark ids feed the acting solver's silhouette objective. |
| `motions` | `production-authoritative` | **Authoritative overrides, not appends**: motions targeting a part replace the derived motion for that semantic channel (deterministic composition modes; no insertion-order dependence). |

## 8. `StyleSemantics` (style.hpp)

| Field | Class | Consumption |
|---|---|---|
| `schema` | `serialization-only` | Version tag. |
| `shape_abstraction` | `production-authoritative` | Construction simplification factor consumed by the compiler. |
| `proportion_exaggeration` | `production-authoritative` | Proportion bias applied at construction. |
| `contour` | `production-authoritative` | Contour behavior drives geometry generation. |
| `line_weight`, `line_taper` | `production-authoritative` | Outline raster parameters. |
| `outline_selection` | `production-authoritative` | Which contour is outlined. |
| `corner_sharpness`, `curve_smoothness` | `production-authoritative` | Geometry smoothing parameters. |
| `palette_policy`, `value_grouping`, `saturation_behavior` | `production-authoritative` | Palette transformation pipeline. |
| `shadow_model` | `production-authoritative` | gradient/cell/hard shading selection. |
| `highlight_model` | `production-authoritative` | Highlight response selection. |
| `texture_density`, `detail_density` | `production-authoritative` | Detail/texture density. |
| `edge_softness` | `production-authoritative` | Edge falloff. |
| `aa_policy` | `production-authoritative` | analytic/none quantization; consumed by raster AA. |
| `pixel_quantization` | `production-authoritative` | Integer/grid quantization for pixel-constrained (documented as quantization-level, NOT cluster-semantic pixel art). |
| `layer_compositing` | `production-authoritative` | Compositing operation per layer. |
| `band_count` | `production-authoritative` | Cel band quantization; **validated, canonicalized, in `style_identity` and `visual_ir_identity`**; changing it changes style identity AND rendered manifestation (cell shading). |
| `max_colors` | `production-authoritative` | Palette cap; **validated, canonicalized, in `style_identity`/`visual_ir_identity`**; when the palette exceeds the cap the raster deterministically remaps and pixels change. |

All StyleSemantics fields participate in `validate_style`, `canonicalize_style`,
and `style_identity`; renderer-affecting fields therefore participate in
`visual_ir_identity` (no renderer-consumed field may change pixels while
leaving identity unchanged — regression-tested).

## 9. `StyleProgram` (style_program.hpp)

| Field | Class | Consumption |
|---|---|---|
| `schema`, `name` | `serialization-only` | Version/preset name. |
| `line`, `shading`, `motion`, `palette`, `fx`, `compositing` | `production-authoritative` | Lowered deterministically by `effective_style()` into `StyleSemantics`, which the compiler/raster consume (see §8). |
| `patches` | `production-authoritative` | Precedence-ordered style patches applied by `compose_style`. |

## 10. `TemporalIdentity` (temporal_identity.hpp)

| Field | Class | Consumption |
|---|---|---|
| `schema` | `serialization-only` | Version tag. |
| `entity_id`, `current_frame` | `production-authoritative` | Registry identity/frame bookkeeping. |
| `frames` | `production-authoritative` | Bounded ring; `register_temporal_frame` matches persistent structures by id+kind+anchor across frames. |
| `live` | `production-authoritative` | Live structure set; `structures_anchored_to` lookup. |
| `PersistentStructure.*` | `production-authoritative` | Correspondence authority for parts/landmarks/markings/material regions/expressive features/FX emitters (kind + anchor matching). |
| `temporal_inconsistency` | `quality-only` | Jitter metric between frames. |
| Identity functions | `identity-only` | Registry identity. |

Production integration: the compiler emits `VisualIr.temporal_*_labels`
(stable semantic-id → deterministic temporal label) for parts, landmarks,
markings, material regions, expressive features and FX emitters, so the same
semantic structure keeps its identity across poses while its transform
changes. Transient FX may appear/disappear per explicit lifetime semantics.

## 11. `FxState` / `FxEffect` / `FxRequest` (fx_semantics.hpp)

| Field | Class | Consumption |
|---|---|---|
| `FxRequest.phenomenon`, `energy` | `production-authoritative` | **Typed phenomenon/energy authority.** FX never derives from force-magnitude heuristics (impact > 0.5 does not invent electricity); a high-impact kinetic event stays kinetic impact. |
| `FxRequest.source_part` / `target_part` | `production-authoritative` | Semantic emitters/receivers; validated to exist in the resolved morphology (missing source fails closed — no `"torso"` anatomy fallback). |
| `intensity`, `temperature`, `charge`, `velocity`, `branching`, `persistence`, `emission`, `color_role` | `production-authoritative` | Scaled into `FxEffect`; force/commitment may scale intensity but never invent phenomenon. |
| `FxState.*` | `production-authoritative` | Validated (`validate_fx_state`), canonicalized, identity-tracked; consumed by raster FX draw parameters via `interpret_fx_effect`. |
| `FxDrawParams.*` | `production-authoritative` | Deterministic style interpretation consumed by the raster. |

## 12. `CinematicSpec` (cinematic.hpp)

| Field | Class | Consumption |
|---|---|---|
| `schema`, `entity_id` | `serialization-only` | Version/tag. |
| `camera.*`, `impact_staging`, `focal_hierarchy` | `future-reserved` | **Explicitly future-ready architecture.** Validated, canonicalized, identity-tracked, but the current 2D projection does not yet consume camera/framing. No claim of active manifestation behavior is made. |

## 13. `VisualIr` (visual_ir.hpp)

| Field | Class | Consumption |
|---|---|---|
| `schema` | `serialization-only` | Version tag. |
| `entity_identity`, `form_id` | `identity-only` | Manifestation identity + form. |
| `projection`, `lighting` | `production-authoritative` | Raster canvas + lighting model. |
| `morphology` | `production-authoritative` | Resolved, enforced geometry; identity-sensitive fields frozen after final gate. |
| `style` | `production-authoritative` | StyleSemantics (§8) consumed by raster. |
| `palette` | `production-authoritative` | Resolved palette consumed by raster. |
| `materials` | `production-authoritative` | MaterialSemantics list consumed by raster. |
| `performance` | `production-authoritative` | Enforced PerformanceState; drives motion/phase/facing. |
| `layer_order`, `channel_requests` | `production-authoritative` | Raster layer order + channel maps. |
| `temporal_*_labels` | `production-authoritative` | Persistent semantic identity (§10); validated/canonicalized/identity-included. |
| `fx_state` | `production-authoritative` | Resolved typed FX (§11); validated before IR acceptance. |

All VisualIr state participates in `validate_visual_ir`, `canonicalize_visual_ir`,
and `visual_ir_identity` — including `temporal_*_labels` and `fx_state`, so no
renderer-consumed field can change pixels while identity stays unchanged.

## 14. `VisualFeature` (resolution features, visual_canon.hpp)

| Field | Class | Consumption |
|---|---|---|
| `id`, `structure_ref` | `production-authoritative` | LOD feature identity; refs validated. |
| `semantic_priority` | `production-authoritative` | Priority 0 = identity-critical; never dropped by LOD. |
| `recognition_importance` | `production-authoritative` | Preservation threshold. |
| `min_resolution` | `production-authoritative` | Threshold for authored rule execution. |
| `substitution_rule` (`merge:<parent>`/`omit`) | `production-authoritative` | Executed when a feature becomes too small. |
| `merge_rule`, `omission_rule` | `production-authoritative` | Executed per authored rules. |

Semantic LOD is **rule execution, not visibility culling**: identity-critical
features never disappear merely because `min_resolution` was crossed; the
authored rule decides omit/merge/substitute/preserve. Low-resolution Voltfox
still preserves declared recognition anchors and silhouette topology (tested).

## 15. `VisualQuality` (visual_quality.hpp)

| Field | Class | Consumption |
|---|---|---|
| `QualityReport.*` metrics | `quality-only` | Objective lower-bound diagnostics: silhouette disconnection, landmark jitter, contour correspondence, marking drift, frame duplication, palette violations, line inconsistency, prohibited AA, pixel-cluster instability. Enforceable; never claim artistic excellence. |
| `evaluate_frame_pair`, region/consistency functions | `quality-only` | Deterministic two-frame quality evaluation. |

---

## Consolidated summary

| System | Production | Validation | Identity | Quality | Future | Dead |
|---|---|---|---|---|---|---|
| VisualCanon | ✓ (construction, forms, palettes, envelopes, invariants, LOD, drivers, support, emotion) | ✓ (ranges, refs) | ✓ (full canon identity) | ✓ (silhouette features) | — | — |
| ConstructionConstraint | ✓ | ✓ | ✓ | — | — | — |
| ProportionRule | ✓ (generative derived) | ✓ | ✓ | — | — | — |
| IdentityInvariant | ✓ (typed severity) | ✓ | ✓ | — | — | — |
| DeformationEnvelope | ✓ (typed axes, aggregate) | ✓ | ✓ | — | — | — |
| PerformanceIntent | ✓ (all fields causal) | ✓ | ✓ | — | — | — |
| KeyPose | ✓ (overrides authoritative) | ✓ | ✓ | — | — | — |
| StyleSemantics | ✓ (all fields) | ✓ | ✓ | — | — | — |
| StyleProgram | ✓ (lowered) | ✓ | ✓ | — | — | — |
| TemporalIdentity | ✓ (correspondence) | ✓ | ✓ | ✓ (inconsistency) | — | — |
| FxState/Request | ✓ (typed phenomenon) | ✓ | ✓ | — | — | — |
| CinematicSpec | — | ✓ | ✓ | — | ✓ (camera/framing) | — |
| VisualIr | ✓ | ✓ | ✓ | — | — | — |
| VisualFeature | ✓ (LOD rules) | ✓ | ✓ | — | — | — |
| VisualQuality | — | — | — | ✓ | — | — |

**No field is classified `dead/unconsumed`.** Fields previously at risk
(emotion, attention, timing_cadence, support contacts, center of mass, line
of action, silhouette goal, gaze target) are either causally consumed
(§6–§7) or explicitly classified `future-reserved` (CinematicSpec camera
fields, §12).

## Enforcement

- `apply_expression` (post-gate morphology mutation) has been **removed**
  (declaration and definition). All expression flows through
  `expression_motions` as typed PartMotions composed before the final
  deformation/identity gate.
- Direct `PerformanceState`, impact, style deformation, transformation
  deformation and manual morphology mutation all converge on the same
  aggregate composition → envelope enforcement → identity validation
  boundary (hostile bypass tests in `tests/visual_intelligence_tests.cpp`).
- No fixture/entity name appears in production compiler or raster source
  (verified by source scan; fixtures differ only by canon data).
