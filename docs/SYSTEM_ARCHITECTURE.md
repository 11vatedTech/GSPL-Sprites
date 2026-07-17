# GSPL Sprites system architecture

The semantic entity is authoritative. The six non-collapsible representations
are Authoring Graph, canonical Sprite Seed, typed Sprite IR, immutable Asset
Graph, Runtime Entity, and Target Package. Each boundary is versioned and
validated. Generated assets may only flow forward through explicit compiler
passes; operational telemetry never affects canonical identity.

The production core is C++23 with explicit values and ownership. TypeScript is
reserved for the authoring application and generated-contract consumers.
Python is a governed research/evaluation environment and is not a core runtime
service. Model execution is adapter-based: CPU is the baseline; ONNX Runtime is
the portable provider contract; CUDA and TensorRT are optional accelerators.

Artifact identity is content-addressed. A build identity includes canonical
seed bytes, compiler and pass versions, target, model descriptors, and named
entropy channels. Engine adapters must preserve a feature or reject it with a
diagnostic.

The first vertical slice exercises the final boundaries without pretending to
implement learned synthesis. Its SVG projection is a real deterministic target
adapter used to prove identity, validation, runtime, packaging, and rebuild
semantics before GPU/model complexity is admitted.

Normative subsystem contracts are defined in `SPRITE_GENE_SPECIFICATION.md`,
`ASSET_GRAPH_SPECIFICATION.md`, and `RIGHTS_AND_PROVENANCE.md`. Major choices
are governed by records under `docs/decisions/`.
