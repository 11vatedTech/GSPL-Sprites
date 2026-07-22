# gspl-genes
## Purpose
First-class typed Sprite Genes
## ADDED Requirements
### Requirement: Gene descriptor types
Sprite Genes SHALL use typed C++ structs (not strings) for each gene family.
#### Scenario: Gene descriptor has kind and payload
Given a GeneDescriptor, When inspected, Then it SHALL have a GeneKind and a typed payload.
#### Scenario: Built-in gene registration
Given the GeneRegistry, When queried for built-in genes, Then they SHALL be discoverable by identity.
### Requirement: Gene composition
Genes SHALL support composition with inheritance and overrides.
#### Scenario: Base gene can be overridden
Given a base gene and a derived gene, When composed, Then the derived gene fields SHALL override the base.
### Requirement: Conflict detection
Gene composition SHALL detect and report conflicts.
#### Scenario: Conflicting genes produce diagnostic
Given two genes with conflicting fields, When composed, Then a conflict diagnostic SHALL be emitted.

