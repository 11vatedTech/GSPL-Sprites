# ADR-0017: Bounded validator-gated GLB target

## Status

Accepted.

## Decision

Export 3D runtime assets as self-contained glTF 2.0 GLB using a deterministic
checked writer over validated projection IR. Embed only referenced PNG textures,
compute skin inverse bind matrices, preserve collision purpose, enforce all
container limits and alignment, and gate fixtures with Khronos glTF Validator.

## Consequences

GSPL Sprites produces an interoperable binary 3D artifact without trusting an
opaque third-party exporter at runtime. Specification mistakes become testable
failures. The adapter must evolve with glTF conformance requirements; animation,
compression, tangents, and target-engine importer tests are still required.
