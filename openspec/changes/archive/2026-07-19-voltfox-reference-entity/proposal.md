## Why

GSPL Sprites has all subsystems implemented (seed parsing, visual-set loading, projection synthesis, animation, combat, transformation, living runtime, package building, verification) but lacks a complete end-to-end reference entity that exercises the full production pipeline from seed definition to published, self-verified package. Without a reference vertical, integration gaps go undetected and the library lacks a repeatable correctness demonstration.

## What Changes

- Create `examples/voltfox.visual.txt` — an authored visual-set manifest referencing concrete PNG art assets for the Voltfox entity
- Create/commit the referenced PNG frame assets under `examples/` or `examples/assets/`
- Add an integration target (CMake `add_test`) that builds the Voltfox package end-to-end and self-verifies it
- Ensure `examples/voltfox.sprite` is a complete seed that works with both `build-visual` and synthesis-only `build` CLI paths
- Document the end-to-end build command in examples

## Capabilities

### New Capabilities
- `sprite-seed-definition`: The `.sprite` key=value file format for defining entity seeds (identity, rights, abilities, rig, collisions, animation graph)
- `visual-set-authoring`: The `.txt` manifest format that maps semantic frames to PNG assets with channel maps for depth, normals, effects, etc.
- `synthesis-pipeline`: Automatic 2D/2.5D/3D projection generation from a palette and rig definition
- `package-lifecycle`: Building sprite packages from seed (+ optional visual set), artifact layout, verification, and target compatibility reporting
- `reference-entity`: The Voltfox reference entity (original.voltfox) serving as a complete, documented implementation example across all representations

### Modified Capabilities
<!-- No existing specs to modify -->

## Impact

- `examples/voltfox.sprite`: May need minor updates for completeness
- `examples/voltfox.visual.txt` + PNGs: New authored content
- `CMakeLists.txt`: New test target for end-to-end package build
- `tests/`: New integration test exercising the full pipeline
