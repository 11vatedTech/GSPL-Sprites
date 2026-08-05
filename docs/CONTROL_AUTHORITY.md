# GSPL Sprites — Control Authority

**Doctrine:** One canonical entity + one control policy. The entity carries semantic and gameplay state; the controller produces intents/commands.

```
Controller (policy)
  → intent / command
    → authoritative simulation (combat, transformations, movement)
      → animation/performance intent
        → visual manifestation
```

## Policies

| Policy | Produces | Future Work |
|---|---|---|
| Player | Input-driven commands (facing, attacks, combos) | Full PlayerController |
| Living | Autonomous goal/action selection | Implemented (existing living_runtime) |
| AI | Heuristic or learned decisions | Not yet |
| Scripted | Pre-authored sequence intents | Not yet |
| Network | Replicated remote authority | Not yet |
| Cinematic | Fixed-camera timeline events | Not yet |
| Hybrid | Multi-source priority arbitration | Not yet |

## Current Proof

The existing Living runtime (`LivingRuntimeProgram` → `step_living_runtime`) has been proven to be one control policy (`living_control_policy()`), not the definition of the entity.

API: `include/gspl_sprites/control.hpp` — `ControlAuthority`, `ControlPolicy`, `living_control_policy()`.
