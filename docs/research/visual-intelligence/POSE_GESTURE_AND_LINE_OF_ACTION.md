# Pose, Gesture and Line of Action

## Core question

What does a pose *mean* beyond joint angles, and how is that meaning
encoded so it can be validated and realized computationally?

## Findings

### F1. Line of action
- **Concept:** An imaginary directional axis through the core of the
  character (root-to-head spine curve) that dictates posture, energy and
  balance of a pose. Poses are designed around it.
- **Source:** Williams, *Animator's Survival Kit*, 2001 (direct); Blair
  1994 (direct).
- **GSPL implication:** a KeyPose carries an explicit line of action
  (direction + curvature); pose validation checks that landmark placement
  is coherent with it (diagnostic, not hard gate).

### F2. Gesture and silhouette goal
- **Concept:** A strong pose is readable as a silhouette first; gesture
  drawing starts from the spine/gesture line and builds anatomy onto it.
- **GSPL implication:** KeyPose declares a *silhouette goal* (target
  positions for priority landmarks/extremities) that the performance
  system resolves before secondary detail.
- **Source:** Preston Blair (direct); gesture-drawing practice.

### F3. Balance, center of mass and support polygon
- **Concept:** A pose communicates physical state: planted, weight-shifted,
  recoiling, intentionally imbalanced. Balance is judged by center of mass
  projection against support contacts.
- **GSPL implication:** KeyPose carries center of mass and support
  contacts; balance intent is a first-class field (`planted`, `shifted`,
  `recoil`, `intentional_imbalance`); validation can measure CoM-vs-support
  deviation as a diagnostic.
- **Source:** animation practice (12 principles: staging/balance);
  Robotics/IK literature (support polygon in balance control — verify
  specific papers, e.g., balance-control surveys).

### F4. Anticipation and compression
- **Concept:** Actions are preceded by reverse-direction preparation
  (crouch before leap); anticipation frames are *semantic prefix states*,
  often compressed in time.
- **GSPL implication:** performance phases explicitly include
  anticipation/initiation/extension/contact/impact/recoil/follow_through/
  recovery/settle; the phase model is per-action, extensible.
- **Source:** Thomas & Johnston 1981 (direct); Dragon Ball action analysis
  (HYBRID_2D_3D_PRODUCTION.md).

### F5. Staging and focal hierarchy
- **Concept:** The presentation makes the idea unmistakably clear: camera
  angle, silhouette visibility, focal hierarchy, avoiding occlusion of
  intent.
- **GSPL implication:** cinematic representation (CameraSpec) keeps
  framing/scale/screen-direction as data so staging remains possible
  without coupling to the canon.
- **Source:** Thomas & Johnston 1981 (direct).

### F6. Asymmetric balance and counter-pasting
- **Concept:** Dynamic poses use controlled asymmetry counterbalanced by
  optical weight (leaning forward with trailing cape, hip pushed out).
- **GSPL implication:** intentional imbalance is valid; the pose declares
  it rather than failing a balance check.
- **Source:** BINUS 2025 (practitioner); gesture practice.

### F7. Weight, timing and spacing in poses
- **Concept:** Spacing (per-frame displacement) and timing (frame count)
  determine perceived weight and energy; the same pose reads differently
  at different spacings.
- **Source:** Williams 2001 (direct).
- **GSPL implication:** performance intent carries force magnitude and
  timing cadence; the key-pose system records spacing/anticipation data
  per phase (KEYFRAME_AND_TIMING_SYSTEMS.md).

### F8. Exaggeration and off-model tolerance
- **Concept:** Pushing beyond realism clarifies intent — but identity
  anchors (signature props, landmark topology) remain rigidly fixed under
  even wild deformation (One Piece practice).
- **GSPL implication:** deformation envelopes (SEMANTIC_DEFORMATION_
  ARCHITECTURE.md) declare hard identity boundaries per structure; pose
  exaggeration is bounded per structure, never global.
- **Source:** One Piece production analysis (HYBRID_2D_3D_PRODUCTION.md);
  animation practice.

## GSPL synthesis

A pose is a **semantic state**, not a joint-angle vector: line of action,
balance intent, support contacts, gaze, force, silhouette goal, expression
and deformation envelope usage. The KeyPose representation makes these
fields first-class so the performance system can validate poses, derive
breakdowns/inbetweens deterministically, and communicate intent to many
visual systems at once.
