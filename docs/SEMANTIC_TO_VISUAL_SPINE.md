# GSPL Sprites — Semantic-to-Visual Spine

**Status:** Implemented (first vertical)
**Frozen SHA:** 04bee74d793d7ac0ba8b304c29741bff1feba665

## Authority Chain

```
GSPL Source / AuthoringProject
  → CanonicalEntity (semantic authority)
    → gspl::sprites::SpriteIr (canonical production IR)
      → synthesis / runtime / combat / transformations
        → Living Visual Package
          → LivingPackageViewer (verified artifact consumer)
```

## Representations

| Representation | Role | Authority |
|---|---|---|
| AuthoringProject | human intent graph | CANONICAL (intent) |
| CanonicalEntity | semantic entity truth | CANONICAL (semantic) |
| SpriteSeed | serialized interchange | COMPATIBILITY |
| gspl::SpriteIr | compiler interchange | COMPATIBILITY |
| gspl::sprites::SpriteIr | production IR | CANONICAL (production) |
| Living Visual Package | sealed artifact set | CANONICAL (artifact) |
| Runtime state | live gameplay | AUTHORITATIVE (per tick) |
| Target packages | engine exports | DERIVED |

## Key Architectural Decisions

1. **One semantic authority:** CanonicalEntity is the single source of truth across 2D/2.5D/3D/manifestations.
2. **IR consolidation:** Strategy C — designate `gspl::sprites::SpriteIr` canonical, `gspl::SpriteIr` compatibility adapter.
3. **Seven identities, not one:** Semantic entity, seed snapshot, canonical IR, runtime state, visual manifestation, artifact, package — deliberately distinct.
4. **Control ≠ Entity:** One entity + multiple controllers (player, living, AI, scripted, network, cinematic, hybrid).
5. **Rights centralization:** Single `rights_class_to_string`/`rights_class_from_string`/`lower_rights_class` authority.
6. **Morphology centralization:** Single `canonicalize_morphology_*` family with byte-stable preimages.

## Implementation Files

- `include/gspl_sprites/rights.hpp`, `src/rights.cpp` — rights authority
- `include/gspl_sprites/morphology.hpp`, `src/morphology.cpp` — morphology authority
- `include/gspl_sprites/identity.hpp` — identity taxonomy
- `include/gspl_sprites/control.hpp`, `src/control.cpp` — control authority
- `include/gspl/ir_authority.hpp` — IR authority documentation
- `include/gspl_sprites/viewer_model.hpp`, `src/viewer_model.cpp` — package viewer
- `tests/semantic_spine_tests.cpp` — 80 assertions
