## Context

The GSPL Sprites codebase has a complete pipeline: SpriteSeed → `parse_seed()` → `SpriteIr` → `compile()` → `SpriteIr` (enriched) → `synthesize_*()` → projections + visual-set → `build_package()`. The previous reference-entity change proved end-to-end seed-to-package correctness but relied on hardcoded values for everything visual (generic circles for 2D, empty structure for 2.5D, a 2-triangle validation mesh for 3D, a hardcoded palette/rig). The living runtime (utility-AI, goals, actions, perception, memory), combat (abilities, effects, statuses, events, commands), transformation (forms, deltas, form-gated abilities), and persistence (serialization, replay) all exist but have never been wired together into a single cohering lifecycle. This design describes how to connect every subsystem into the first fully living GSPL sprite.

## Goals / Non-Goals

**Goals:**
- Extend the seed language so Voltfox can declare its forms (six fox forms), transformations (all 30 form pairs), full morphology (11 body parts), and runtime attributes
- Extend `compile()` so FormDefinition, TransformationDelta, MorphologyDefinition, and AnimationIntent are first-class fields in SpriteIr
- Replace the 2-triangle validation mesh with a real 8-primitive Voltfox mesh for 3D (torso box, head box, 2 ear caps, eye spheres, muzzle cone, tail capsule, limb cylinders) with bone skeleton, skinning, and glTF animation export
- Generate real 2.5D plane images (RGBA with depth channels) for each form of Voltfox
- Replace the generic-circle 2D synthesis with rig-driven semantic part rendering producing distinct form sprites with collision shapes
- Wire the living runtime through combat → transformation → manifestation so Voltfox has behavior-driven form switching, directional lightning as an ability, and a complete deterministic lifecycle
- Add deterministic headless evidence: run a full-scenario trace, record events + state, verify against oracle
- Build a package-driven preview executable using SDL2 for interactive rendering
- Add a fuzz harness for seed-parsing robustness

**Non-Goals:**
- Not adding network access or cloud services
- Not building a game engine — the preview is a minimal viewer, not a playable game
- Not supporting author-time edit-and-continue (the preview loads a static package)
- Not adding audio, music, or cinematic cutscenes
- Not implementing a GUI editor
- Not supporting multi-entity scenes — preview shows one sprite at a time

## Decisions

### 1. Seed grammar extensions use INI-like block syntax (not YAML/JSON)
The existing seed grammar is key=value with bracket sections (`[identity]`, `[rights]`, `[ability.*]`). We extend this with `[form.*]` for form declarations (with fields: `forms=`, `transformations=`, `morphology.`), `[transformation.*]` sections, and `[morphology.*]` sections with part-specific subfields. This keeps the language uniform and avoids a second parser.

**Alternatives considered:** Switching to JSON/YAML would require rewriting `parse_seed()` entirely. Sticking with INI-like is minimal delta and preserves the "generative language" design philosophy.

### 2. Form definitions carry explicit transformation mappings
Each form lists which transformations it can undergo (`transformations=` field referencing transformation section names), and each transformation declares its `from_form=` and `to_form=`. This enables compile-time validation of the complete form graph and prevents dangling transformation references.

### 3. Morphology uses semantic part names with sub-parameter blocks
Voltfox has 11 body parts: torso, head, left_ear, right_ear, left_eye, right_eye, muzzle, tail, left_front_leg, right_front_leg, aura. Each part has position/size/color/rotation parameters. For 3D, these map to box/cylinder/capsule/sphere/cone primitives. For 2D, they map to filled shapes. For 2.5D, they map to layered planes. A single morphology definition drives all three projections.

### 4. Synthesis functions for 2D/2.5D/3D are separate overloads sharing the morphology
`synthesize_projection2d_voltfox()`, `synthesize_projection25d_voltfox()`, `synthesize_projection3d_voltfox()` each read the same `MorphologyDefinition` and `SynthesisPalette` but produce different projection types. They are not templated because each output type has completely different geometry and rendering.

### 5. 3D mesh uses indexed primitives with shared vertex buffer
All 8 body parts are assembled into a single indexed mesh with per-vertex bone weights (0 to 4 influences), stored in a flat vertex buffer. This is compatible with glTF accessor-based storage and GPU skinning.

### 6. Skeleton uses a simple hierarchy with 13 bones
Bones: root → spine → neck → head | → left_ear, right_ear, tail, left_shoulder → left_arm, right_shoulder → right_arm. Each bone has a bind-pose transform relative to parent. The skin maps mesh vertices to bone indices.

### 7. 2D frames are rendered via a raster part assembler
Instead of pre-authored PNGs, the 2D synthesis draws each morphology part as a filled pixel buffer (ellipse for most parts, triangle for ears, extended ellipse for tail, line + glow for lightning), composites them in z-order, and outputs SpriteFrame pixel buffers with associated collision shape data.

### 8. Living runtime integration uses the existing UtilityAI + CombatManifestation
The existing `LivingSpriteRuntime` already has a utility-AI system with goals, actions, perception, and memory. The wiring adds:
- A combat perception sensor that translates combat events into utility-score modifiers
- Form actions that trigger transformations when utility thresholds are crossed
- A manifestation driver that updates the active projection when form changes

### 9. Headless evidence mode uses a deterministic RNG and fixed tick rate
The evidence scenario runs at a fixed tick interval (16ms) with a seeded RNG, records every event and state snapshot, and produces a JSON trace. A test verifies that this trace matches a committed oracle file.

### 10. Preview uses SDL2 (not GLFW or Qt)
SDL2 is the most portable minimal windowing library. It handles window creation, input events, and texture presentation. 3D rendering uses software rendering (not OpenGL) to avoid GPU-state complexity in the project.

## Risks / Trade-offs

- **[Compile-time growth]** Adding forms/transformations/morphology to SpriteIr increases compile output size. Mitigation: Use flat buffers, not deeply nested objects.
- **[3D quality gap]** The primitive-based 3D mesh won't look as polished as an artist-authored model. Mitigation: The authoritative representation is semantic; 3D is a projection. Quality targets clarity and recognizability, not artistry.
- **[No GPU rendering]** Software rendering limits frame rate on complex scenes. Mitigation: Preview is for demonstration and verification, not production rendering.
- **[SDL2 dependency]** Adding SDL2 requires users to have it installed. Mitigation: The preview is optional (separate CMake target, `BUILD_PREVIEW` option). Core library does not depend on SDL2.
- **[Living runtime determinism]** The utility-AI uses floating-point scores; floating-point determinism across compilers is not guaranteed. Mitigation: Use fixed-point or integer scores for the evidence oracle; only the headless scenario requires cross-build determinism.
- **[Fuzz harness complexity]** A custom fuzz harness needs input mutation logic. Mitigation: Provide a simple round-trip fuzz (parse → canonicalize → serialize → parse → compare). Full AFL/libFuzzer integration is deferred.
