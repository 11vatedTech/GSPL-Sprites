# ADR-0003: Explicit image semantics and deterministic atlases

Status: accepted — 2026-07-17

## Decision

All decoded images carry dimensions, channel storage, named color space, and
alpha mode. Codecs are adapters that must enforce encoded-byte, dimension,
pixel, decoded-byte, and format-metadata limits before allocation. Atlas packing
uses a stable height/width/ID order and bounded dimensions. Animation timing is
integer ticks and references stable frame identities.

libspng is the planned PNG adapter, OpenColorIO 2 is the production color
transform authority, and KTX 2.0/Basis is a target-delivery format. The bundled
strict PPM P6 codec exists for dependency-free conformance and hostile-input
tests, not as the authoring product's preferred format.

## Consequences

Color and alpha assumptions cannot be implicit, atlas outputs are reproducible
across source ordering, decompression-bomb allocation is bounded, and texture
compression never replaces canonical source imagery.

