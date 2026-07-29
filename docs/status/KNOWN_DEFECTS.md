# Known Defects

Updated: 2026-07-28 (session: Sprite IR result-based migration — ir.cpp now exclusively uses fail-closed reads, DEF-0014/15/16 updated)

Severity scale: Critical / High / Medium / Low.

## Resolved defects

### DEF-0001 — Gene declarations were not compiled into canonical semantic state

- Severity: High
- Status: ✅ RESOLVED (commit `35d18d6`)
- Resolution: GeneCompositionPhase now collects AST GeneDecls, validates known kinds/dependencies, lowers typed values, and stores composed genes. CanonicalizePhase passes composed_genes to Canonicalizer::apply_genes(). Tests updated to expect gene-authoritative behavior.
- Verification: `gspl_sprites_gene_tests` (ALL PASSED), `gspl_sprites_semantic_pipeline_tests` (ALL PASSED), MSVC `/W4 /WX` clean.

### DEF-0002 — Semantic IR serializer/deserializer is incomplete

- Severity: High
- Status: ✅ RESOLVED
- Resolution: `IrSerializer::serialize()` outputs full EntityIr tree including identity, entity_id, schema_version, dependency_ids, genes, properties, and children. `IrSerializer::deserialize()` uses fail-closed `SpriteIrDeserializeResult` (no fabricated defaults). Gene array parsed with GeneRegistry lookup, preserving kind/schema/type/source/values. Graph methods added: `transitive_dependencies()`, `reverse_dependencies()`, `dependency_closure()`.
- Verification: `gspl_sprites_semantic_pipeline_tests` test 19 (ALL PASSED), `gspl_sprites_compiler_tests` (ALL PASSED).

### DEF-0003 — Canonical entity JSON deserialization is not implemented

- Severity: High
- Status: ✅ RESOLVED
- Resolution: `CanonicalEntitySerializer::from_json()` returns `CanonicalEntityDeserializeResult` (fail-closed, single-arg API). Parses all structural fields: forms, transformations, abilities (normal + storm), morphology parts, bones, sockets, clips, tracks, clip_events, animation states, transitions, collision_shapes, collision_windows, resources, and runtime. Empty/malformed/missing-stable_id input returns `value = nullopt` with diagnostics.
- Verification: `gspl_sprites_semantic_pipeline_tests` tests 16-18 (ALL PASSED).

### DEF-0004 — IR optimization pass reports success without transformation or explicit no-op contract

- Severity: Medium
- Status: ✅ RESOLVED
- Resolution: `IrOptimizePhase` implements real canonical normalization: sorts forms, transformations, abilities, storm_abilities, bones, sockets, clips, states, collision_shapes, collision_windows, resources, and genes by deterministic keys. Also normalizes `ctx.ir` gene and dependency_id ordering. Verified idempotent (double-opt produces identical hash).
- Verification: `gspl_sprites_semantic_pipeline_tests` test 20 (ALL PASSED).

### DEF-0005 — Repository status and verification evidence are source-reported, not current executable evidence

- Severity: Medium
- Status: ✅ RESOLVED
- Resolution: Live build and test verification completed. MSVC `/W4 /WX /permissive-` builds pass with zero warnings. All targeted test suites pass.

### DEF-0006 — `to_json()` outputs only counts, not full structural data

- Severity: Medium
- Status: ✅ RESOLVED
- Resolution: `CanonicalEntitySerializer::to_json()` now serializes full structural data: forms, transformations, morphology, abilities, bones, sockets, clips (with tracks and events), states, transitions, collision_shapes, collision_windows, resources, and runtime (with animation_intents). Uses `json_escape()` for all string fields. Gene data uses `gene_value_to_string()` fallback; full typed gene serialization via `gene_value_to_json()` is available in `gspl/json.hpp`.
- Verification: Round-trip test (test 16) verifies all top-level identity fields survive serialization; full structural round-trip testing deferred to next phase.

### DEF-0007 — Gene and children array deserialization is deferred

- Severity: Medium
- Status: ✅ RESOLVED
- Resolution: `IrSerializer::deserialize()` now parses the genes array from entity JSON, reconstructing `GeneInstance` objects with kind/schema/type/source/values. Uses `GeneRegistry` for descriptor lookup. Gene values stored as typed strings with `gene_value_to_string()`. `BoundedJsonReader` in `gspl/json.hpp` provides full `GeneValue` ↔ typed JSON round-trip via `gene_value_to_json()`/`json_to_gene_value()` with `GeneValueTag` variant preservation.
- Verification: `gspl_sprites_semantic_pipeline_tests` test 19 (ALL PASSED).

### DEF-0008 — `IrOptimizePhase` only normalizes `CanonicalEntity`, not `SpriteIr`

- Severity: Low
- Status: ✅ RESOLVED
- Resolution: `IrOptimizePhase` now normalizes both `ctx.canonical` (forms, transformations, abilities, bones, sockets, clips, states, collisions, resources, genes) AND `ctx.ir` (entity dependency_ids sorted, genes sorted by kind). Both representations maintain deterministic ordering.
- Verification: `gspl_sprites_semantic_pipeline_tests` test 20 verifies abilities and bones are sorted, and repeated optimization is idempotent.

### DEF-0009 — Fail-closed deserialization with no fabricated defaults

