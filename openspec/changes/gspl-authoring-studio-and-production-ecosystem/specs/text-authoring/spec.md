## ADDED Requirements

### Requirement: GSPL syntax highlighting
The text editor SHALL apply syntax highlighting using token classes (keyword, identifier, string, comment, number, operator, type, punctuation) with configurable color themes.

#### Scenario: Keywords are colored
- **WHEN** the user opens a `.gspl` file
- **THEN** all language keywords (e.g. `entity`, `behavior`, `gene`, `if`, `return`) SHALL be rendered in the keyword color of the active theme

#### Scenario: Theme switching
- **WHEN** the user selects a different color theme in Preferences
- **THEN** all open editors SHALL re-apply syntax highlighting with the new theme colors immediately

### Requirement: Multi-cursor editing
The text editor SHALL support multi-cursor editing with simultaneous typing, copy, paste, and cursor movement.

#### Scenario: Add cursors with Alt+Click
- **WHEN** the user holds Alt and clicks at three different lines
- **THEN** three cursors SHALL be active and typing SHALL insert the same text at all cursor positions simultaneously

#### Scenario: Add cursors for all occurrences
- **WHEN** the user selects a word and presses Ctrl+Shift+L
- **THEN** a cursor SHALL be placed at each occurrence of that word in the document

### Requirement: Bracket matching
The text editor SHALL highlight matching brackets and auto-insert closing brackets for `(`, `[`, `{`, and matching pairs.

#### Scenario: Highlight matching pair
- **WHEN** the cursor is adjacent to a `{` or `}`
- **THEN** both the opening and closing brace SHALL be highlighted with a colored border or background

#### Scenario: Auto-insert closing bracket
- **WHEN** the user types `[`
- **THEN** a closing `]` SHALL be inserted automatically and the cursor SHALL be placed between them

### Requirement: Auto-indent
The text editor SHALL automatically indent new lines based on the previous line's indentation and the language's block structure.

#### Scenario: Indent after opening brace
- **WHEN** the user presses Enter after typing `entity Foo {`
- **THEN** the new line SHALL be indented one level deeper than the opening brace

#### Scenario: Outdent after closing brace
- **WHEN** the user types `}` on a new line
- **THEN** the closing brace SHALL be outdented to match the indentation of the opening block

### Requirement: Code folding
The text editor SHALL support collapsing and expanding code regions based on folding ranges provided by the language service.

#### Scenario: Collapse all
- **WHEN** the user presses Ctrl+K Ctrl+0
- **THEN** all foldable regions in the document SHALL collapse

#### Scenario: Expand region
- **WHEN** the user clicks the fold icon next to a collapsed region
- **THEN** the region SHALL expand to show its full content

### Requirement: Split-view editing
The text editor SHALL support splitting the editor pane into multiple views of the same document or different documents.

#### Scenario: Split horizontally
- **WHEN** the user selects Window > Split > Split Horizontal
- **THEN** the current editor SHALL split into two panes stacked vertically, both showing the same document

#### Scenario: Edit in one split reflects in other
- **WHEN** the user types in the top split pane
- **THEN** the bottom split pane SHALL show the same content changes in real time

### Requirement: Minimap
The text editor SHALL display a minimap that provides a zoomed-out overview of the entire document.

#### Scenario: Minimap scrolls the editor
- **WHEN** the user drags the minimap viewport rectangle
- **THEN** the editor SHALL scroll to the corresponding position in the document

#### Scenario: Toggle minimap visibility
- **WHEN** the user selects View > Toggle Minimap
- **THEN** the minimap SHALL be shown if hidden or hidden if shown

### Requirement: Incremental search
The text editor SHALL support incremental search with live match highlighting, case-sensitive toggle, and regex mode.

#### Scenario: Search highlights all matches
- **WHEN** the user types "sprite" in the search bar
- **THEN** all occurrences of "sprite" in the document SHALL be highlighted and the current match SHALL be distinguished

#### Scenario: Search navigation with F3
- **WHEN** the user presses F3 after a search
- **THEN** the cursor SHALL move to the next match

### Requirement: Inline error squiggles
The text editor SHALL display diagnostic squiggles (red for errors, green for warnings, blue for info) inline on the affected text range.

#### Scenario: Error squiggle on invalid code
- **WHEN** the language service produces a parse or type diagnostic
- **THEN** a wavy red underline SHALL appear under the affected text range

#### Scenario: Hover squiggle for diagnostic message
- **WHEN** the user hovers over a squiggled text range
- **THEN** a tooltip SHALL show the full diagnostic message

### Requirement: Quick-fix suggestions
The text editor SHALL suggest code actions (quick-fixes) for diagnostics and allow the user to apply them with a single click or key press.

#### Scenario: Light bulb on diagnostics
- **WHEN** a diagnostic has an associated fix
- **THEN** a light bulb icon SHALL appear near the squiggled range

#### Scenario: Apply quick-fix
- **WHEN** the user clicks the light bulb and selects a quick-fix
- **THEN** the editor SHALL apply the fix and the diagnostic SHALL be re-evaluated

### Requirement: Code snippets
The text editor SHALL support inserting code snippets with tab-stop navigation and placeholder values.

#### Scenario: Expand snippet from completion
- **WHEN** the user types "entity" and selects the entity-snippet from code completion
- **THEN** the editor SHALL insert an entity template with placeholders for name and members, and select the first placeholder

#### Scenario: Tab through placeholders
- **WHEN** the user presses Tab after expanding a snippet
- **THEN** the cursor SHALL move to the next placeholder in the snippet

### Requirement: Line numbers and word wrap
The text editor SHALL display line numbers in the gutter and support configurable word wrap.

#### Scenario: Line numbers displayed
- **WHEN** a document is open
- **THEN** each line SHALL have a line number displayed in the left gutter, starting at 1

#### Scenario: Toggle word wrap
- **WHEN** the user selects View > Toggle Word Wrap
- **THEN** lines exceeding the editor width SHALL wrap to the next line instead of requiring horizontal scrolling
