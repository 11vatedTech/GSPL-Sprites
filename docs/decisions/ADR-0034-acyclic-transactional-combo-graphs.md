# ADR-0034: Acyclic transactional combo graphs

## Status

Accepted.

## Decision

Compile combos as bounded acyclic graphs referencing registered combat
abilities. Put cancel timing and hit-confirm requirements on transitions and
commit combo history with combat state as one transaction.

## Rationale

Typed edges make branching, reachability, timing, and infinite-chain analysis
possible before runtime. Atomic execution prevents a failed combat admission
from advancing the combo or a failed combo admission from applying an ability.
Revalidating history makes restored or externally supplied state fail closed.

## Consequences

Finite branching chains and cancel windows are deterministic and portable.
Intentional repetition requires ending and restarting a combo rather than a
cycle. Input buffering, counters, and animation synchronization require
additional explicit contracts instead of weakening graph validation.
