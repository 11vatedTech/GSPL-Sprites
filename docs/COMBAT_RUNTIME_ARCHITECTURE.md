# Combat runtime architecture

The engine-neutral combat runtime owns authoritative combat state independently
of animation, rendering, physics integration, and target engines. A bounded
program declares typed abilities with target rules, resource cost, cooldown,
range, and ordered damage, healing, or status effects. Actor state records team,
health, resource, integer millimeter position, cooldown expiry, and bounded
status expiry.

Commands are tick-addressed and fail closed when actors or abilities are absent,
time moves backward, an actor is defeated, target affiliation is invalid,
resource or cooldown admission fails, or the target is outside range. Integer
distance tests avoid floating-point simulation authority. Effects clamp damage
and healing to valid bounds, status application is capacity checked, and every
applied effect emits a monotonically sequenced semantic event.

Execution and time advancement operate on candidates and commit only after full
state validation. A rejected command therefore cannot consume resources, start
a cooldown, partially apply a multi-effect ability, or advance sequence state.

This slice establishes typed hit/effect authority. Collision-derived hit
confirmation, defense modifiers, projectiles, area queries, combo graphs,
cancel windows, and transformations remain separate governed layers. They must
produce or consume this semantic contract instead of encoding combat solely in
animation markers.
