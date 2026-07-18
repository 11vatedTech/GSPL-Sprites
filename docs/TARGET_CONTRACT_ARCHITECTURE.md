# Target preservation contract

Target adapters must explicitly resolve every requested GSPL feature as `native`, `adapter-emulated`, or `unsupported`. A required unsupported feature fails compatibility; omission is treated as unsupported. Optional requirements remain visible in the canonical report without blocking export.

The initial built-in descriptors describe code that exists in this repository, not the theoretical capabilities of an engine:

- `portable-package` covers the currently verified canonical seed, rights/provenance, 2D assets, rig, animation graph, collisions, and channel maps.
- `glb-2.0` covers the validated mesh, PBR material, skin, morph, and animation exporter. LOD is adapter-emulated because GSPL metadata preserves levels but selection remains consumer-owned.

The CLI command `target-check <adapter> <required-feature>...` emits a deterministic JSON report and exits unsuccessfully if fidelity would be lost. Future engine adapters must ship their implementation, descriptor evidence, compatibility tests, and engine importer validation together.

Current engine research supports this conservative boundary. Godot recommends glTF 2.0 for 3D scenes and supports imported animation libraries. Unreal documents that glTF cannot represent every engine feature and that unsupported extensions may prevent loading. Consequently, engine brand-level claims are not used as capability evidence; only GSPL adapter behavior is.

## Primary research

- Godot stable documentation, [Importing 3D scenes](https://docs.godotengine.org/en/stable/tutorials/assets_pipeline/importing_3d_scenes/index.html), accessed 2026-07-17.
- Godot documentation, [ResourceImporterScene](https://docs.godotengine.org/en/stable/classes/class_resourceimporterscene.html), accessed 2026-07-17.
- Epic Games, [glTF File Format Support in Unreal Engine](https://dev.epicgames.com/documentation/unreal-engine/gltf-file-format-support-in-unreal-engine?lang=en-US), accessed 2026-07-17.
