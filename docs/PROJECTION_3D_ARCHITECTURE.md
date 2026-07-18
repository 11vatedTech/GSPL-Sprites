# 3D projection architecture

The 3D projection IR stores validated geometry rather than trusting an opaque
model path. Positions use micrometers, normals and rotations use signed
parts-per-million, UVs use parts-per-million, and skin weights sum exactly to
one million. This makes canonical identity independent of locale and ordinary
floating-point formatting differences while retaining sub-millimeter range.

Render and collision meshes contain indexed triangles. Validation checks index
bounds, incomplete buffers, repeated indices, zero-area triangles, unit normals,
UV range, vertex/triangle budgets, and—when required—undirected edge incidence
and opposing directed winding. Collision meshes always require closed-manifold
validation and cannot carry render materials.

Materials use a bounded metallic-roughness PBR contract with explicit alpha
mode and governed texture asset identities. Skeletons require one root, valid
parents, acyclic hierarchy, normalized rotations, bounded joint count, no more
than four unique influences per vertex, and exact normalized weights. Morph
targets bind only to render meshes and match base vertex cardinality.

LOD entries are contiguous from level zero and must reference render meshes with
strictly decreasing triangle counts and screen-coverage thresholds. Projection
limits participate in canonicalization because changing a production budget is
a semantic build change. Set-like definitions are sorted; vertex and index order
remain intrinsic mesh data.

This IR is designed to map to glTF 2.0's right-handed, meter-based runtime model,
triangle primitives, metallic-roughness materials, skins, and morph targets.
Import/export, UV-overlap analysis, tangent generation, deformation tests,
animation clips, retargeting, mesh optimization, and texture completeness across
actual asset-graph blobs remain subsequent compiler passes.
