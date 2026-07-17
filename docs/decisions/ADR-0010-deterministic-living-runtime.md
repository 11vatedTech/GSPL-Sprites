# ADR-0010: Deterministic engine-neutral living runtime

## Status

Accepted.

## Decision

Use bounded fixed-tick runtime state with ordered perception memory, an integer
blackboard, goal activation rules, deterministic utility selection, and explicit
action/event lifecycles. Keep the policy independent from render and engine
targets. Admit additional behavior-tree, GOAP, rule, and learned-policy adapters
through the same action and event contracts rather than making one architecture
universal.

## Consequences

Runtime decisions are reproducible, testable, and portable. Perception and
memory have explicit confidence and expiry instead of hidden global knowledge.
The first policy is operational but does not claim that every required behavior
architecture, persistence feature, or replication layer is complete.
