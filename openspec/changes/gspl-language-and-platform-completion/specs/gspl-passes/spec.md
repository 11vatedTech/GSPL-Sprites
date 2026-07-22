# gspl-passes
## Purpose
Compiler-pass architecture, dependency graph, scheduling
## ADDED Requirements
### Requirement: Pass dependency graph
Passes SHALL declare typed inputs/outputs and the pass graph SHALL use topological scheduling.
#### Scenario: Passes execute in dependency order
Given registered passes, When scheduled, Then they SHALL execute in topological order respecting dependencies.
### Requirement: Incremental pass execution
The pass manager SHALL support incremental execution where completed passes are not re-executed.
#### Scenario: Already-completed pass skipped
Given a pass that already completed, When running the pipeline again, Then the pass SHALL not be re-executed.

