## ADDED Requirements

### Requirement: Real-time parse diagnostics
The language service SHALL report parse errors in real time as the user types, with error location (line, column, length) and a human-readable message.

#### Scenario: Syntax error underlined immediately
- **WHEN** the user types an unclosed string literal (e.g. `"hello`)
- **THEN** the language service SHALL produce a parse diagnostic at the string start with message "Unterminated string literal" within 500 ms of the last keystroke

#### Scenario: Error list is updated
- **WHEN** parse diagnostics are produced
- **THEN** the Error List tool window SHALL display each diagnostic with severity, file, line, column, and message

### Requirement: Real-time type diagnostics
The language service SHALL report type errors including mismatched types, undefined symbols, and invalid assignments.

#### Scenario: Type mismatch on assignment
- **WHEN** the user writes `int x = "hello"`
- **THEN** the language service SHALL produce a type diagnostic "Cannot assign value of type 'string' to variable of type 'int'"

#### Scenario: Undefined symbol reference
- **WHEN** the user writes `print(undefinedVar)`
- **THEN** the language service SHALL produce a diagnostic "Cannot resolve symbol 'undefinedVar'"

### Requirement: Real-time semantic diagnostics
The language service SHALL report semantic errors such as duplicate definitions, invalid visibility, and constraint violations.

#### Scenario: Duplicate entity definition
- **WHEN** the user defines two entities with the same name in the same scope
- **THEN** the language service SHALL produce a semantic diagnostic "Duplicate definition of entity 'Foo'"

#### Scenario: Invalid constraint expression
- **WHEN** the user writes a constraint that references a non-existent attribute
- **THEN** the language service SHALL produce a semantic diagnostic identifying the invalid reference

### Requirement: Code completion
The language service SHALL provide context-sensitive code completion suggestions including keywords, symbols, member access, and snippets.

#### Scenario: Keyword completion
- **WHEN** the user types `ent` in an entity definition context
- **THEN** the completion list SHALL include `entity` as the top suggestion

#### Scenario: Member completion after dot
- **WHEN** the user types `mySprite.`
- **THEN** the completion list SHALL show all accessible members of the `mySprite` entity

### Requirement: Hover information
The language service SHALL display type, documentation, and source location when hovering over a symbol.

#### Scenario: Hover over variable
- **WHEN** the user hovers the mouse over a variable name
- **THEN** a tooltip SHALL show the variable's type, declared-at location, and any attached doc comment

#### Scenario: Hover over keyword
- **WHEN** the user hovers over the `entity` keyword
- **THEN** a tooltip SHALL show a brief summary of the entity declaration syntax

### Requirement: Go-to-definition
The language service SHALL navigate to the definition of a symbol when the user invokes go-to-definition.

#### Scenario: Navigate to entity definition
- **WHEN** the user Ctrl+clicks a reference to entity `Warrior`
- **THEN** the editor SHALL scroll to and highlight the line where `Warrior` is defined

#### Scenario: Navigate to imported symbol
- **WHEN** the user invokes go-to-definition on a symbol imported from another module
- **THEN** the editor SHALL open the source file of the imported module at the definition line

### Requirement: Find references
The language service SHALL find all references to a symbol across the current project and display results in a unified pane.

#### Scenario: Find all usages
- **WHEN** the user right-clicks a symbol and selects "Find All References"
- **THEN** a results pane SHALL list every reference grouped by file with line numbers and surrounding context

#### Scenario: Reference count in status bar
- **WHEN** references are found
- **THEN** the status bar SHALL display "N references found"

### Requirement: Rename symbol
The language service SHALL rename a symbol and all its references across the project with preview and undo support.

#### Scenario: Rename with preview
- **WHEN** the user renames a symbol and selects Preview
- **THEN** a diff-like preview SHALL show all affected locations before applying changes

#### Scenario: Rename triggers diagnostics refresh
- **WHEN** a rename is applied
- **THEN** all diagnostics SHALL be re-evaluated within 500 ms

### Requirement: Document symbols
The language service SHALL provide a structured list of symbols defined in the current document, accessible from the Outline tool window.

#### Scenario: Outline shows entities and functions
- **WHEN** the user opens a `.gspl` file containing entity and function definitions
- **THEN** the Outline tool window SHALL list all entities (with their members) and top-level functions in hierarchy

#### Scenario: Click outline item navigates
- **WHEN** the user clicks an item in the Outline
- **THEN** the editor SHALL scroll to the definition of that symbol

### Requirement: Folding ranges
The language service SHALL compute folding ranges for code blocks, comments, and import sections.

#### Scenario: Fold entity body
- **WHEN** the user clicks the fold icon next to an entity declaration
- **THEN** the entity body SHALL collapse to a single line showing `{ ... }`

#### Scenario: Fold comment block
- **WHEN** the user clicks the fold icon next to a multi-line comment
- **THEN** the comment SHALL collapse to a single line with `/* ... */`

### Requirement: Inline documentation
The language service SHALL parse doc comments (e.g. `///` or `/** */`) and surface them in hover, completion, and signature help.

#### Scenario: Signature help with parameter docs
- **WHEN** the user types the opening parenthesis of a function call
- **THEN** a signature help popup SHALL show the function signature and the doc comment for each parameter

#### Scenario: Doc comment on hover
- **WHEN** the user hovers over a documented symbol
- **THEN** the hover tooltip SHALL include the parsed doc comment rendered as formatted text

### Requirement: Syntax highlighting tokens
The language service SHALL produce token classifications (keyword, identifier, string, comment, number, operator) for the text editor to consume.

#### Scenario: Token classification on open
- **WHEN** a `.gspl` file is opened
- **THEN** the language service SHALL classify all tokens and the editor SHALL apply syntax highlighting within 200 ms

#### Scenario: Token updates on edit
- **WHEN** the user types a new keyword such as `entity`
- **THEN** the token classification SHALL update within 200 ms and the keyword SHALL receive the keyword color
