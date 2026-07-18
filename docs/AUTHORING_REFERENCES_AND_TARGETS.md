# Authoring references and target requests

Authoring projects retain reference inputs as immutable records containing a stable ID, URI, SHA-256 content identity, explicit rights class, intended use, and whether the reference is required. A URI is descriptive provenance, not permission to fetch mutable external content. Downstream ingestion must resolve bytes and prove the recorded hash independently.

Reference use is typed as semantic structure, visual asset, motion asset, or audio asset. Required semantic references must permit research use. Required asset references must permit commercial export before lowering can produce a seed. Optional unresolved references remain inspectable but do not enter the lowered entity.

Target requests name an implemented adapter and enumerate typed feature requirements. Validation invokes the shared fail-closed target contract. Unknown adapters, duplicate targets or features, and unsupported required features invalidate the authoring project. Optional unsupported features remain visible without falsely blocking compatibility.

Both collections participate in canonical authoring identity and canonical persistence. The current lowered `SpriteSeed` does not embed reference or target records; production packages must subsequently carry their provenance and compatibility reports through the asset graph. Until that compiler linkage is implemented, the authoring project remains the authority for these records.
