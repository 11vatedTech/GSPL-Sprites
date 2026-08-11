# Visual Fidelity Validation

## Responsibility

Make visual quality **measurable as enforceable lower bounds** — without
claiming to replace human artistic judgment. Deterministic diagnostics
over semantic inputs (canon, IR, rendered output) plus human review
artifacts constitute the two-part acceptance mechanism.

## Metric categories

### Canon-level (new: visual_quality)
- structural graph validity (acyclic, typed, refs resolve);
- proportion validity (ranges, preferred within range);
- landmark validity (refs, ordering, symmetry, existence per form);
- silhouette-feature declarations (anchors, negative-space minima);
- color-role topology consistency across forms;
- material truth consistency across styles;
- deformation envelope validity (ranges, hard boundaries);
- resolution-feature validity (priority, min-resolution ordering).

### Rendered-image level (existing visual_metrics + extensions)
- silhouette: occupancy, connected components, aspect, centroid, bounds;
- alpha-edge quality (AA partial coverage);
- palette adherence (covered pixels near palette colors);
- shape overlap (material channel multiplicity);
- frame similarity / distinction / loop closure;
- NEW negative-space measure (void ratio inside bounds);
- NEW landmark drift (canon landmarks vs rendered projections, when
  applicable).

### Temporal level (new)
- landmark jitter (ID-tracked landmark positions across frames);
- marking drift (ID-tracked marking geometry distance);
- frame duplication (unintended identical frames);
- deliberate-incoherence windows (documented smear/stepped frames do not
  count as defects).

## Invariants

1. Every metric is a pure deterministic function of its inputs.
2. Metrics are lower bounds: they enforce structural conditions, they do
   not score aesthetics.
3. Canon-level hard invariants fail closed; softer diagnostics report
   severity + code, never silent.
4. Regression testing distinguishes: exact deterministic regression
   (byte hash), semantic geometry regression (metrics), perceptual
   regression (structural similarity), intended art-direction change
   (explicit golden update).
5. Human review artifacts (contact sheets, model-sheet sheets) are
   generated from the same semantic representation used by runtime
   compilation — never hand-painted substitutes.

## Ownership and interactions

- **Owner:** visual_metrics/visual_quality modules.
- **Inputs:** VisualCanon, VisualIr, RasterOutput, TemporalIdentityRegistry.
- **Consumers:** CI gates, evidence manifests, viewer debug panels.

## Testing

- good fixture vs intentionally degraded fixture → metrics separate
  (degraded produces worse diagnostics);
- empty/single-component/AA edge cases;
- determinism A/B (metrics stable across repeated runs);
- canon-level validation negative cases (cycle, dangling, inverted range);
- regression classes: byte-hash + semantic + perceptual distinctions.

## Artifact pipeline

Model-sheet and review artifacts (turnaround, silhouette, landmark,
expression, pose, deformation, style, generalization sheets) are rendered
from canon + poses + styles and recorded in an evidence manifest with
per-image identity and RGBA hash. These are acceptance evidence, not
semantic authority.

## Extension strategy

A future Fidelity Critic (learned or hybrid) consumes the same typed
diagnostics and structures — it proposes quality judgments that are
recorded as diagnostics, never as silent truth.
