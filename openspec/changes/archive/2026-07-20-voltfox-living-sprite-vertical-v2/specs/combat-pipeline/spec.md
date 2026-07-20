# Complete Combat Pipeline

## Requirements
1. Form gate: directional_lightning usable only in Attack/Special or storm form
2. Resource cost consumed on ability activation
3. Cooldown begins after use
4. Full lifecycle: activation → anticipation → release → projectile effect → collision → damage → status
5. Projectile originates from evaluated muzzle socket
6. Bolt collision tested against target hurtbox
7. Numeric damage applied on collision
8. Timed electrical status with duration_ticks
9. Every stage produces deterministic RuntimeEvent
10. Early interruption cancels activation without resource cost
11. Invalid target rejected without resource consumption
12. Form mismatch produces diagnostic

## Scenarios
1. Full lifecycle executes with all events emitted
2. Cancellation before release: no cost, no cooldown
3. Ability rejected in wrong form
4. Bolt collision applies expected damage
5. Electrical status duration matches configured ticks
