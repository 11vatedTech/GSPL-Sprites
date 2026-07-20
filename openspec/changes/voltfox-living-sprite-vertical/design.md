# Design: Voltfox Living-Sprite Vertical

## Architecture

The Voltfox pipeline maintains the existing architecture with targeted enhancements:

```
voltfox.sprite → parse_seed() → validate() → canonicalize() → compile() → SpriteIr
  → synthesize_unified_entity(SpriteIr) → SynthesisResult
    → 2D: Full RGBA frames, atlas, channels, collision, timing, events
    → 2.5D: Rendered plane images, parallax, depth collision
    → 3D: Proper mesh geometry, skeleton, skinning, animations, glTF
  → LivingRuntimeState (authoritative)
    → manifests 2D/2.5D/3D from single identity
    → behavior cycle (perceive → decide → act)
    → combat (directional lightning, damage, status)
    → transformation (ascend/descend with progressive frames)
  → Immutable artifact graph (content-addressed, dependency-tracked)
  → Portable package (all artifacts, closure-verified)
  → Preview (loads package, displays all reps, headless mode)
```

### Key Design Decisions

1. **Sprite IR as typed lowering target**: `compile()` produces a fully populated `SpriteIr` that is the exclusive input to synthesis. No synthesis function reads the seed directly. All semantic data is resolved and validated in `SpriteIr`.

2. **Storm form as morphological delta**: `TransformationDelta` for `ascend` contains `morphology_deltas` that override specific morphology parts (ears elongate, tail forks, electrical markings added, aura emissive). The `descend` delta reverses these.

3. **Frame distinctness via hash**: Each generated frame carries a SHA-256 content hash computed from the rendered RGBA pixels. Tests assert that frames with different semantic pose IDs produce different hashes.

4. **2.5D plane rendering**: The existing `projection25d` structures are extended so that `synthesize_projection25d_voltfox()` produces actual `ImageRgba8` buffers per depth plane, not just metadata. RGBA images are computed from the same morphology-driven part renderer used for 2D.

5. **3D mesh topology**: Primitive generators (`add_box`, `add_sphere`, etc.) are upgraded to produce nondegenerate triangles with computed normals and UVs. Collision geometry is stored alongside render mesh.

6. **Authoritative identity**: `LivingRuntimeState` gains an `identity` field of a new `EntityStateIdentity` struct containing all state fields that define the identity. Serialization captures/restores this identity. Manifestations carry a reference to it.

7. **Artifact graph**: `AssetGraph` is extended with formal artifact nodes keyed by SHA-256 content hash. `build_package()` tracks all artifacts. Selective invalidation tests verify that changing one input changes only dependent outputs.

8. **Resource limits**: Each limit is defined as a constexpr in its respective header. `validate()` and `compile()` enforce limits with diagnostics. Tests verify boundary acceptance and over-boundary rejection.

## Modules

### 1. Seed Language (`core.hpp`, `core.cpp`)
- Add `base` and `storm` forms
- Add `ascend` and `descend` transformations
- Storm morphology deltas (ear/tail geometry, electrical markings, aura)
- Schema version handling
- Backward compatibility

### 2. Sprite IR (`core.hpp`, `core.cpp`)
- Expand `SpriteIr` with complete semantic fields
- Immutable, versioned, hashable, serializable

### 3. 2D Synthesis (`synthesis.hpp`, `synthesis.cpp`)
- Upgrade morphology-driven renderer with shape primitives
- Pixel hash per frame
- Distinctness assertion
- Collision, timing, events

### 4. 2.5D Synthesis (`synthesis.hpp`, `synthesis.cpp`)
- Render RGBA plane images per depth layer
- Content-addressed plane assets

### 5. 3D Synthesis (`synthesis.hpp`, `synthesis.cpp`)
- Proper mesh topology (normals, UVs)
- Enhanced 13-bone skeleton

### 6. Living Runtime (`living_runtime.hpp`, `living_runtime.cpp`)
- EntityStateIdentity struct
- Full behavior lifecycle
- Perception → decision → action

### 7. Combat (`combat.hpp`, `combat.cpp`)
- Complete directional lightning pipeline
- Projectile state, collision, damage, status

### 8. Artifact Graph (`domain.hpp`, `domain.cpp`)
- Content-addressed artifact nodes
- Dependency tracking
- Selective invalidation

### 9. Package (`package.hpp`, `package.cpp`)
- Vertical closure validation
- Reproducibility test
- Corruption rejection

### 10. Preview (`preview.cpp`)
- Package-driven loading
- Headless acceptance mode

### 11. Resource Limits (across all modules)
- Definitions, enforcement, boundary tests

### 12. Testing
- Property tests, mutation tests, fuzzing
