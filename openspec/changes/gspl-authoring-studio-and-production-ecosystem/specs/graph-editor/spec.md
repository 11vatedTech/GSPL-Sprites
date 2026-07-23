## ADDED Requirements

### Requirement: Node-based schematic canvas
The graph editor SHALL render a pannable, zoomable canvas displaying nodes with typed input/output ports and directed edges between them.

#### Scenario: Create a node
- **WHEN** the user right-clicks a blank area of the canvas and selects "Add Node > Gene"
- **THEN** a new Gene node SHALL appear at the cursor position with default input and output ports

#### Scenario: Pan the canvas
- **WHEN** the user holds the middle mouse button and drags
- **THEN** the canvas viewport SHALL scroll to follow the drag

### Requirement: Drag-and-drop node placement
The graph editor SHALL support dragging nodes from a palette tool window onto the canvas and repositioning existing nodes.

#### Scenario: Drag from palette
- **WHEN** the user drags a "Morphology" gene from the gene palette onto the canvas
- **THEN** a node representing that gene SHALL be instantiated at the drop location

#### Scenario: Reposition a node
- **WHEN** the user drags a node by its title bar to a new position
- **THEN** the node SHALL snap to the grid and all connected edges SHALL reroute to follow

### Requirement: Edge routing
The graph editor SHALL automatically route directed edges between connected ports, avoiding overlaps with nodes and other edges.

#### Scenario: Auto-route on connection
- **WHEN** the user draws an edge from an output port to an input port
- **THEN** the edge SHALL render as a Bezier curve that routes around intervening nodes

#### Scenario: Edge reroute on node move
- **WHEN** the user moves a node that has connected edges
- **THEN** all incident edges SHALL reroute within 100 ms

### Requirement: Input/output port mapping
The graph editor SHALL allow connecting compatible ports and SHALL validate type compatibility at connection time.

#### Scenario: Connect compatible ports
- **WHEN** the user drags from an output port of type `Sprite` to an input port of type `Sprite`
- **THEN** a valid edge SHALL be created and SHALL render in the default connection color

#### Scenario: Reject incompatible connection
- **WHEN** the user attempts to connect an output port of type `Int` to an input port of type `Sprite`
- **THEN** the connection SHALL be rejected and a type-mismatch tooltip SHALL be displayed

### Requirement: Subgraph grouping
The graph editor SHALL support grouping multiple nodes into a subgraph with its own boundary, input/output proxy ports, and collapsible body.

#### Scenario: Create subgraph from selection
- **WHEN** the user selects three nodes, right-clicks, and selects "Group into Subgraph"
- **THEN** a subgraph node SHALL replace the selection with proxy ports for all exposed connections

#### Scenario: Expand subgraph
- **WHEN** the user double-clicks a subgraph node
- **THEN** the canvas SHALL zoom to show the subgraph's internal nodes

### Requirement: Minimap
The graph editor SHALL provide a minimap overlay showing the full graph extent with a viewport rectangle indicator.

#### Scenario: Minimap navigation
- **WHEN** the user drags the viewport rectangle in the minimap
- **THEN** the main canvas SHALL pan to the corresponding position

#### Scenario: Minimap updates on graph change
- **WHEN** a new node is added outside the current viewport
- **THEN** the minimap SHALL update to include the new node's extent

### Requirement: Zoom-to-fit
The graph editor SHALL provide a zoom-to-fit command that scales and pans the canvas to display all nodes.

#### Scenario: Zoom to fit all nodes
- **WHEN** the user presses Ctrl+Shift+F
- **THEN** the canvas SHALL zoom and pan so that all nodes are visible within the viewport with 10% padding

#### Scenario: Zoom to fit selection
- **WHEN** the user selects a subset of nodes and presses Ctrl+Shift+F
- **THEN** the canvas SHALL zoom and pan to show only the selected nodes

### Requirement: Auto-layout
The graph editor SHALL provide auto-layout algorithms (hierarchical, force-directed, orthogonal) to arrange nodes automatically.

#### Scenario: Apply hierarchical layout
- **WHEN** the user selects Layout > Hierarchical
- **THEN** all nodes SHALL be arranged in top-to-bottom tiers based on their depth in the dependency graph

#### Scenario: Preserve layout after manual edit
- **WHEN** the user manually moves one node after auto-layout
- **THEN** other nodes SHALL retain their auto-layout positions

### Requirement: Node property panel
The graph editor SHALL display a property panel for the selected node, showing all configurable fields with type-appropriate editors.

#### Scenario: Select node shows properties
- **WHEN** the user clicks on a node
- **THEN** the property panel SHALL display all editable fields (name, type, constraints) with current values

#### Scenario: Edit property updates node
- **WHEN** the user changes the node's name in the property panel and presses Enter
- **THEN** the node title SHALL update immediately on the canvas
