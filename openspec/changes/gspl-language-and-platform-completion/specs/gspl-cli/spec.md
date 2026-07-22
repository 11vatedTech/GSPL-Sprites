# gspl-cli
## Purpose
Complete command-line interface
## ADDED Requirements
### Requirement: CLI subcommands
The CLI SHALL support parse, check, compile, migrate, ir, graph, package, verify, and preview subcommands.
#### Scenario: User invokes --help
Given a user runs gsplc --help, Then the help text SHALL be printed to stdout And the exit code SHALL be 0.
#### Scenario: User compiles a file
Given a valid GSPL source file, When the user runs gsplc <file>, Then the compiler SHALL produce output without errors.
#### Scenario: User migrates legacy files
Given a legacy .sprite file, When the user runs gsplc --migrate <file>, Then the file SHALL be converted to GSPL format.
#### Scenario: User displays pass graph
When the user runs gsplc --graph, Then the pass dependency graph SHALL be printed in DOT format

