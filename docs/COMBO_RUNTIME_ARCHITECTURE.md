# Combo runtime architecture

Combo programs form bounded directed acyclic graphs over registered typed combat
abilities. Explicit entry abilities begin a chain. Every edge defines an
inclusive integer-tick cancel window and may require confirmation that the
previous ability hit. Duplicate edges, absent abilities, self loops, malformed
windows, unreachable transitions, and any reachable cycle fail validation.

Execution admits an entry or transition before invoking combat authority on
candidate state. Combo history and combat effects commit together only when the
target, range, resources, cooldown, timing, confirmation, history bound, and all
combat effects succeed. A rejected branch cannot damage a target, spend a
resource, start a cooldown, or alter combo history.

Runtime history is itself validated against graph reachability, chronological
order, every cancel window, and prior hit confirmation. It therefore cannot be
forged into a state that the live executor could not produce. Chains end through
an explicit reset; higher-level behavior decides when recovery or interruption
ends a combo.

The graph is deliberately acyclic to reject unbounded authored chains. Repeated
gameplay sequences remain possible by ending one combo and starting another.
Input buffering, defense/counter rules, collision-derived hit confirmation, and
animation recovery timing remain distinct layers.
