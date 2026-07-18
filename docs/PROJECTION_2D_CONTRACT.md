# Compiled 2D projection contract

`Projection2dDefinition` is the first-class in-memory production artifact for a
2D entity representation. It owns bounded source frames, compiled color atlas,
alpha and outline atlases, exact canonical placement metadata, animation clips,
typed channel maps, rig, and collision windows.

Validation requires unique stable frame and animation identities, valid image
storage, total source-frame animation coverage, exact placement/source agreement,
in-bounds non-overlapping atlas rectangles, mask dimensions matching the color
atlas, canonical metadata, resolved and structurally valid channel maps, a valid
rig, and collision timing within the governed ability interval. Partial or
internally inconsistent compiled assets fail closed.

Canonical identity hashes every source, atlas, mask, and channel-map pixel;
placement metadata; animation ordering, duration, and events; rig; channel
semantics; and collision contracts. A visual pixel or gameplay timing change
therefore changes representation identity. This contract is now strong enough
for transformation bindings without trusting file names or metadata alone.
