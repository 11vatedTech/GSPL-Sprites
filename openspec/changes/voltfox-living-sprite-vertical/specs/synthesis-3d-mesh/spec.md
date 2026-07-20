# Proper 3D Mesh Topology

## Requirements
1. All mesh triangles have nonzero area (no degenerate faces)
2. Per-vertex normals computed from triangle geometry, unit length
3. UV coordinates per vertex for material mapping
4. Material references (body, head, eyes, aura, storm_markings)
5. 13-bone skeleton hierarchy with valid bind-pose transforms
6. Per-vertex skinning weights (0-4 influences) sum to 65535
7. Animation clips for idle, locomotion, attack, ascend, storm_idle, storm_attack, descend
8. Valid glTF 2.0 binary export with structural verification

## Scenarios
1. Vertex count matches expected from morphology parts
2. No degenerate triangles in any mesh section
3. All normals are unit length (within 1% tolerance)
4. Skinning weights per vertex sum to exactly 65535
5. Skeleton has valid parent<child ordering
6. glTF verifier reports structural correctness
7. Animation clips produce different poses at different times
