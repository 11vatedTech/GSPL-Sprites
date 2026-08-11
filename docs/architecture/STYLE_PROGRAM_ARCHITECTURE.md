# StyleProgram Architecture

## Responsibility

Style is **typed visual behavior, not a label**. The StyleProgram is the
factorized, composable, deterministic representation of style; labels
(`clean-flat`, `inked`, `soft-shaded`, `pixel-constrained`) are presets
that lower to behaviors. The StyleProgram controls construction
abstraction, line behavior, shading behavior, motion cadence, palette
behavior, FX representation and compositing — scoped per region where the
canon allows.

## Data model

```text
StyleProgram
 ├── LineProgram          line weight/taper policy, outline selection, per-region
 │                        weight rules, edge treatment, stroke pattern kind
 ├── ShadingProgram       shadow model (flat/gradient/cell), band count,
 │                        ramp curve, highlight model, hatch/halftone policy,
 │                        shadow terminator intent (artist-directed normals analog)
 ├── MotionProgram        timing cadence, spacing behavior, smear/stepped motion
 │                        policy, motion abstraction gating
 ├── PaletteProgram       palette policy (preserve/harmonize/quantize), value
 │                        grouping, saturation behavior, ramp hue-shift curve
 ├── FxProgram            effect interpretation: aura, emission, halftone dots,
 │                        registration, impact frames, speed lines
 └── CompositingProgram   layer compositing (source_over/additive), alpha policy,
                          anti-aliasing policy, pixel quantization policy
```

Composition follows explicit precedence: base < project < entity < form <
performance. `effective_style()` deterministically lowers the composed
program to the existing `StyleSemantics` consumed by the Visual Semantic
Compiler — the renderer sees the same resolved style as today.

## Invariants

1. No global mutable style state; composition is a pure function.
2. Precedence is explicit and tested (later/更高-precedence patches win).
3. Style changes alter permitted visual properties and the Visual
   Manifestation Identity — never entity identity, never material truth,
   never color-role topology.
4. Thresholded behaviors use anchored thresholds to avoid flicker
   (temporal coherence).
5. Coherence risk of factorization is documented: an inked LineProgram +
   soft-shaded ShadingProgram may clash; taste filters combinations —
   the system's job is deterministic composition, not taste.
6. No franchise-imitation presets. Styles are original behavior vectors.

## Ownership and interactions

- **Owner:** authored StyleProgram data + preset library.
- **Consumers:** Visual Semantic Compiler (lower to StyleSemantics),
  FX interpretation, resolution/LOD behavior, fidelity diagnostics
  (palette violations, prohibited AA, line inconsistency).
- **Renderer:** effective style only.

Interactions:
- canon regions scope line/shading rules per structure;
- performance motion phase feeds MotionProgram cadence;
- transformation trajectory may drive style parameters along escalation;
- palette program derives ramps deterministically (hue-shift curves).

## Determinism and serialization

`validate_style_program`, `canonicalize_style_program`,
`style_program_identity`, `effective_style` (deterministic lowering).
Same input twice → byte-identical style and output.

## Testing

- preset correctness (clean-flat no outlines; inked heavier lines; pixel
  disables AA);
- precedence (later patches win; scope respected);
- factorization coherence: composed program lowers to valid StyleSemantics;
- identity: same entity under 4 programs → materially distinct frames,
  identical entity identity;
- determinism: repeated effective_style byte-identical;
- per-region scoping.

## Extension strategy

New style dimensions = new typed behaviors with canonicalization +
lowering. New presets = data. Learned style specialists (future) propose
behavior parameter patches; patches are typed, validated and compiled
deterministically — never silent.
