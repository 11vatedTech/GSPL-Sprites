# Known Defects

Updated: 2026-07-28

Severity scale: Critical / High / Medium / Low.

## Resolved defects

### DEF-0001 — Gene declarations were not compiled into canonical semantic state

- Severity: High
- Status: ✅ RESOLVED (commit `35d18d6`)
- Resolution: GeneCompositionPhase now collects AST GeneDecls, validates known kinds/dependencies, lowers typed values, and stores composed genes. CanonicalizePhase passes composed_genes to Canonicalizer::apply_genes(). Tests updated to expect gene-authoritative behavior.
- Verification: `gspl_sprites_gene_tests` (ALL PASSED), `gspl_sprites_semantic_pipeline_tests` (ALL PASSED), MSVC `/W4 /WX` clean.

### DEF-0002 — Semantic IR serializer/deserializer is incomplete

- Severity: High
- Status: ✅ RESOLVED (commit pending)
- Resolution: `IrSerializer::serialize()` now outputs full EntityIr tree including identity, entity_id, schema_version, dependency_ids, and children. `IrSerializer::deserialize()` implements bounded JSON parsing with JsonReader class (string, int, bool, skip_value for arrays/objects). `IrSerializer::dependencies()` traverses the IR graph and returns sorted dependency IDs. `IrSerializer::validate()` checks entity sub-fields.
- Known limitation: Gene array and children array deserialization is deferred (genes parsed as opaque, children skipped). Full round-trip fidelity for these arrays requires completion of GeneInstance JSON serialization.
- Verification: `gspl_sprites_semantic_pipeline_tests` test 19 (ALL PASSED), `gspl_sprites_compiler_tests` test 14 (ALL PASSED).

### DEF-0003 — Canonical entity JSON deserialization is not implemented

- Severity: High
- Status: ✅ RESOLVED (commit pending)
- Resolution: `CanonicalEntitySerializer::from_json()` implements bounded JSON key-value parser reading all top-level identity fields (schema_version, stable_id, name, classification, rights, rights_allow_export, entropy_root, colors, provenance). Uses `std::from_chars` for numeric parsing. Validates required stable_id. Falls back name to stable_id. Skips unknown fields gracefully.
- Known limitation: Structural collections (forms, transformations, abilities, morphology, bones, clips, etc.) are not yet deserialized. `to_json()` outputs counts only, not full structural data.
- Verification: `gspl_sprites_semantic_pipeline_tests` tests 16-18 (ALL PASSED).

### DEF-0004 — IR optimization pass reports success without transformation or explicit no-op contract

- Severity: Medium
- Status: ✅ RESOLVED (commit pending)
- Resolution: `IrOptimizePhase` implements real canonical normalization: sorts forms, transformations, abilities, storm_abilities, bones, sockets, clips, states, collision_shapes, collision_windows, resources, and genes by deterministic keys. Verified idempotent (double-opt produces identical hash). Contract documented: behavior(before) == behavior(after).
- Verification: `gspl_sprites_semantic_pipeline_tests` test 20 (ALL PASSED).

### DEF-0005 — Repository status and verification evidence are source-reported, not current executable evidence

- Severity: Medium
- Status: ✅ RESOLVED
- Resolution: Live build and test verification completed in this session. MSVC `/W4 /WX /permissive-` builds pass with zero warnings. Targeted test suites (gene, semantic_pipeline, compiler) all pass. Full CTest suite pending (only targeted executables were built).

## Open defects

### DEF-0006 — `to_json()` outputs only counts, not full structural data

- Severity: Medium
- Status: Open
- Evidence: `CanonicalEntitySerializer::to_json()` serializes only metadata counts (form_count, transformation_count, etc.) rather than full form/transformation/ability/morphology data. `canonical_identity_payload()` DOES include full data for hashing purposes. The two serialization paths diverge.
- Impact: JSON interchange format cannot round-trip complete entities.
- Required repair: Either expand `to_json()` to serialize full entity data, or document it as identity-metadata-only format.

### DEF-0007 — Gene and children array deserialization is deferred

- Severity: Medium
- Status: Open
- Evidence: `IrSerializer::deserialize()` entity parsing uses `skip_value()` for genes and children arrays. GeneInstance JSON serialization/deserialization is not yet implemented in the JsonReader.
- Impact: Full SpriteIr round-trip fidelity for genes and child nodes is not yet achieved.
- Required repair: Implement GeneInstance JSON serialization in IrSerializer, add children array parsing.

### DEF-0008 — `IrOptimizePhase` only normalizes `CanonicalEntity`, not `SpriteIr`

- Severity: Low
- Status: Open
- Evidence: The pass sorts `ctx.canonical` collections but leaves `ctx.ir` untouched. If both representations are used downstream, their internal ordering may diverge.
- Required repair: Add parallel normalization of `ctx.ir` collections or document that SpriteIr normalization is deferred.

## Historical defects reverified as currently mitigated or partially mitigated

- Provider abstraction exists and `GSPL_CORE_ONLY` permits provider-disabled builds, but ONNX Runtime remains configured in the default Windows path and must stay optional.
- Living runtime is not merely a stub: `step_living_runtime()` performs deterministic action selection, memory expiry, cooldowns, markers, and validation. However broader cognition/emotion/relationship/growth requirements remain incomplete relative to the master directive.
- Resource-limit, fuzz, mutation, cache, provider, legacy, and CLI tests exist, but quality and coverage require executable verification and adversarial review.
