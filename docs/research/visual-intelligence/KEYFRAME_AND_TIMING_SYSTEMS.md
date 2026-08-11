# Keyframe and Timing Systems

## Core question

How is motion structured in time — keys, breakdowns, inbetweens, arcs,
spacing, timing — and what computational machinery already exists to
represent and generate it?

## Findings

### F1. Key poses → breakdowns → inbetweens
- **Concept:** Extreme keys define narrative beats; breakdowns define the
  trajectory path; inbetweens fill gaps. This is a sparse-to-dense
  hierarchy, not undifferentiated interpolation.
- **Source:** Thomas & Johnston 1981 (direct); Williams 2001 (direct).
- **GSPL implication:** the key-pose system (KEY POSE SYSTEM in
  VISUAL_PERFORMANCE_ARCHITECTURE.md) makes keys semantic (line of action,
  CoM, contacts, gaze, force, silhouette goal) and derives inbetweens
  around them — including optional deterministic procedural inbetweening
  of part trajectories.

### F2. Timing charts and easing curves
- **Concept:** Timing charts specify exact inbetween distribution
  (slow-in/slow-out); computationally these are easing curves (Hermite/
  Bézier interpolation with tangent control).
- **Source:** Williams 2001 (direct); Eberly, *Game Physics*, 2010
  (research textbook, verify publisher).
- **GSPL implication:** phase-based timing curves are canon/action data
  (anticipation fast-out, impact hold, recovery slow-in).

### F3. Arcs
- **Concept:** Natural action follows curved trajectories; straight-line
  interpolation reads robotic.
- **GSPL implication:** part motion during actions should be arc-constrained
  (velocity-intent curvature), a determinism-safe, testable property.
- **Source:** Williams 2001 (direct).

### F4. Spacing and timing determine weight
- **Concept:** Spacing (Δp/Δt) and timing (frame count) encode mass,
  energy and cadence; the same pose reads differently with different
  spacing.
- **GSPL implication:** PerformanceIntent carries timing cadence and force
  magnitude; phase durations are data, never hardcoded.
- **Source:** Williams 2001 (direct); Thomas & Johnston 1981 (direct).

### F5. Anticipation, follow-through, overlapping action
- **Concept:** Anticipation precedes action; parts continue after the mass
  stops (follow-through); parts move at staggered rates (overlapping
  action) — classically realized with spring-damper/Verlet chains for
  secondary elements.
- **Source:** Thomas & Johnston 1981 (direct); Williams 2001 (direct).
- **GSPL implication:** phases model anticipation/follow-through; secondary
  motion (already in PerformanceState) is the semantic hook for
  deterministic procedural follow-through.

### F6. Exposure sheets (x-sheets)
- **Concept:** Tabular multi-track timelines (action, camera, dialog,
  layers per frame) are the production master schedule.
- **GSPL implication:** matches the existing deterministic tick-based
  runtime/event model; the animation ontology is timeline-first, not
  image-first.
- **Source:** Thomas & Johnston 1981 (direct).

### F7. FK/IK and FABRIK
- **Concept:** FK propagates transforms parent-to-child; IK solves joint
  angles from end-effector targets; FABRIK is an O(n) iterative heuristic
  avoiding matrix inverses.
- **Source:** Buss, *Introduction to Inverse Kinematics*, UCSD, 2004;
  Aristidou & Lasenby, *FABRIK*, Graphical Models 73(5), 2011 (research).
- **GSPL implication:** future pose resolution may use FK with
  deterministic IK for contact/look-at constraints; geometry stays
  semantic (landmarks + constraints), solvers are a below-the-line concern.

### F8. Pose spaces
- **Concept:** Poses as points in high-dimensional space; interpolated via
  RBF/PCA; used for pose-space deformation corrections.
- **Source:** Lewis, Cordner, Fong, *Pose Space Deformation*, SIGGRAPH 2000
  (research).
- **GSPL implication:** deformation envelopes are parameterized by pose/
  phase/commitment — a semantic pose space, not a raw joint space.

### F9. Motion graphs and blend spaces
- **Concept:** Motion graphs stitch clips at matching frames; blend spaces
  blend clips by control parameters (speed, direction) via barycentric
  interpolation.
- **Source:** Kovar, Gleicher, Pighin, *Motion Graphs*, SIGGRAPH 2002
  (research); Epic Games, *Blend Spaces in Unreal Engine* docs.
- **GSPL implication:** action selection is a graph/parameter problem at
  the runtime layer; the canonical animation ontology must not preclude
  these — key poses with semantic labels are the compatible unit.

### F10. Motion matching and learned motion
- **Concept:** Database search for the best-matching clip/frame against
  predicted trajectory; learned variants embed the database.
- **Source:** Clavet, *Motion Matching for For Honor*, GDC 2016 (direct);
  Holden et al., *Learned Motion Matching*, ACM TOG 2020 (research).
- **GSPL implication:** future learned specialists may propose motion;
  proposals become typed key-pose/intent sequences validated by the canon —
  never silent authority.

### F11. Retargeting
- **Concept:** Transfer motion between skeletons with different
  proportions/hierarchies via bone mapping + local-space rotations +
  proportional scale correction.
- **Source:** Agata & Igarashi, *Motion Control via Metric-Aligning Motion
  Matching*, SIGGRAPH 2025 (research); industry practice.
- **GSPL implication:** key poses are defined on landmarks/roles, which
  retarget across entities sharing a canon structure — the canonical
  animation ontology makes retargeting structural by construction.

### F12. Procedural animation
- **Concept:** Runtime generation via ODEs/state machines/rule systems for
  terrain adaptation, balance recovery, secondary motion.
- **Source:** Holden et al. 2020 (research); game practice.
- **GSPL implication:** procedural motion is a *realizer* below the
  semantic layer; determinism is preserved by construction.

## GSPL synthesis

Motion is **sparse semantic keys + timing curves + derived inbetweens +
bounded procedural realization**. The PerformanceIntent/KeyPose/Phase
representation is the semantic layer; FK/IK, springs, graphs and learned
systems are realizers below it. The existing fixed-tick runtime and event
model already provides the timeline substrate.
