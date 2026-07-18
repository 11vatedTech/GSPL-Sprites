# Authoring references and target requests

Authoring projects retain reference inputs as immutable records containing a stable ID, URI, SHA-256 content identity, explicit rights class, intended use, and whether the reference is required. A URI is descriptive provenance, not permission to fetch mutable external content. Downstream ingestion must resolve bytes and prove the recorded hash independently.

Reference use is typed as semantic structure, visual asset, motion asset, or audio asset. Required semantic references must permit research use. Required asset references must permit commercial export before lowering can produce a seed. Optional unresolved references remain inspectable but do not enter the lowered entity.

Target requests name an implemented adapter and enumerate typed feature requirements. Validation invokes the shared fail-closed target contract. Unknown adapters, duplicate targets or features, and unsupported required features invalidate the authoring project. Optional unsupported features remain visible without falsely blocking compatibility.

Both collections participate in canonical authoring identity and canonical persistence. Authoring builds carry the revision identity and reference records into `authoring-provenance.json`, and carry canonical adapter decisions into `target-compatibility.json`. Both artifacts are content-addressed, declared by the package manifest, and linked to the canonical seed through the asset graph. Direct seed builds emit explicit empty evidence documents rather than silently omitting governance.
