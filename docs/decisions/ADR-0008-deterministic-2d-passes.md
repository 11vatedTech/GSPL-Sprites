# ADR-0008: Deterministic 2D pass foundation

## Status

Accepted.

## Decision

Represent canonical 2D working images as bounded RGBA8 with explicit color and
alpha semantics. Build small deterministic passes for trimming, scaling,
palette control, semantic masks, pixel-art validation, atlas packing, and
metadata before admitting learned visual transformations. Preserve pivots and
stable semantic frame identifiers through every pass.

## Consequences

Basic pixel and atlas processing is local, inspectable, reproducible, and does
not depend on a model. Learned segmentation, enhancement, and generation will
enter through governed provider adapters and produce ordinary provenance-linked
artifacts. Higher-precision/HDR work remains a separate future image type rather
than being silently quantized into this RGBA8 path.
