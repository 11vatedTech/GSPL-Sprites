# Job System Architecture 0.1

The synthesis job system owns a bounded `std::jthread` pool. Construction
requires explicit worker, retained-job, event, RAM, and per-accelerator VRAM
budgets. Deterministic scheduling mode forces one worker. Jobs have stable IDs,
integer priority, existing dependencies, bounded retries, optional start
deadlines, explicit resource requirements and accelerator affinity, and a
cooperative stop token.

Submission only permits dependencies already present, making cycles
unrepresentable. A runnable job requires every dependency to have succeeded and
available RAM/VRAM. Selection is descending priority then ascending stable ID.
Failures retry within the declared bound; terminal failure, cancellation, or
deadline expiry propagates `dependency_failed` without executing dependents.
Cancellation reaches running work through its stop token. Shutdown cancels
queued/running work, wakes workers, and joins every owned thread.

Structured events record a monotonic operational sequence, job, status,
attempt, bounded message, and declared resources. Metrics expose counts by
status plus current RAM and per-device VRAM reservations. The event buffer is bounded and intentionally
excluded from canonical identities. Provider implementations must check stop
tokens at documented safe points; native or model calls that cannot cooperate
must run behind a killable worker-process boundary.

The current deadline is a start deadline. Execution timeouts belong at provider
or sandbox-process boundaries because standard C++ cannot safely preempt an
arbitrary in-process function.
