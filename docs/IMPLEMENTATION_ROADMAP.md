# Implementation roadmap and completion gates

1. Foundation: independent repository, typed seed, validation, identity,
   deterministic compiler/runtime/projection/package vertical, CI and threat
   model.
2. Domain: complete gene families, authoring graph, constraints, provenance,
   rights enforcement, migrations, property and mutation testing.
3. Asset graph: immutable content store, incremental invalidation, cancellation,
   bounded CPU/GPU scheduler, recovery and telemetry.
4. 2D: safe image ingestion, segmentation/matting adapters, layered rigs,
   animation graphs, atlases, pixel/vector/high-resolution projections.
5. Living runtime: behavior, perception, abilities, combat, transformations,
   audio events, save/load and deterministic replication. Canonical,
   hash-bound authoritative full-state convergence is implemented; prediction,
   rollback simulation, combat/combos, and transformations remain open gates.
6. 2.5D and 3D: depth layers, multi-view, reconstruction, topology, materials,
   rigging, retargeting, validation and LOD.
7. Targets: portable runtime, Godot, Unity, Unreal, web and explicit feature
   matrices.
8. Authoring product: semantic editor, previews, locked regeneration, variant
   comparison, diagnostics, rights and provenance inspection.
9. Hardening: fuzzing, genuine mutation, sanitizers, hostile-media sandboxes,
   benchmarks, cross-platform CI, reproducible releases and SDK stability.

No phase is complete without executable acceptance evidence. The full platform
must not be called complete until every gate in the master directive is met.
