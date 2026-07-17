# Animation Architecture 0.1

Rigs are explicit bounded bone trees with one root, stable bone identities,
rest transforms, joint limits, and typed sockets. Cycles, missing parents,
non-finite transforms, malformed limits, and duplicate sockets are fatal. Skin
vertices permit one to four unique bone influences whose finite positive
weights sum to one.

Skeletal clips use integer ticks, strictly ordered keys, shortest-path angular
interpolation, bounded duration, and governed events. State graphs reference
validated clips, require one present initial state, unique transition priorities
per state, valid targets, and complete reachability. Runtime transition choice
is deterministic ascending priority.

Collision shapes attach to stable bones or sockets. Hit/hurt windows are half-open semantic
tick intervals and must fit inside the owning ability's active duration. Visual
animation never becomes the authority for damage or ability effects; both are
projections of the semantic ability contract.
