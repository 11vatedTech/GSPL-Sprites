# Completion Plan

Updated: 2026-07-27

This plan is dependency-aware and must be updated from executable evidence, not README claims.

## Cycle 1 — Truth and stabilization

1. Re-run git baseline commands and record exact branch, SHA, status, and recent history.
2. Configure, build, and test the current branch using the least-provider path first (`GSPL_CORE_ONLY=ON` where supported), then Windows/default and Qt Studio profiles as tools permit.
3. Update `CURRENT_REPOSITORY_STATE.md` with command outputs, test totals, failures, skips, warnings, and CI status if remotely verified.
4. Close foundational false-success gaps that block meaningful verification:
   - gene composition and canonical lowering;
   - canonical/SIR serialization round trips;
   - pass stages that silently no-op.

## Cycle 2 — Canon and traceability

1. Create `docs/canon/CANON_INDEX.md`, `docs/canon/CANON_CONFLICTS.md`, and `docs/canon/CANON_TO_IMPLEMENTATION_TRACEABILITY.md`.
2. Treat local GSPL-Sprites docs/ADRs/OpenSpec archives as product canon.
3. Treat adjacent GSPL/Paradigm reference repos as read-only evidence unless explicit product records adopt a requirement.
4. Map master directive requirements and the historical 37-item checklist to source, tests, status, and discrepancies.

## Cycle 3 — Language, genes, semantic IR

1. Finish gene composition and canonical payload preservation.
2. Implement canonical entity JSON deserialization and stable SHA-256 identity.
3. Implement complete Sprite IR serialization/deserialization/dependency inspection/diff/explain.
4. Add golden, negative, and round-trip tests.
5. Reconcile UTF-8/module/type/expression/gene checklist evidence with current tests.

## Cycle 4 — Compiler, runtime, projection

1. Ensure every compiler pass has typed input/output, invariants, diagnostics, failure behavior, and tests.
2. Complete gaps in runtime organism semantics: goals, perception, memory, emotion/relationships/growth as product-scoped requirements define them.
3. Preserve construct-vs-organism distinction in canonical IR and runtime/projection paths.
4. Ensure projection contract/plan/artifact/verification evidence remain separate.

## Cycle 5 — Infrastructure and product surface

1. Complete cache corruption/concurrency/invalidation/eviction/read-only/disabled mode evidence.
2. Verify provider abstraction, provider-disabled build, fake deterministic provider, and no provider-specific types in core public APIs.
3. Verify `.sprite` migration semantics through canonical compiler path.
4. Stabilize SDK and CLI command contracts, exit codes, JSON diagnostics, resource limits, and package verification.
5. Finish Qt Studio gate only after core truth is stable.

## Cycle 6 — Release proof

1. Run full build/test/lint/format/static-analysis/fuzz/mutation/sanitizer gates where available.
2. Produce deterministic end-to-end workflow evidence for one organism and one construct.
3. Produce package/archive/checksum/install evidence.
4. Update release readiness and traceability matrices.
5. Commit logical milestones with truthful messages after verification.

## Current next implementation action

Finish and verify the gene-composition repair because semantic identity is a prerequisite for compiler, IR, cache, runtime, and projection correctness.
