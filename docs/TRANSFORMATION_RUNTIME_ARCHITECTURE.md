# Transformation runtime architecture

Transformation programs preserve one stable entity identity while moving among
typed semantic forms. Each form declares deterministic maximum-health and
maximum-resource deltas plus the combat abilities available in that form.
Transitions declare source, destination, energy cost, integer-tick duration,
and reversibility. A reversible transition requires an explicit reverse
definition rather than an inferred mutation.

Validation rejects absent base forms, duplicate or unreachable forms, unknown or
duplicate abilities, malformed deltas, duplicate edges, missing explicit
reverses, invalid histories, and active transitions inconsistent with their
definition. History begins at the governed base form and must explain the
current form exactly, preserving identity continuity.

Beginning a transformation spends transformation energy and establishes a
fixed completion tick. Until completion, form-specific combat activation is
blocked. Completion applies the difference between old and new form deltas to
the matching authoritative combat actor, clamps current health and resource to
new maxima, records history, and changes form. Transformation and combat states
commit together only after both candidates validate.

Transformation programs have a canonical identity that incorporates the full
canonical combat-program identity. Transformation state uses the bounded
`gspl.transformation-state/0.1` format and records both identities, stable entity,
current form, energy, complete history, and any active transition. Loading
requires exact canonical bytes and full semantic validation. The canonical
state SHA-256 is replication convergence evidence: peers cannot agree while
using different form or combat semantics.

The current semantic delta covers combat capacity and ability availability.
Appearance, anatomy, behavior, animation, equipment, projection, and target
implications require additional typed delta families. An authenticated
replication transport and visual/animation consumers must bind this identity
rather than creating independent form authority.

The manifestation binding layer provides that consumer boundary for validated
skeletal animation graphs, transition clips, and 3D projections. It requires
exactly one binding for every form and transformation. Each derived frame
contains the authoritative transformation-state identity, stable entity, current
form, animation state, projection, and—while transitioning—the transition clip
and deterministic parts-per-million progress. Missing states, clips,
projections, incomplete coverage, invalid assets, or ticks outside an active
transition fail closed. The frame contains no independently mutable form state.
