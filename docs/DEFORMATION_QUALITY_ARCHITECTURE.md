# Deformation quality architecture

The deformation-quality pass evaluates validated fixed-tick clips through the
same linear-blend skinning model exported to glTF. It constructs rest global
transforms and inverse bind matrices, samples local joint tracks with linear
translation/scale and shortest-path normalized quaternion interpolation, then
skins every render vertex using its normalized influences.

Sampling always includes clip endpoints and authored joint keys. Remaining
capacity is filled deterministically across the clip duration. Both sampled tick
count and total vertex evaluations are bounded; a clip too large to evaluate
within policy fails rather than receiving partial approval.

For every sampled triangle, the pass compares deformed area against bind-pose
area. Ratios below policy count as collapsed triangle samples. It also records
maximum vertex displacement and rejects clips beyond the target budget. Animated
GLB export invokes this gate before writing any channel data.

This validation catches collapse and explosive transforms but does not yet
measure self-intersection, volume preservation, joint-limit violation, cloth or
soft-body behavior, silhouette error, or LOD-to-LOD animated correspondence.
Those require additional geometry and target-specific policies.
