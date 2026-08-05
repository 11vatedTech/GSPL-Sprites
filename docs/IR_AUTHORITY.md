# GSPL Sprites — IR Authority

Two IR types exist. Strategy C: designate canonical + compatibility adapter.

| Type | Namespace | Role | Producer | Consumers |
|---|---|---|---|---|
| `gspl::SpriteIr` | `gspl` | COMPILER INTERCHANGE (compatibility) | `IrGenPhase` | CLI diagnostics, headless evidence, IR persistence |
| `gspl::sprites::SpriteIr` | `gspl::sprites` | CANONICAL PRODUCTION IR | `SpriteIrLowering::lower(CanonicalEntity)` | Synthesis, packages, runtime wiring, all downstream production |

Both derive from `CanonicalEntity` — the single semantic authority.

API: `include/gspl/ir_authority.hpp` — `IrPathRole` enum, `ir_path_role_name()`.
