# Godot 4.7 3D target adapter

The `godot-4.7-3d` adapter publishes a directly importable Godot project from validated GSPL 3D IR. Export is transactional and refuses an existing destination. The project contains:

- a validator-gated GLB containing meshes, materials, skinning, morph targets, animation, and governed LOD metadata as applicable;
- a Godot 4 format-3 `PackedScene` that instantiates the GLB scene;
- `project.godot` with the generated scene as the runnable main scene;
- the canonical target-compatibility report;
- canonical source-package evidence containing the package, seed, authoring
  provenance, and target-compatibility identities when supplied;
- a canonical SHA-256 target manifest.

Requirements are derived from the actual projection and animation payload, then merged with caller requirements. Required unsupported semantics fail before staging begins. The adapter currently does not translate the GSPL living runtime, deterministic replay, 2D, or 2.5D contracts, so those required features are rejected rather than discarded.

Every project includes `source-evidence.json`. Exports originating from a
verified portable package bind its four lowercase SHA-256 identities into that
artifact; lower-level projection exports state `sourcePackage: null` explicitly.
The artifact participates in the target manifest and therefore in project
identity. `target_source_evidence_from_package` issues the typed evidence only
after the independent package verifier accepts the source and hashes its
governance artifacts from disk; callers cannot construct evidence directly.
Invalid or modified source packages fail before target export.

The independent verifier rejects modified or missing artifacts, undeclared files, symlinks, excessive directory entries, and manifest disagreement. Engine-generated `.godot` import caches are not target artifacts and therefore are produced only after verification/publication.

## Import validation

The generated project was imported and its main scene loaded with the official Godot 4.7 stable Windows editor using:

```text
Godot_v4.7-stable_win64_console.exe --headless --path <fixture> --import
```

The first import identified missing GLB vertex-attribute strides. GSPL then added exact `byteStride` declarations for all vertex buffer views. The regenerated project imported with exit code 0 and no warnings or errors.

## Primary sources

- Godot, [Download archive](https://godotengine.org/download/archive/), accessed 2026-07-17. Godot 4.7 stable was released 2026-06-18.
- Godot stable documentation, [Command line tutorial](https://docs.godotengine.org/en/stable/tutorials/editor/command_line_tutorial.html), accessed 2026-07-17.
- Godot 4.6 documentation, [TSCN file format](https://docs.godotengine.org/en/4.6/engine_details/file_formats/tscn.html), accessed 2026-07-17.
