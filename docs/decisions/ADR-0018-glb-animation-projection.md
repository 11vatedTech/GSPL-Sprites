# ADR-0018: Project fixed-tick animation into glTF channels

## Status

Accepted.

## Decision

Convert validated fixed-tick joint tracks into glTF linear transform channels.
For each mesh, merge independently keyed morph tracks onto a sorted union
timeline and emit one complete weights channel. Preserve GSPL loop policy and
semantic events in animation extras rather than pretending glTF owns runtime
playback semantics.

## Consequences

Animated GLBs remain standards-conformant while retaining the information a
GSPL runtime adapter needs. Morph tracks with different key schedules do not
create conflicting glTF channels. Target engines must still translate semantic
events and loop policy through their runtime adapters.
