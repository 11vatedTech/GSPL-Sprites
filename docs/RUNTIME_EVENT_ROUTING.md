# Runtime event routing

The runtime event router projects authoritative living-runtime events into
typed animation, combat, effect, and audio commands. Bindings reference a
validated action and, for marker events, a declared semantic marker. Consumer
operation identifiers remain engine-neutral; target adapters translate them to
their native animation graphs, hit-resolution systems, effect pools, or mixers.

Routing is deterministic. Bindings are canonically ordered, events must have a
contiguous sequence, and each output retains its source tick and sequence.
Per-event and per-batch command limits prevent fan-out abuse. Processing is
transactional: invalid sequences, overflow, or command-limit failures do not
advance the router cursor, so callers can diagnose or retry safely.

Combat commands are requests to the authoritative combat consumer. Rendering,
particles, and audio never decide whether damage occurred. Idempotent external
consumers should checkpoint the source event sequence alongside their own
state. Concrete animation playback, combat resolution, effect pooling, spatial
audio, and preview UI are subsequent adapter layers.
