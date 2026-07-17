# ADR-0009: Typed authored channel maps

## Status

Accepted.

## Decision

Represent material, normal, depth, emissive, effects, and collision channels as
typed frame-aligned artifacts. Distinguish explicitly declared data textures
from unknown color metadata. Validate encoding rules before packaging and retain
source/target dependency edges and provenance.

## Consequences

Renderers and exporters no longer infer channel meaning from filenames or pixel
appearance. Untagged PNG is accepted only after a typed non-color map declaration.
Emissive remains sRGB until a verified color-management pass exists. Generated
maps can reuse the artifact contract but cannot bypass model governance.
