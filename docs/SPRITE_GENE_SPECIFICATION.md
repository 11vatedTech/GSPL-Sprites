# Sprite Gene Specification 0.1

A Sprite Gene is a typed, versioned semantic component. Each instance carries a
stable type identity, schema version, closed kind, and a payload whose C++ type
must match that kind. The registry owns purpose, dependencies, conflicts, and
runtime/target relevance. Unknown type-version pairs and payload mismatches are
fatal diagnostics. Non-repeatable genes are unique; abilities are repeatable.

The initial executable inventory covers identity, classification, appearance,
ability, and rights. `GeneKind` reserves the complete domain-family vocabulary,
but an enum value is not an implemented gene: a family becomes supported only
when it has a descriptor, typed payload, validation, compiler lowering, and test
vectors. This prevents aspirational labels from masquerading as functionality.

