# Canonical Sprite Seed Specification 0.1

The parsed `SpriteSeed` is the immutable semantic source for this release. Its
canonical JSON contains abilities, animation clips, animation state graph,
classification, collisions, colors, entropy root, identity, name, rig, rights,
and schema. Collections whose source order has no meaning are sorted by stable
identity. Track keys retain strictly validated temporal order. Any semantic
change—including motion keys, timing, collision windows, or topology—changes
the seed identity.

## Text source profile

The dependency-free source uses UTF-8 `key=value` records. It is bounded to
1 MiB, 4,096 lines, 8 KiB per line, 64 abilities, 256 bones, 256 sockets,
256 clips, 65,536 keys per track, 512 collision shapes, and 2,048 collision
windows. `#` begins a full-line comment after leading whitespace.

Repeatable records and their fields are:

```text
ability=id|effect|cost|cooldown_ticks|active_ticks
bone=id|parent_or_-|x|y|rotation|scale_x|scale_y|length|min_angle|max_angle
socket=id|bone|x|y|rotation|scale_x|scale_y
clip=id|duration_ticks|looping
track=clip|bone|tick,x,y,rotation,scale_x,scale_y;...
clip_event=clip|event|tick
state=id|clip
transition=source|target|parameter|comparison|threshold|min_ticks|blend_ticks|priority
collision=id|CIRCLE_or_AXIS_ALIGNED_BOX|bone_or_socket|offset_x|offset_y|extent_x|extent_y
collision_window=ability|shape|start_tick|end_tick|deals_damage
```

`rig=id` must precede bones and sockets. A clip must precede its tracks/events.
A source state must precede transitions originating from it. These ordering
rules make unresolved-reference errors local and deterministic; canonical
identity remains independent of declaration ordering where semantics are
unordered.

Rig/animation/collision domains are optional for entities whose projection does
not use them. If any one is present, a valid rig is required. Collision windows
must reference a declared ability and fit inside that ability's active timing.

