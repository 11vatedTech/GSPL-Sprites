## ADDED Requirements

### Requirement: Diff view for GSPL files
The git integration SHALL provide a structured diff view for GSPL files that understands the file's entity structure, showing entity-level additions, deletions, and modifications.

#### Scenario: Entity-level diff display
- **WHEN** the user views the diff of a `.gspl` file with changes
- **THEN** each added entity SHALL be prefixed with a green `+`, each removed entity with a red `-`, and modified entities SHALL show inline highlights per changed line

#### Scenario: Syntax highlighting in diff
- **WHEN** the diff view is open for a `.gspl` file
- **THEN** both sides of the diff SHALL display GSPL syntax highlighting

### Requirement: Stage and unstage files
The git integration SHALL allow staging and unstaging individual files or selected hunks from the diff view.

#### Scenario: Stage a file
- **WHEN** the user clicks the stage icon next to a modified file in the Git panel
- **THEN** the file SHALL move from "Changes" to "Staged Changes" and its diff SHALL update to show staged vs HEAD

#### Scenario: Stage a single hunk
- **WHEN** the user selects a specific hunk in the diff view and clicks "Stage Hunk"
- **THEN** only that hunk SHALL be staged, leaving other changes in the working tree

### Requirement: Commit with message
The git integration SHALL provide a commit dialog with a message input field, recent commit history, and a commit button.

#### Scenario: Create a commit
- **WHEN** the user enters "Add Fireball ability" in the commit message field and clicks Commit
- **THEN** a git commit SHALL be created with the staged changes and the provided message

#### Scenario: Commit message validation
- **WHEN** the user clicks Commit with an empty message
- **THEN** the commit SHALL be rejected and a warning SHALL be displayed

### Requirement: Branch display
The git integration SHALL show all local and remote branches in a branch panel with an indicator for the currently checked-out branch.

#### Scenario: Switch branch
- **WHEN** the user double-clicks a different branch in the branch panel
- **THEN** the repository SHALL check out that branch and the file tree SHALL refresh

#### Scenario: Branch upstream status
- **WHEN** the current branch is behind its upstream
- **THEN** the branch panel SHALL display "behind by N commits"

### Requirement: Status indicators in project tree
The git integration SHALL overlay status indicators (modified, added, deleted, untracked, conflicted) on files and folders in the Project Explorer.

#### Scenario: Modified file indicator
- **WHEN** a tracked `.gspl` file has uncommitted changes
- **THEN** the file icon in the Project Explorer SHALL show a blue "M" overlay

#### Scenario: Untracked file indicator
- **WHEN** a new file exists in the project directory that is not tracked by git
- **THEN** the file icon SHALL show a green "U" overlay

### Requirement: Blame annotations
The git integration SHALL display blame (annotated with author, date, and commit hash) for each line of a GSPL file in the editor gutter.

#### Scenario: Toggle blame view
- **WHEN** the user right-clicks the editor gutter and selects "Toggle Blame"
- **THEN** each line SHALL display the author name and commit date in the gutter area

#### Scenario: Blame tooltip shows full commit info
- **WHEN** the user hovers over a blame annotation in the gutter
- **THEN** a tooltip SHALL show the full commit hash, author, date, and commit message

### Requirement: `.gspl-git-ignore` support
The git integration SHALL respect a `.gspl-git-ignore` file that filters GSPL-specific artifacts (generated sprite sheets, cache files, build output) from git tracking.

#### Scenario: Ignored file hidden from changes
- **WHEN** a file matches a pattern in `.gspl-git-ignore`
- **THEN** the file SHALL not appear in the "Changes" list of the Git panel

#### Scenario: Create `.gspl-git-ignore` from template
- **WHEN** the user selects Git > Initialize GSPL Ignore
- **THEN** a `.gspl-git-ignore` file SHALL be created with default patterns for `build/`, `*.png`, `*.bmp`, and `cache/`

### Requirement: Unstaged diff highlighting
The git integration SHALL display unstaged changes with color-coded line highlights (green for additions, red for deletions, yellow for modifications) in the editor gutter.

#### Scenario: Unstaged change gutter marks
- **WHEN** a tracked file has unstaged modifications
- **THEN** the editor gutter SHALL show colored vertical bars on changed lines

#### Scenario: Revert unstanged change from gutter
- **WHEN** the user clicks a gutter change mark and selects "Revert Change"
- **THEN** the line or hunk SHALL revert to its committed state

### Requirement: Git log viewer
The git integration SHALL provide a graphical commit log viewer showing commit graph, author, date, and message for the current branch.

#### Scenario: View commit history
- **WHEN** the user selects Git > View History
- **THEN** a dialog SHALL display the commit log with a graph visualization showing branching and merging

#### Scenario: Click commit shows diff
- **WHEN** the user clicks a commit in the log viewer
- **THEN** the full diff for that commit SHALL be displayed in the lower panel
