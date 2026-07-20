# Immutable Artifact Graph

## Requirements
1. Every artifact stored by SHA-256 content hash
2. Each node tracks: type, schema_version, producing_pass_hash, dependency_hashes, semantic_owner, form, representation, validation_state, provenance, determinism_class
3. Dependency edges link artifacts to their inputs
4. Changing source artifact invalidates only transitive dependents
5. Each artifact records origin (compiler pass, synthesis function, runtime)
6. Each artifact carries validation result

## Scenarios
1. Palette change → visual artifacts change hash; behavior artifacts stable
2. Ability timing change → animation-event artifacts change; geometry stable
3. Storm morphology change → storm artifacts change; base artifacts stable
4. All vertical artifacts reachable from root package artifact
5. Artifact graph serializes/deserializes round-trip with identical hash
