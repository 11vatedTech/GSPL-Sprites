# GSPL Sprites

GSPL Sprites is an independent semantic living-entity compiler, runtime, and
asset-production platform. It compiles a typed, canonical Sprite Seed into a
Sprite IR, executable entity behavior, visual projections, and inspectable
packages.

This repository does not depend on the GSPL canon repository or any research
repository. Those repositories informed concepts only.

## Current verified boundary

The first production **Living Sprite 2D** vertical is complete. An original
rights-safe entity — the governed Voltfox acceptance fixture — compiles
through the full governed pipeline:

```text
GSPL semantics → canonical entity → SpriteSeed → source skeletal semantics
→ living synthesis → 48 governed frames → 9 clips → 48 retained samples
→ 4 source-bound generated events → channel semantics → collision semantics
→ base/storm/transformation morphology → deterministic atlas → canonical
Living Visual Package → cryptographic inventory → independent artifact
reconstruction → semantic verification → separate-process verification
```

The package system proves exact semantic sets and provenance for frames,
samples, frame hashes, poses, clips, generated events, channels, morphology,
and the atlas; independently parses `source-skeletal-animations.json` from
immutable verified bytes and binds generated events to reconstructed source
events; enforces every governed `PackageReadLimits` field; derives channel and
atlas provenance from decoded pixel identity; and rejects hostile
self-consistent mutations at exact semantic diagnostics (SC1–SC19).

`gsplc --verify-package <path>` verifies a package in a separate OS process
(exit 0 for valid packages; nonzero with the exact semantic diagnostic for
hostile packages).

Voltfox is an acceptance fixture, not a hardcoded architecture: any unrelated
original entity that lowers to a valid `SpriteSeed` enters the same pipeline
without entity-specific logic. Remaining platform gates are governed by
`docs/IMPLEMENTATION_ROADMAP.md`.

## Build on Windows

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug
./build/windows-msvc/gspl-sprites.exe build examples/voltfox.sprite outputs/voltfox
./build/windows-msvc/gspl-sprites.exe verify outputs/voltfox
```
