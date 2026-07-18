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

The current semantic delta covers combat capacity and ability availability.
Appearance, anatomy, behavior, animation, equipment, projection, and target
implications require additional typed delta families. Canonical persistence and
replication must encompass transformation state before networked authority can
claim form convergence.
