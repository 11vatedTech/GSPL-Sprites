# GSPL Sprites

GSPL Sprites is an independent semantic living-entity compiler, runtime, and
asset-production platform. It compiles a typed, canonical Sprite Seed into a
Sprite IR, executable entity behavior, visual projections, and inspectable
packages.

This repository does not depend on the GSPL canon repository or any research
repository. Those repositories informed concepts only.

## Current verified boundary

The first production vertical compiles an original rights-safe electric fox
entity through typed seed and gene validation, IR lowering, a deterministic
runtime ability transition, deterministic SVG projection, an immutable asset
graph, end-to-end provenance, an explicit rights decision, and a checksummed
transactional package. It is deliberately not described as the complete
platform; the remaining gates are governed by `docs/IMPLEMENTATION_ROADMAP.md`.

## Build on Windows

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug
./build/windows-msvc/Debug/gspl-sprites.exe build examples/voltfox.sprite outputs/voltfox
```
