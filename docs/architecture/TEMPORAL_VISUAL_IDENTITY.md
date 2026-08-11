# Temporal Visual Identity

## Responsibility

Every persistent visual structure — contour segment, marking, highlight,
line, stroke cluster, effect emitter, anatomical landmark — must keep a
**stable semantic identity across frames**. Coherence is achieved by
construction (structures bind to canon semantics), and incoherence is
always deliberate (typed smear/stepped behaviors), never accidental.

## Data model

```text
TemporalStructure
 ├── id                     stable across frames
 ├── kind                   contour_segment | marking | highlight | line |
 │                          stroke_cluster | effect_emitter | landmark | material_region
 ├── owner                  canon structure id (binding surface)
 ├── anchor                 landmark ref or surface parameter (future)
 └── persistence_rule       always | until_form_change | while_visible

TemporalIdentityRegistry
 └── deterministic assignment of structure IDs from (canon, form, state)
     — the same (entity, form, pose-class) always yields the same ID set
```

## Invariants

1. IDs derive deterministically from semantics — never from frame-local
   analysis. Rendering the same state twice yields the same ID set.
2. Structures bind to canon surfaces/landmarks (markings already bind via
   `part_id`); binding provides coherence by construction.
3. Deliberate incoherence (smear frames, stepped animation, ghost
   multiples) is a **typed behavior** gated by performance/style intent —
   it does not change structure IDs, it changes realization.
4. Thresholded behaviors (shadow bands, dither, AA) use anchored
   thresholds to avoid crossing-boundary flicker.
5. The registry is state-free: no hidden mutable global; ownership is
   explicit (compiler-owned, per-manifestation).

## Ownership and interactions

- **Owner:** TemporalVisualIdentity module + compiler.
- **Consumers:** raster channel generation (marking/emission/material
  regions), future stroke/cluster realizers, fidelity diagnostics
  (landmark jitter, marking drift, frame duplication).
- **Renderer:** receives stable IDs for persistent structures.

Interactions:
- canon markings/lines/highlights obtain stable IDs at VisualIr compile;
- motion/smear behaviors signal deliberate incoherence windows;
- temporal fidelity metrics consume ID-tracked structures.

## Determinism and serialization

`assign_temporal_identities(canon, ir) → registry`; deterministic,
canonicalizable. Tests: same state twice → identical ID map; form change →
new IDs for changed structures, stable IDs for invariant structures.

## Testing

- ID stability across repeated compiles of the same state;
- ID stability across pose changes (marking on a part keeps its ID when
  the part moves);
- deliberate incoherence: smear-requested frame still uses the same
  structure IDs;
- determinism A/B;
- registry canonicalization round-trip.

## Extension strategy

Future stroke/cluster realizers consume registry IDs (surface-anchored
stroke correspondence, pixel-cluster persistence). Learned specialists
propose structures; the registry assigns IDs only after validation.
