# Package-Driven Preview and Headless Acceptance

## Requirements
1. Preview loads Voltfox only from portable package (no test fixtures)
2. Package verification runs before instantiation
3. 2D sprite rendering from package assets
4. 2.5D plane composition from package assets
5. 3D wireframe from package geometry
6. Plays idle, locomotion, attack through all representations
7. Executes directional lightning with visual feedback
8. Renders ascend/descend progression
9. Save/restore: save state, destroy, restore, verify identity matches
10. Headless acceptance: deterministic mode emitting frames, machine-readable state, artifact hashes, package identity, final identity

## Scenarios
1. Preview loads package and displays all reps without crash
2. Keyboard switches between representations
3. Headless mode produces deterministic frame output
4. Save/restore round-trip preserves authoritative identity
