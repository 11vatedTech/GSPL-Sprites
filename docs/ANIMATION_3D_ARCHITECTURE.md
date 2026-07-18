# 3D animation and retargeting architecture

3D clips advance on explicit integer ticks and bind directly to a validated 3D
projection. Joint tracks contain translation, normalized quaternion rotation,
and positive bounded scale. Morph tracks reference compiled morph targets and
use parts-per-million weights. Every track begins at tick zero, key times are
strictly increasing, and duration, rate, track, key, and event counts are
bounded.

Animation events are semantic identifiers at governed ticks. They can later feed
the same runtime event-routing boundary used by 2D projections; target playback
does not become gameplay authority. Canonicalization sorts independent joint,
morph, and event tracks while preserving intrinsic key order.

Retarget maps name their source and target skeletons, use a bounded fixed-point
translation scale, and provide one-to-one joint mappings. Both skeletons are
validated independently. Complete mappings cover every target joint and must
preserve root and immediate-parent relationships. This rejects topology-blind
humanoid remapping for morphologically incompatible entities; explicit partial
mapping can be requested only by callers that accept its limitations.

Pose interpolation, root-motion policy, inverse kinematics, constraint solving,
actual clip conversion, deformation-error measurement, and target-engine
animation export remain subsequent runtime/compiler layers.
