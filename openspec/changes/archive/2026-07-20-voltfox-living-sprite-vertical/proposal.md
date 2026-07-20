## Why

GSPL Sprites has a complete architecture (seed compilation, projection synthesis, animation, combat, transformation, living runtime, package build, verification) proven via a reference entity. However, every visual and behavioral layer is still a hardcoded placeholder: 2D renders generic circles, 2.5D has no plane-image generation, 3D uses validation triangles, the seed language lacks form/transformation/morphology declarations, and the living runtime has never been wired through combat and transformation for an actual sprite. Without a fully living sprite exercising the entire vertical — from seed declaration through semantic runtime to rendered manifestation — the system remains a collection of disconnected subsystems.

## What Changes

- **Extend the GSPL seed language** with form declarations, transformation definitions, full morphology (torso, head, ears, eyes, muzzle, tail, limbs, aura, lightning), runtime behavior attributes, and animation intents
- **Extend `compile()`** to produce typed FormDefinition, TransformationDelta, MorphologyDefinition, and AnimationIntent records in SpriteIr
- **Implement rig-driven 2D Voltfox synthesis**: semantic part renderer producing distinct frame sprites for each form (Idle, Running, Jumping, Attack, Special, Hurt), with collision shapes, ability timing overlays, and aura VFX
- **Implement 2.5D Voltfox synthesis**: plane-image generation for each form with depth channels, parallax layers
- **Implement 3D Voltfox synthesis**: meaningful mesh geometry (box/cylinder/capsule primitives for each body part), bone skeleton, skinning weights, linear-blend-skin animation, glTF export with animation clips and skin
- **Wire the living runtime** through combat → transformation → manifestation for Voltfox: utility-AI behavior selects form based on combat state, directional lightning applies ability effects, transformation triggers form-switch with VFX
- **Add deterministic headless evidence mode**: run a full Voltfox lifetime scenario, record the deterministic event + state trace, verify against recorded oracle
- **Build a package-driven reference preview** (minimal SDL2/sdl2/glut window that loads a package, renders the current manifestation, and drives the living runtime)
- **Add comprehensive acceptance tests** covering all new capabilities, property tests for invariants, and a fuzz harness for seed parsing

## Capabilities

### New Capabilities
- `seed-forms`: Form declarations, transformation definitions, and full morphology in the GSPL seed language
- `compile-forms`: Compilation of forms/transformations/morphology into typed SpriteIr records
- `synthesis-2d-voltfox`: Rig-driven 2D part rendering producing Voltfox sprites with forms, collision, ability timing
- `synthesis-25d-voltfox`: Plane-image generation for 2.5D Voltfox with depth and parallax
- `synthesis-3d-voltfox`: Meaningful 3D geometry, skeleton, skin, animation, and glTF export for Voltfox
- `living-combat-wiring`: Integration wiring the living runtime's utility-AI with combat commands and transformation triggers
- `headless-evidence`: Deterministic headless lifetime scenario with event trace oracle
- `preview-package`: Package-driven preview executable loads and renders a sprite at runtime
- `fuzz-parsing`: Fuzz harness for seed parsing robustness

### Modified Capabilities
<!-- No existing specs to modify -->

## Impact

- `include/gspl_sprites/core.hpp`: Extended SpriteSeed grammar, SpriteIr has new fields (forms, transformations, morphology, animation_intents)
- `include/gspl_sprites/synthesis.hpp`: New overloads for VD-2D/2.5D/3D synthesis, part renderers
- `include/gspl_sprites/combat.hpp`: May need extensions for directional lightning integration
- `include/gspl_sprites/runtime_persistence.hpp`: Integration points with combat state
- `src/` compilation units: Significant additions to core.cpp, synthesis.cpp; new files for 2D/2.5D/3D synthesis, living runtime wiring, preview
- `CMakeLists.txt`: New targets for preview executable, fuzz harness; additional test registrations
- `examples/voltfox.sprite`: Complete rewrite with forms, transformations, morphology, runtime attributes
- `tests/`: Extensive new test suite (acceptance, property, fuzz)
- `packages/`: Example portable sprite packages for verification
