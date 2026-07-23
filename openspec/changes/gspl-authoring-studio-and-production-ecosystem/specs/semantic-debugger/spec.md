## ADDED Requirements

### Requirement: Line breakpoints
The debugger SHALL support setting and clearing breakpoints on source lines in GSPL sprite definitions.

#### Scenario: Set and hit a line breakpoint
- **WHEN** the user sets a breakpoint on line 42 of a sprite definition and runs the sprite
- **THEN** execution pauses at line 42 before the instruction executes and the debugger displays the current source line

#### Scenario: Clear a line breakpoint
- **WHEN** the user removes a breakpoint from line 42
- **THEN** execution no longer pauses at that line

### Requirement: Gene breakpoints
The debugger SHALL support breakpoints triggered when a specific gene is evaluated during sprite compilation.

#### Scenario: Hit a gene breakpoint
- **WHEN** the user sets a breakpoint on gene `color_mutation` and compiles a sprite that invokes that gene
- **THEN** execution pauses at the first instruction of the gene body

### Requirement: Field breakpoints
The debugger SHALL support breakpoints triggered when a specific semantic field is read or written.

#### Scenario: Field write breakpoint
- **WHEN** the user sets a write breakpoint on the `Rights` field and any gene assigns to it
- **THEN** execution pauses before the assignment completes

### Requirement: Expression breakpoints
The debugger SHALL support breakpoints triggered when a watch expression evaluates to true at any point during execution.

#### Scenario: Expression breakpoint triggers on condition
- **WHEN** the user sets an expression breakpoint with condition `entropy > 0.85` and execution reaches a state where entropy exceeds 0.85
- **THEN** execution pauses and the debugger highlights the current expression in the source view

### Requirement: Step-over, step-into, step-out
The debugger SHALL provide step-over, step-into, and step-out commands that advance execution by one semantic unit.

#### Scenario: Step-over a gene call
- **WHEN** the user presses step-over while paused on a line that calls a gene
- **THEN** execution advances to the next line after the gene call without entering the gene body

#### Scenario: Step-into a gene
- **WHEN** the user presses step-into while paused on a gene call
- **THEN** execution enters the gene body and pauses at its first instruction

#### Scenario: Step-out of a gene
- **WHEN** the user presses step-out while paused inside a gene body
- **THEN** execution completes the gene and pauses at the instruction after the original call site

### Requirement: Call stack display
The debugger SHALL display the current call stack showing gene invocation hierarchy with source location for each frame.

#### Scenario: Call stack during nested gene calls
- **WHEN** execution pauses inside a deeply nested gene invocation
- **THEN** the call stack panel shows each active frame with its gene name, file, and line number

### Requirement: Watch expressions
The debugger SHALL allow the user to register arbitrary GSPL expressions for continuous evaluation and display.

#### Scenario: Evaluate a watch expression
- **WHEN** the user adds a watch expression `field.footprint_area` and execution advances
- **THEN** the watch panel updates the expression value at each pause, showing the current result or an error diagnostic

### Requirement: Variable inspection tree
The debugger SHALL display the current semantic entity state as an expandable tree of fields, sub-fields, and their values.

#### Scenario: Inspect entity state
- **WHEN** execution pauses at a breakpoint
- **THEN** the variable tree shows all entity fields with their current values, grouped by category, and nested structures are expandable

### Requirement: State timeline
The debugger SHALL record and display a timeline of entity state changes across execution steps.

#### Scenario: Navigate state timeline
- **WHEN** execution has paused after several steps
- **THEN** the user can rewind to any prior step in the timeline and see the full entity state at that point

### Requirement: Conditional breakpoints
The debugger SHALL support attaching a GSPL boolean expression to any breakpoint so that it only triggers when the expression evaluates to true.

#### Scenario: Conditional line breakpoint
- **WHEN** the user adds condition `generation > 10` to a line breakpoint and execution passes that line with generation values both below and above 10
- **THEN** execution only pauses when the line is reached and generation is greater than 10

### Requirement: Expression evaluation
The debugger SHALL provide a REPL panel for evaluating arbitrary GSPL expressions against the current paused state.

#### Scenario: Evaluate expression in REPL
- **WHEN** execution is paused and the user enters `entity.field.footprint_width * 2` in the REPL panel
- **THEN** the debugger evaluates the expression against the current entity state and displays the result

### Requirement: Attach to running synthesis
The debugger SHALL support attaching to an already-running synthesis process without restarting it.

#### Scenario: Attach to live synthesis
- **WHEN** a synthesis process is in progress and the user initiates an attach command
- **THEN** the debugger connects to the running process, displays its current state, and accepts breakpoint commands

### Requirement: Load saved traces for debugging
The debugger SHALL support loading previously captured execution traces and replaying them in debugging mode.

#### Scenario: Debug from a saved trace
- **WHEN** the user loads a `.gspl-trace` file into the debugger
- **THEN** the source view, state tree, and timeline reflect the trace contents and the user can step through the trace as if live
