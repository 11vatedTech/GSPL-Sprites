# Known Defects

Updated: 2026-07-27

Severity scale: Critical / High / Medium / Low.

## Open defects

### DEF-0001 — Gene declarations were not compiled into canonical semantic state

- Severity: High
- Status: Repair in progress
- Evidence:
  - `src/passes.cpp` had `GeneCompositionPhase::execute()` return success without reading AST genes.
  - `CanonicalizePhase` invoked `canonicalizer.lower(*ctx.ast, {})`, discarding all gene declarations.
  - `tests/semantic_pipeline_tests.cpp` expected `stable_id` and classification to fall back to entity/default values and commented that `GeneCompositionPhase` was a stub.
- Impact: Identity, classification, appearance, provenance, and other gene semantics could be silently ignored while the pass pipeline reported success.
- Repair started: `CompilationContext` now stores `composed_genes`; `GeneCompositionPhase` collects AST `GeneDecl`s, validates built-in gene kinds/dependencies, and `CanonicalizePhase` lowers with composed genes. Tests are being updated to require gene payload preservation.
- Verification needed: rebuild and run `gspl_sprites_gene_tests`, `gspl_sprites_semantic_pipeline_tests`, and full relevant CTest suite.

### DEF-0002 — Semantic IR serializer/deserializer is incomplete

- Severity: High
- Status: Open
- Evidence:
  - `src/ir.cpp::IrSerializer::serialize()` emits only version/entity/seed/schema metadata.
  - `src/ir.cpp::IrSerializer::deserialize()` ignores input and returns hardcoded placeholder identities.
  - `src/ir.cpp::IrSerializer::dependencies()` always returns an empty vector.
- Impact: SIR is not a complete canonical, versioned, round-trippable, dependency-traceable IR as required by the master directive.
- Required repair: implement canonical JSON escaping/parsing or a bounded internal parser for the IR schema, include nodes/plans/dependencies, validate malformed input, and add negative/golden tests.

### DEF-0003 — Canonical entity JSON deserialization is not implemented

- Severity: High
- Status: Open
- Evidence: `src/semantics.cpp::CanonicalEntitySerializer::from_json()` returns `std::nullopt` for all inputs.
- Impact: Canonical semantic state is not fully serializable/forward-migratable/cacheable; SDK and cache contracts remain incomplete.
- Required repair: implement bounded schema parser, deterministic round-trip tests, malformed-input tests, and version handling.

### DEF-0004 — IR optimization pass reports success without transformation or explicit no-op contract

- Severity: Medium
- Status: Open
- Evidence: `src/passes.cpp::IrOptimizePhase::execute()` casts context to void and returns success.
- Impact: A formal compiler stage can be mistaken as implemented optimization. If optimization is intentionally identity-preserving, it must record an explicit pass artifact/invariant rather than being a silent no-op.
- Required repair: either implement deterministic canonical sorting/deduplication or rename/mark as explicit identity validation with tests.

### DEF-0005 — Repository status and verification evidence are source-reported, not current executable evidence

- Severity: Medium
- Status: Open
- Evidence: `AGENTS.md` claims 83/83 tests pass and CI status, but live git/build/test commands have not yet run in this session.
- Impact: Completion could be overstated if current local branch differs from recorded state.
- Required repair: run git baseline, configure/build/test, capture totals/failures/skips/warnings, and update status docs.

## Historical defects reverified as currently mitigated or partially mitigated

- Provider abstraction exists and `GSPL_CORE_ONLY` permits provider-disabled builds, but ONNX Runtime remains configured in the default Windows path and must stay optional.
- Living runtime is not merely a stub: `step_living_runtime()` performs deterministic action selection, memory expiry, cooldowns, markers, and validation. However broader cognition/emotion/relationship/growth requirements remain incomplete relative to the master directive.
- Resource-limit, fuzz, mutation, cache, provider, legacy, and CLI tests exist, but quality and coverage require executable verification and adversarial review.
