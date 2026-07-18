# Mesh quality architecture

The mesh-quality pass validates parameterization and shading geometry before a
textured 3D projection reaches target export. It checks complete bounded triangle
sets, positive-area UV overlap, degenerate UV triangles, and whether authored
vertex normals face the same hemisphere as geometric triangle normals.

UV overlap uses a sweep broad phase over triangle bounds, followed by convex
polygon clipping. Shared edges and points have zero area and are not false
positives. Candidate comparisons are explicitly bounded; adversarial or highly
overlapping inputs that exceed the configured work budget fail instead of
silently returning partial results.

The same pass accumulates UV-derived tangent and bitangent directions, performs
Gram-Schmidt orthogonalization against each authored normal, and emits normalized
fixed-point tangent vectors plus handedness. Textured GLB export requires the
configured quality policy to pass. Normal-mapped materials then receive an
explicit glTF `TANGENT` attribute, avoiding target-dependent reconstruction.

Intentional mirrored or overlapping UV workflows can use a separately governed
policy, but strict production export defaults to no positive-area overlap. Mesh
simplification error, texel density, seam quality, MikkTSpace conformance, and
animated deformation error remain additional quality passes.
