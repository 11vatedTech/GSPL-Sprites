# fuzz-parsing

## Purpose

Fuzz-test the seed parser with random inputs to ensure crash-safety, determinism, and round-trip structural equality.

## Requirements

### Requirement: Fuzz harness SHALL test seed parsing with random mutations
A fuzzing test target SHALL generate random byte sequences, feed them to `parse_seed()`, and verify that:
- The parser does not crash (no segmentation faults, no unhandled exceptions)
- The parser does not hang (each parse completes within a configurable timeout, default 1 second)
- If parsing succeeds, the resulting SpriteSeed SHALL round-trip through `canonicalize()` and re-parsing

#### Scenario: Random bytes do not crash parser
- **WHEN** feeding 10000 random byte sequences (length 1..256) to `parse_seed()`
- **THEN** no crash occurs

### Requirement: Fuzz harness SHALL test round-trip equality
For any seed that successfully parses, `canonicalize()` followed by writing to string and re-parsing SHALL produce an equivalent canonical seed. The test SHALL verify structural equality (same identity, same form count, same morphology parts).

#### Scenario: Round-trip preserves structure
- **WHEN** a random valid seed round-trips through parse → canonicalize → write → parse
- **THEN** the two parsed SpriteSeed objects have equal form counts and morphology parts

### Requirement: Fuzz harness SHALL be time-boxed
The fuzz test SHALL complete within a configurable maximum wall-clock time (default 30 seconds for CI, 5 minutes for local runs). It SHALL report the number of iterations completed within the time box.

#### Scenario: Fuzz run reports iterations
- **WHEN** running the fuzz test for 10 seconds
- **THEN** the test outputs the number of iterations attempted and completed

### Requirement: Fuzz harness SHALL use a deterministic seed
The fuzz random generator SHALL be seeded with a fixed value (42) by default for reproducible CI runs, with an option to pass a different seed via environment variable `GSPL_FUZZ_SEED`.

#### Scenario: Deterministic fuzz produces same sequence
- **WHEN** running fuzz with seed 42 twice
- **THEN** both runs produce the same sequence of mutation inputs