- Severity: High
- Status: ✅ RESOLVED
- Resolution: `IrSerializer::deserialize()` returns `SpriteIrDeserializeResult` with `std::optional<SpriteIr>` + `DiagnosticResult`. `CanonicalEntitySerializer::from_json()` returns `CanonicalEntityDeserializeResult`. Both reject empty/malformed/missing-required-field input with `value = nullopt` and exact diagnostics. No fabricated defaults like "deserialized", "seed", or "UNCLASSIFIED".
- Verification: `gspl_sprites_semantic_pipeline_tests` tests 16-19 verify fail-closed behavior.

### DEF-0010 — Governed JSON implementation with resource limits

- Severity: Medium
- Status: ✅ RESOLVED
- Resolution: `gspl::BoundedJsonReader` in `include/gspl/json.hpp` provides configurable limits: `max_input_bytes` (64MB), `max_nesting_depth` (64), `max_object_members` (10k), `max_array_length` (100k), `max_string_length` (64KB). Enforces limits before/during allocation. Proper escape handling including `\uXXXX` sequences. Depth tracking with `enter_object`/`leave_object`/`enter_array`/`leave_array`.
- Verification: Builds cleanly with `/W4 /WX`.

### DEF-0011 — GeneValue type preservation across serialization

- Severity: Medium
- Status: ✅ RESOLVED
- Resolution: `GeneValueTag` enum maps variant indices (string_val=0, bool_val=1, int64_val=2, uint64_val=3, double_val=4, string_list_val=5). `static_assert` verifies alignment with `GeneValue` variant size. `gene_value_to_json()` visitor handles all types; doubles use `snprintf("%.17g")` with NaN/infinity → "null". `json_to_gene_value()` reconstructs typed values; parse failures fall back to string representation. `gene_value_variant_index()` provides tag introspection.
- Verification: Framework in place; exhaustive round-trip tests for all GeneValue types deferred to next phase.

## Open defects

_All previously open defects (DEF-0006 through DEF-0008) have been resolved. See above._

### DEF-0012 — Full structural round-trip tests

- Severity: Medium
- Status: ✅ RESOLVED
- Resolution: Maximally populated CanonicalEntity round-trip test with exact field equality for all structural fields AND genes (all 6 GeneValue types). Identity hash survives round-trip. SpriteIr round-trip with genes. `from_json()` parses genes array with typed values.
- Verification: `gspl_sprites_semantic_pipeline_tests` test 21 (ALL PASSED).

### DEF-0013 — GeneValue type-tagged JSON format

- Severity: Low
- Status: ✅ RESOLVED
- Resolution: Type-tagged format `{"t":<tag>,"v":<json>}` fully wired into both `IrSerializer::deserialize()` and `CanonicalEntitySerializer::from_json()`. Exact type+value assertions. Malformed values fail closed via `json_to_gene_value_result()`. Complete input consumption enforced (trailing data rejected). Unquoted strings rejected in current schema. Legacy bare-string fallback removed from ir.cpp.
- Verification: `gspl_sprites_semantic_pipeline_tests` test 21, DEF-0015 tests.

### DEF-0014 — Resource-limit boundary enforcement

- Severity: Medium
- Status: ✅ RESOLVED
- Resolution: `BoundedJsonReader` with configurable limits, unified mixed-nesting depth via recursive `skip_value()`. Both `ir.cpp` and `semantics.cpp` deserializers exclusively use `BoundedJsonReader` with document closure (trailing data rejection). Node kind validation for all node types. `require()` fail-closed structural token enforcement. `has_more()` returns false on error for clean cascading propagation. CanonicalEntity migrated to BoundedJsonReader with typed codecs.
- Verification: `gspl_sprites_semantic_pipeline_tests` DEF-0015 tests (truncated, wrong type, missing entity). Full suite passes with `/W4 /WX`.

### DEF-0015 — Malformed-input fail-closed verification

- Severity: High
- Status: ✅ RESOLVED
- Resolution: Both `IrSerializer::deserialize()` and `CanonicalEntitySerializer::from_json()` use exclusively result-based reads. All deprecated `read_string()`/`read_int64()`/`expect()` calls replaced with `read_string_result()`/`read_int64_result()`/`require()`. Truncated JSON, trailing data, missing braces, unsupported versions, unknown node kinds, invalid gene tags, duplicate fields, trailing commas, and malformed scalars all fail closed with diagnostics. Nonfinite doubles rejected in serialization. String lists require complete closure.
- Verification: `gspl_sprites_semantic_pipeline_tests` (ALL PASSED), `gspl_sprites_gene_tests` (ALL PASSED), `gspl_sprites_compiler_tests` (ALL PASSED).

### DEF-0016 — Unified bounded parser migration

- Severity: Medium
- Status: ✅ RESOLVED
- Resolution: All production deserializers now use `BoundedJsonReader` exclusively. CanonicalEntity migrated with typed codecs (`decode_object<T>` template, all 16 codecs rewritten). Sprite IR migrated with result-based lambdas (`read_str`, `read_i64`, `require`, `parse_ir_node`). No deprecated fallback reads remain in any production parser. Duplicate field detection, trailing comma rejection, required fields, schema version enforcement, and validator invocation all in place.
- Verification: `gspl_sprites_semantic_pipeline_tests` (ALL PASSED), `gspl_sprites_gene_tests` (ALL PASSED), `gspl_sprites_compiler_tests` (ALL PASSED). MSVC `/W4 /WX` clean.

## Historical defects reverified as currently mitigated or partially mitigated

- Provider abstraction exists and `GSPL_CORE_ONLY` permits provider-disabled builds.
- Living runtime performs deterministic action selection, memory expiry, cooldowns, markers, and validation.
- Resource-limit, fuzz, mutation, cache, provider, legacy, and CLI tests exist but coverage requires expansion.
