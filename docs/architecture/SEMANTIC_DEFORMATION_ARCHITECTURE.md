# Semantic Deformation Architecture

## Responsibility

Describes **how far an entity may deviate from its canonical form** —
per structure, per context (action/expression/style/transformation) — and
separates that *intent* from any geometric solver. Being off-model is
valid when intentional and bounded; unbounded or undeclared deviation is
rejected.

## Data model

```text
DeformationEnvelope
 ├── structure_id
 ├── canonical_value          (canonical proportion/rotation/position/scale)
 ├── allowed_deviation        (everyday range, e.g. pose/expression)
 ├── action_scale             multiplier for action-driven deviation
 ├── expression_scale         multiplier for expression-driven deviation
 ├── style_scale              multiplier for style-driven deviation
 ├── transformation_scale     multiplier for transformation-driven deviation
 ├── hard_boundary            absolute identity limit
 ├── rigid                    whether structure must remain rigid (identity-critical)
 └── volume_preserving        request volume conservation when squashing/stretching
```

`DeformationCanon` = set of envelopes (per structure, including joints and
expressive features). Effective allowed deviation for a context =
`allowed_deviation × action_scale × expression_scale × style_scale ×
transformation_scale`, clamped by `hard_boundary`.

## Invariants

1. hard_boundary ≥ effective allowed deviation in every context.
2. Envelopes are validated: finite values, ranges ordered, refs resolve.
3. Structures flagged `rigid` (or covered by hard invariants) cannot
   exceed their boundary regardless of context.
4. Intent-driven deformation is bounded by envelopes; the compiler
   clamps/validates; violations produce diagnostics and fail closed for
   hard boundaries.
5. Determinism: same canon + same context → same effective bounds.
6. Solver separation: FFD/cage/ARAP/DQS/PSD etc. are realization layers
   below the semantic line (future work). The semantic layer never
   manipulates vertices.

## Ownership and interactions

- **Owner:** DeformationCanon (authored data per entity) + compiler
  enforcement.
- **Consumers:** pose bake (KeyPose deformations), expression controls,
  transformation trajectories, fidelity diagnostics (deviation reports).
- **Renderer:** never sees envelopes.

Interactions:
- PerformanceIntent (phase/force) selects envelope activation;
- transformation trajectory drives `transformation_scale` along the
  morphogenesis path;
- fidelity diagnostics measure realized deviation vs canonical value.

## Determinism and serialization

`validate_deformation_canon`, `canonicalize_*`, identity. Envelope tests:
boundary violation rejection, context multiplication, rigid structure
protection, determinism A/B.

## Testing

- envelope validation (inverted ranges, nonfinite, dangling structure);
- allowed-vs-boundary: within-envelope passes, hard-boundary breaks;
- context scaling (action/expression/style/transformation);
- rigid structures never exceed boundary;
- identity: deformation within envelopes preserves invariants;
- determinism: identical context → identical effective bounds.

## Extension strategy

New deformation axes = envelope vocabulary additions. New solvers (FFD,
cage, ARAP, DQS, PSD) are pluggable below the semantic line; they consume
the same envelopes. Learned deformation specialists (future) propose
envelope parameters, validated by the canon.
