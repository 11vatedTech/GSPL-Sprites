# Complete Portable Package

## Requirements
1. Contains: seed, SpriteIR, forms, transformations, behavior, combat, ability, runtime def, persistence schema, 2D/2.5D/3D assets, animation, collision, rights, provenance, checksums
2. Deterministic file ordering for reproducible package identity
3. Complete dependency closure: all referenced IDs resolve
4. No unresolved or duplicate artifact IDs
5. Each artifact hash verified against manifest
6. Safe paths (no directory traversal)
7. Archive-safe (extract without data loss)

## Scenarios
1. Two builds from identical inputs → identical package identity hash
2. Corrupt each major artifact category → verification rejects
3. Missing dependency → closure error reported
4. Package contains all required vertical artifacts
5. Extract and re-verify produces same result
