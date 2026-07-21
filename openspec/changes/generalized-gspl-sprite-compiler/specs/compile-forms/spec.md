## ADDED Requirements

### Requirement: compile() SHALL enforce resource limits
The `compile()` function SHALL check each element count (forms, transformations, bones, sockets, clips, abilities) against ResourceLimits before proceeding. If any limit is exceeded, compile() SHALL throw a runtime_error with a RESOURCE_ prefix diagnostic.

#### Scenario: Form count exceeds compile limit
- **WHEN** a SpriteSeed has more forms than ResourceLimits.max_forms
- **THEN** `compile()` throws an exception with a diagnostic containing "RESOURCE_FORMS"
