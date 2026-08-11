# Visual Performance Architecture

## Responsibility

Represents *what an entity is doing and feeling* in a way that drives pose,
deformation, expression, FX and timing simultaneously — the semantic layer
above raw animation frames. Never animation-truth by itself; the runtime's
authoritative state remains the source of intents.

## Data model

```text
PerformanceIntent
 ├── action                 (attack, idle, jump, transform, ...)
 ├── phase                  (anticipation|initiation|extension|contact|
 │                           impact|recoil|follow_through|recovery|settle|custom)
 ├── commitment             [0..1]
 ├── force                  direction (Vec2) + magnitude [0..1]
 ├── emotion                (extensible: focused_aggression, fear, joy, ...)
 ├── balance                (planted|shifted|recoil|intentional_imbalance)
 ├── gaze_target            (landmark ref or direction)
 ├── attention              (explicit attention vector, optional)
 └── timing_cadence         (spacing/timing behavior)

KeyPose
 ├── id
 ├── line_of_action         direction + curvature
 ├── center_of_mass         Vec2 (entity-local)
 ├── support_contacts       Vec2 list
 ├── balance_intent
 ├── gaze
 ├── force
 ├── silhouette_goal        target positions for priority landmarks/extremities
 ├── expression             facial controls (per FacialCanon)
 └── deformations           per-structure envelope application requests

PoseIntent (lightweight, derived)
 └── part motions (existing PerformanceState lowering target)
```

## Invariants

1. Intents are typed and validated (phase spelling, commitment range,
   balance spelling, refs resolve).
2. Lowering intent → `PerformanceState` is deterministic.
3. Intent changes visual output but never entity identity; it feeds
   Visual Manifestation Identity correctly (pose changes manifestation
   identity).
4. Motion abstraction (smear requests, stepped cadence) is *declared* by
   intent + style, never automatic.
5. Inbetweening/breakdown derivation is deterministic or explicitly
   deferred to a future procedural realizer — never random.

## Ownership and interactions

- **Producer:** runtime (gameplay/Living/controller layers produce typed
  intents) and authoring fixtures (key poses).
- **Consumer:** Visual Semantic Compiler (intent/key pose → PartMotion
  bake), DeformationCanon enforcement, FX interpretation, style timing
  behaviors.
- **Renderer:** only the compiled result.

Interactions:
- intent → key pose resolution (silhouette goal, contacts, gaze);
- key pose → performance state (existing `PerformanceState`/`PartMotion`);
- intent.force + phase → deformation envelope activation;
- intent.emotion → FacialCanon expression controls.

## Determinism and serialization

`validate_performance_intent`, `canonicalize_performance_intent`,
`performance_intent_identity`; the KeyPose equivalent. Deterministic
tick→frame sequences are tested (existing playback determinism gate
extended to key poses).

## Testing

- intent validation (bad phase, out-of-range commitment);
- deterministic lowering intent → performance state;
- key pose resolution determinism (same intent → same pose);
- phase coverage for typical actions;
- pose changes pixels; identity stable; frames deterministic;
- force/impact gated motion abstraction is only applied when declared.

## Extension strategy

New actions/phases = data. New expression channels = FacialCanon data.
New realizers (IK, springs, motion matching) slot below the semantic
layer as deterministic proposal/realization engines; proposals are typed
and validated.
