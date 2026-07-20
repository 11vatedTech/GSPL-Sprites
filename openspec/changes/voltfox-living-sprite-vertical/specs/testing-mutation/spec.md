# Property and Mutation Testing

## Requirements
1. Deterministic canonicalization: canonicalize(parse(seed)) identical for 100 random seeds
2. State identity round-trip: serialize(deserialize(state)) produces identical bytes
3. Save/restore: full state save, restore, compare identity hashes
4. Replay: record events, replay, compare final identity
5. Cross-representation parity: same state → same behavior across reps
6. Package reproducibility: two builds from same inputs → identical hash
7. Artifact-graph closure: every artifact reachable from root
8. Selective invalidation: single-source change only invalidates dependents

## Mutation Tests (each must be killed)
- Remove form binding → test fails
- Remove transition binding → test fails
- Corrupt transformation progress → test fails
- Accept unresolved plane asset → test fails
- Make animation frames identical → test fails
- Disable collision window → test fails
- Skip ability resource cost → test fails
- Omit state identity → test fails
- Mutate save data → test fails
- Reuse stale artifact → test fails
- Omit package checksum → test fails
- Bypass rights validation → test fails

## Scenarios
1. All mutation tests produce failure when mutant applied
2. Property test runs 100 iterations without failure
