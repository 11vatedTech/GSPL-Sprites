# Silhouette and Shape Language

## Core question

What makes a character recognizable before any internal detail is visible,
and how can that be made a measurable, computable constraint?

## Findings

### F1. Silhouette is the primary pre-attentive recognition channel
- **Concept:** The human visual system recognizes objects/characters
  primarily from boundary contours processed pre-attentively (peripheral
  vision, brief exposures); internal detail is secondary.
- **Evidence:** Recognition-by-components theory holds that objects are
  decomposed into volumetric primitives (geons) recoverable from
  silhouette features. Biederman, I. "Recognition-by-Components: A Theory
  of Human Image Understanding." *Psychological Review* 94(2), 1987.
- **GSPL implication:** The Visual Canon must treat silhouette as a
  first-class, *testable* identity structure with anchors, masses,
  protrusions, concavities and negative spaces — not as a by-product of
  part placement.

### F2. The "solid black fill" test
- **Concept:** Pokémon creature designs (Ken Sugimori methodology) must be
  identifiable as a solid black silhouette alone; if the filled contour
  cannot be distinguished from peers, the outer contour is redesigned
  before any interior work.
- **Source:** Game Freak/Sugimori developer interviews; Game Informer, 2017
  (direct); Yahoo Lifestyle design retrospective, 2026 (interpretation).
- **GSPL implication:** A computable silhouette-distinction metric between
  canon-defined entities (contour signatures, radial descriptors) is a
  validation gate for the canon itself.

### F3. Shape language: round / angular / tapered / block
- **Concept:** Primitive geometry families (circle, triangle, square and
  taper) are assigned to characters to communicate temperament; consistent
  shape language across a design creates rhythm and recognition.
- **Evidence:** 80 Level, *Character Design: Shape Language and
  Readability*, 2018 (practitioner); Artemisia College, *Shape Psychology
  in Character Design*, 2026 (practitioner synthesis).
- **Caveat (research interpretation):** the "psychology" claims are
  convention-based, not a hard perception law; the *transferable* core is
  that **consistent shape vocabulary + contrast** improves discrimination.
- **GSPL implication:** each structural region declares a shape-language
  family; the canon validates internal consistency and records contrast
  relationships between adjacent regions.

### F4. Shape contrast and visual hierarchy
- **Concept:** Contrasting forms (blocky torso vs tapered limbs) create
  focal hierarchy, direction and rhythm; monotony causes visual fatigue.
- **GSPL implication:** validation can quantify local curvature variance
  across adjacent regions (a region-adjacency metric) as a *diagnostic*,
  not a hard gate.
- **Source:** RMCAD, 2024 (practitioner synthesis).

### F5. Negative space as identity
- **Concept:** Negative pockets (between legs, under a cape, inside a tail
  curl) separate masses and are themselves signature features (e.g., a
  distinctive tail gap).
- **GSPL implication:** Silhouette canon carries explicit negative-space
  features with minimum separation; metrics measure negative-space
  preservation between forms/frames.
- **Source:** BINUS DKV, 2025 (practitioner); visual saliency and
  figure-ground literature.

### F6. Silhouette anchors and protrusions
- **Concept:** Every character has a small set of dominant contour anchors
  (head mass, ear tips, tail tip, muzzle point) that define its profile.
- **GSPL implication:** landmarks flagged `silhouette_anchor` get priority
  in resolution reduction and deformation-limit enforcement; protrusion and
  concavity features are declared, not inferred.
- **Source:** model-sheet practice (CHARACTER_DESIGN_AND_MODEL_SHEETS.md).

### F7. Gestalt grouping as measurable constraints
- **Concept:** Proximity, similarity, closure, continuation and figure-
  ground separation govern how parts group into a whole.
- **GSPL implication:** usable as diagnostics: connected-component analysis
  (accidental splits), color-similarity grouping, closure completeness of
  contours, continuity of contour flow.
- **Source:** IxDF, *What are the Gestalt Principles?*, 2026 (synthesis);
  Wertheimer/Koffka foundations (verify: original 1923 German paper).

### F8. Iconicity and perceptual simplification
- **Concept:** Recognition performance is preserved under strong
  simplification when *signature structure* (landmark topology, macro
  silhouette, two-tone contrast) survives.
- **GSPL implication:** resolution canon — features carry
  `recognition_importance` and `minimum_resolution`; simplification
  substitutes lower-fidelity renderings without dropping signature
  structure.
- **Source:** mascot/icon design practice (80 Level 2021); recognition
  literature (Biederman 1987).

### F9. Character recognition at small scale
- **Concept:** Thumbnail legibility is the canonical test used across
  games/merchandise pipelines; features that do not survive 16x16 must not
  be load-bearing for identity.
- **GSPL implication:** pixel-scale landmark requirements and
  feature-omission rules are part of the canon, not the renderer.
- **Source:** Game Informer Pokémon feature 2017 (direct); Pokémon design
  practice.

## GSPL synthesis

The silhouette canon is a **declared structure** — dominant masses,
anchors, protrusions, concavities, negative spaces — with **measurable
tests** (occupancy, connected components, anchor alignment, negative-space
preservation, cross-entity distinction). It is the enforcement point for
the corpus rule: *identity must survive resolution loss, deformation and
restyling*.
