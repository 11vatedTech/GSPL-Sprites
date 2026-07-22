# gspl-types
## Purpose
Type system, dimensional safety
## ADDED Requirements
### Requirement: Primitive types
The type system SHALL define Bool, Int, UInt, Fixed, String, Color, Duration, Distance, Angle primitives.
#### Scenario: Primitive type has correct representation
Given a primitive type, When queried for its string representation, Then it SHALL match the expected name.
### Requirement: Dimensional safety
Cross-dimension arithmetic SHALL be rejected at compile time.
#### Scenario: Duration + Distance rejected
Given a Duration and a Distance type, When checking dimension compatibility, Then they SHALL be incompatible.
### Requirement: Type assignability
The type system SHALL enforce assignability rules (Int to Fixed, UInt to Int, Optional wrapping).
#### Scenario: Int assigned to Fixed
Given an Int value, When assigning to a Fixed variable, Then the assignment SHALL be valid.

