# Semantic authoring project

The authoring project is editable intent state and is deliberately separate from the immutable canonical Sprite Seed. It preserves alternatives, unresolved decisions, property locks, ability inclusion, named variants, and revision ancestry.

Mandatory seed fields exist as bounded semantic paths. A field may remain unresolved while the user explores alternatives, but it cannot be locked until selected and cannot lower while mandatory. Alternatives are explicit rather than hidden model state. Named variants may select only declared alternatives and may not override locked properties or disable locked abilities.

Revisions use optimistic concurrency. Every edit supplies the expected SHA-256 identity of the complete canonical authoring state. A stale identity fails without mutation. Successful revisions retain the parent identity and increment the revision number. Locked values require an explicit unlock revision before a later value change, preventing regeneration from combining unlock and replacement into one opaque operation.

Lowering applies the optional named variant, resolves typed rights and entropy values, constructs a canonical `SpriteSeed`, and runs the existing seed validator. Unresolved, malformed, rights-denied, or semantically invalid projects do not produce a seed.

The initial contract owns the canonical identity, classification, rights, entropy, palette, and ability boundary. Rig, animation, behavior, reference ingestion, target requests, and richer domain-gene authoring remain subsequent typed extensions; they must not be stored as ungoverned generic JSON.
