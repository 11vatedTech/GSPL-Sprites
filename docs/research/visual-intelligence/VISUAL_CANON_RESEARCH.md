# Visual Canon Research — Corpus Overview

Status: research phase of GSPL Native Visual Intelligence.
Scope: how professional animation, creature design, NPR, pixel art, game
pipelines and perception science solve the problem of **constructing,
recognizing, posing, deforming, transforming and re-projecting a visual
entity**. The corpus exists to ground the GSPL Visual Canon, Performance,
Deformation, StyleProgram and Temporal Visual Identity architectures in
evidence rather than taste.

## Corpus layout

| Document | Question answered |
|---|---|
| CHARACTER_DESIGN_AND_MODEL_SHEETS.md | How do productions keep a character consistent across poses, artists, media? |
| SILHOUETTE_AND_SHAPE_LANGUAGE.md | What makes a character recognizable before any detail is visible? |
| ANATOMY_AND_STRUCTURAL_CONSTRUCTION.md | What is the structural grammar beneath a design? |
| CREATURE_DESIGN_SYSTEMS.md | How are non-human entities designed readably at every scale? |
| FACIAL_EXPRESSION_AND_ACTING.md | How is expression encoded, and how does it generalize beyond faces? |
| POSE_GESTURE_AND_LINE_OF_ACTION.md | What does a pose *mean* beyond joint angles? |
| KEYFRAME_AND_TIMING_SYSTEMS.md | How is motion structured in time? |
| STYLIZED_DEFORMATION.md | How far may an entity deviate from canonical form, and how? |
| TRANSFORMATION_AND_MORPHOGENESIS.md | How do entities change identity while preserving lineage? |
| COLOR_LIGHT_AND_MATERIALS.md | What is color truth vs. artistic response? |
| LINE_ART_AND_CONTOUR_SYSTEMS.md | How are contours and lines represented and controlled? |
| PIXEL_ART_SEMANTICS.md | What is real pixel-art construction (not downsampling)? |
| TEMPORAL_COHERENCE.md | How do persistent visual structures survive frame-to-frame? |
| NON_PHOTOREALISTIC_RENDERING.md | How is stylized rendering computed? |
| HYBRID_2D_3D_PRODUCTION.md | How do studios blend 2D and 3D (Guilty Gear, Spider-Verse, Arcane)? |
| GAME_CHARACTER_PIPELINES.md | How do games represent, animate, retarget and deform characters? |
| STYLE_REPRESENTATION_RESEARCH.md | Can style be factorized into typed programs? |
| VISUAL_FIDELITY_EVALUATION.md | How is visual quality measured without "looks better"? |
| RESEARCH_TO_ARCHITECTURE_MAPPING.md | Which research finding drove which architecture decision? |
| VISUAL_REFERENCE_MATRIX.md | Reference-by-reference knowledge matrix. |
| BIBLIOGRAPHY.md | Permanent, organized source list. |

## Methods and source quality

- Primary/technical sources were prioritized: classic animation texts
  (Williams, Blair, Thomas & Johnston), SIGGRAPH/NPAR/Eurographics papers,
  GDC talks, studio engineering blogs, official production documentation.
- Sources are classified in each document as `direct source statement`,
  `research interpretation`, or `GSPL architectural inference`.
- Citations that could not be verified during the research pass are marked
  `(verify)`; they are retained only as leads, never as load-bearing claims.
- Fandom wikis and SEO content farms were used only for discovery, never as
  evidence.

## Headline conclusions (evidence-backed)

1. **Visual identity is relational, not pictorial.** Model sheets encode
   proportion relationships, landmark anchors and silhouette profiles —
   invariants that survive pose, artist and medium change. (Williams 2001;
   Blair 1994; Thomas & Johnston 1981.)
2. **Silhouette is the primary recognition channel.** Recognition operates
   on boundary contours pre-attentively; production practice (Pokémon's
   "solid black fill" test; Spider-Verse staging) treats the silhouette as
   the first-class identity artifact. (Biederman 1987; Sugimori interviews;
   RMCAD/80 Level practitioner literature.)
3. **Deformation must be intentional and bounded.** Every production that
   permits off-model deformation (One Piece, Dragon Ball, anime) preserves
   signature anchors (props, colors, landmark topology) while freeing other
   structure. "Off-model is not wrong; unbounded off-model is."
4. **Facial expression is a semantic control problem, not a geometry
   problem.** FACS AUs, blendshape banks and stylized systems (floating
   brows, visor apertures, ear angles) all reduce to *orthogonal semantic
   channels over a structure*. (Ekman & Friesen 1978; Lewis et al. 2014.)
5. **Style is a set of behaviors, not a label.** Cel bands, line weight,
   halftones, motion cadence and palette policy are all independently
   controllable behaviors; hybrid productions (Guilty Gear, Spider-Verse,
   Arcane) prove they can be parameterized and recombined.
6. **Temporal coherence is structure identity over time.** The same contour
   segment, marking, highlight or pixel cluster must keep a stable identity
   across frames; incoherence must be *deliberate* (smear frames, stepped
   animation). (Bénard et al. 2011 STAR.)
7. **Pixel art is a constructive discipline.** Clusters, ramps, banding
   avoidance, subpixel motion and temporal cluster stability are semantic
   rules, not rendering artifacts of nearest-neighbor downsampling.
8. **Material truth and artistic response must be separated.** A surface
   "is" fur or steel semantically; a style program decides whether that
   reads as cel bands, hatching, palette ramps or PBR.

## GSPL consequence

The architecture derived from this corpus is documented in
`docs/architecture/VISUAL_INTELLIGENCE_THEORY.md` and the six companion
architecture documents. The mapping from evidence to decision is in
`RESEARCH_TO_ARCHITECTURE_MAPPING.md`.
