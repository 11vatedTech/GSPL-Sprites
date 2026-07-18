# 2.5D projection architecture

GSPL Sprites represents 2.5D as a distinct compiled projection, not as a flag on
a 2D atlas. The semantic representation kind distinguishes `2D`, `2.5D`, `3D`,
and `HYBRID`; a 2.5D projection accepts only `2.5D` or `HYBRID`. Hybrid is
required when selective 3D geometry is attached to sprite planes.

A projection contains ordered depth planes with visual, normal, and material
asset identities; signed depth; fixed-point parallax; bounded camera-relative
deformation; optional multi-plane rig nodes; and explicit lighting participation.
Lit planes require normal maps. Asset identities are references into the
immutable asset graph rather than filesystem paths.

Angular views use millidegrees and must explicitly cover every plane, including
planes intentionally hidden in that view. Generated rotational views identify a
direct authored source; generated-on-generated provenance chains are rejected.
Discrete multi-angle billboards require at least two unique angles. Nearest-view
selection uses circular angular distance and a stable identity tie-break.

Hybrid geometry attaches a governed geometry asset to a plane and semantic
socket. Depth-aware collision volumes bind to a plane and use non-empty XY and
depth intervals. Fixed-point parallax avoids platform floating-point drift and
rejects multiplication overflow. Canonicalization sorts all set-like records so
declaration order cannot change projection identity.

This layer defines and validates the projection contract. Image synthesis,
depth estimation, view generation, target rendering, asset-graph resolution,
and concrete hybrid geometry loading remain governed compiler/provider and
adapter responsibilities.
