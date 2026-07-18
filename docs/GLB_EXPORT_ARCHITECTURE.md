# GLB export architecture

The GLB adapter converts validated 3D projection IR into glTF 2.0 binary runtime
assets. It emits little-endian, four-byte-aligned position, normal, conditional
UV, joint, weight, index, morph, and inverse-bind accessors. Positions convert
from micrometers to glTF meters. Joint hierarchies become nodes and skins with
computed global inverse bind matrices; render and collision purpose remains in
node/mesh extras for downstream adapters.

Materials map to metallic-roughness PBR with explicit alpha behavior. Referenced
textures must be supplied exactly once as governed PNG assets. They pass the
bounded hostile-media decoder before embedding; missing, duplicate, malformed,
oversized, unsupported, or unreferenced textures fail closed. UV attributes are
emitted only when the material samples a texture.

The writer enforces GLB's 32-bit size ceiling and a stricter configurable output
limit. JSON chunks use space padding, BIN chunks use zero padding, chunks remain
in normative order, and buffer views omit invalid targets for data such as
images and inverse bind matrices. The entire artifact is returned in memory so
the caller can commit it through the package system's transactional write path.

The conformance fixture is checked with Khronos glTF Validator
`2.0.0-dev.3.10`; the accepted baseline has zero errors, warnings, infos, or
hints. Validated fixed-tick joint tracks export as linear translation, rotation,
and scale channels. Morph tracks targeting the same mesh are merged onto a
deterministic union timeline and emitted as the complete ordered weight vector
required by glTF. Loop policy and semantic tick events remain GSPL extras because
glTF defines key storage but not playback or gameplay event behavior.

Textured meshes pass the bounded mesh-quality gate before their UV attributes
are emitted. Normal-mapped materials additionally receive generated normalized
`TANGENT` vectors and handedness. UV overlap, degenerate parameterization, or
inconsistent normal orientation therefore fails before GLB construction.

Tangent generation, texture color-space cross-checks, compression, and engine
import conformance remain subsequent passes.
