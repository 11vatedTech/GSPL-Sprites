# living-combat-wiring

## Purpose

Extend the LivingRuntime with combat-driven utility AI form selection, combat perception sensors, directional lightning ability, transformation VFX, and state persistence.

## Requirements

### Requirement: LivingRuntime SHALL drive form selection via utility AI
The existing `LivingSpriteRuntime` utility-AI system SHALL be extended with a form-selection action that evaluates utility scores based on current combat state, health, and form constraints. When a transformation's utility threshold is met, the runtime SHALL enqueue a form-switch command.

#### Scenario: Low health triggers Hurt form
- **WHEN** Voltfox health drops below 25% of max
- **THEN** the utility AI assigns high utility to the Hurt form and enqueues a transformation

### Requirement: Combat perception SHALL feed combat events into the utility AI
A `CombatPerceptionSensor` SHALL translate incoming `CombatEvent` records (damage taken, damage dealt, ability used, status applied) into perception signals that modify utility scores for relevant goals (survival, aggression, curiosity).

#### Scenario: Taking damage increases survival goal
- **WHEN** Voltfox takes damage
- **THEN** the utility score for the survival goal increases by a configured delta value

### Requirement: Directional lightning SHALL be a mapped combat ability
The existing `CombatAbility` system SHALL be extended to include a `directional_lightning` ability that reads the current form's directional orientation and applies a targeted lightning strike effect.

#### Scenario: Lightning ability requires Attack form
- **WHEN** Voltfox uses the directional lightning ability while not in Attack form
- **THEN** the ability command fails with a form-mismatch error

#### Scenario: Lightning deals configured damage
- **WHEN** directional lightning is used in Attack form
- **THEN** it deals damage equal to the ability's base damage value to the target

### Requirement: Form transformations SHALL trigger VFX
When a transformation occurs (form switch), the runtime SHALL trigger a visual effect: a brief flash frame or particle burst on the 2D/2.5D manifestation, and an aura-size animation on the 3D manifestation.

#### Scenario: Idle → Attack triggers VFX
- **WHEN** transforming from Idle to Attack form
- **THEN** the manifestation driver emits a transformation VFX event with expected duration

### Requirement: Living runtime state SHALL be persisted and restored
The existing `RuntimePersistenceHeader` SHALL be extended to include the current form ID, combat state (HP, statuses, cooldowns), and animation state. `save_runtime()` and `restore_runtime()` SHALL correctly round-trip this data.

#### Scenario: Full state round-trip
- **WHEN** saving runtime state and restoring into a fresh runtime
- **THEN** the restored runtime's form, HP, active statuses, and animation state match the saved values
