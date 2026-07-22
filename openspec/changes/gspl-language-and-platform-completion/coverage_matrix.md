# Test Coverage Matrix

## Resource Limits (30 fields)
| Field | Below | At Limit | Above | Status |
|-------|-------|----------|-------|--------|
| max_source_size | ✓ | ✓ | ✓ | Verified |
| max_tokens | ✓ | ✓ | ✓ | Verified |
| max_forms | ✓ | ✓ | ✓ | Verified |
| max_string_length | ✓ | ✓ (~) | ✓ | Verified |
| max_gene_count | ✓ | ✓ | ✓ | Verified |
| max_seed_entropy | ✓ | n/a | ✓ | Verified |
| max_transformations | — | — | ✓ | mutation_tests |
| max_bones | — | — | ✓ | mutation_tests |
| max_sockets | — | — | ✓ | mutation_tests |
| max_animation_clips | — | — | ✓ | mutation_tests |
| max_abilities | — | — | — | Defaults only |
| max_parts | — | — | — | Defaults only |
| max_morphology | — | — | — | Defaults only |
| max_collision_shapes | — | — | — | Defaults only |
| max_collision_windows | — | — | — | Defaults only |
| max_bones_per_rig | — | — | — | Defaults only |
| max_sockets_per_rig | — | — | — | Defaults only |
| max_clips | — | — | — | Defaults only |
| max_tracks_per_clip | — | — | — | Defaults only |
| max_frames_per_clip | — | — | — | Defaults only |
| max_states | — | — | — | Defaults only |
| max_transitions | — | — | — | Defaults only |
| max_expression_depth | ✓ | ✓ | ✓ | Verified |
| max_expression_nodes | — | — | — | Defaults only |
| max_expression_steps | ✓ | ✓ | ✓ | Verified |
| max_genes | ✓ | ✓ | ✓ | Verified |
| max_gene_dependencies | — | — | — | Not yet bounded |
| zero_length_source | ✓ | n/a | n/a | Verified |
| large_seed_entropy | ✓ | n/a | ✓ | Verified |

✓ = tested  ~ = partial  — = not tested

## Module System (10 tests)
- ModulePath construction, parsing, round-trip
- ModuleResolver register + resolve, duplicate detection
- NameResolver simple entity, duplicate entity
- Full lex/parse/resolve pipeline, form extension error, source root

## Type System (9 tests)
- Primitive types, composites, equality, string rep
- Dimension compatibility, assignability, TypeRef resolve
- Type-check pipeline, serialization

## Expression + Entropy (13 tests)
- Literal eval, entropy channel, channel isolation
- Depth limit, step counting, deterministic seed
- INT64_MAX, zero

## Sprite Genes (9 tests)
- Registry lookup, built-in registration, dep validation
- Missing dep, dependency chain, custom gene, composition
- Full pipeline compilation

## UTF-8 (21 tests)
- ASCII, multibyte, emoji, combining chars, BOM
- Overlong, isolated continuation, truncated, surrogate
- >U+10FFFF, null bytes, invalid lead, sequence length
- Codepoint extraction, lexer integration, UTF-8 in comments

## Artifact Cache (9 tests)
- Disabled mode, key generation, hash_inputs
- put/get round-trip, cold miss, invalidation
- Eviction, integrity validation, read-only rejection

## Provider Abstraction (8 tests)
- NullProvider, FakeTestProvider, ProviderRegistry
- Execution, failure mode, initialization

## Legacy Compatibility (7 tests)
- Detection, parsing, migrate file, migrate directory
- Dry-run, output, non-existent, non-legacy skip

## Mutation Tests (26 scenarios)
- Determinism, state round-trip, save/restore, replay
- Cross-representation parity, package reproducibility
- Artifact graph, selective invalidation
- Form binding, transition binding, corrupt progress
- Frame distinctness, collision window, resource cost
- Identity, save data corruption, stale artifact
- Package checksum, rights validation, resource limits
- Frame hash, pixel AABB, RGBA corruption, preview toggle
- Headless output schema

## Fuzz Parsing (5-second time-bounded)
- Random byte sequences, round-trip canonicalization

## CLI Tests (11 scenarios)
- Help, version, input file, multiple flags
- Output dir, stop-after, unknown option, migrate flags
- Graph flag, migrate dry-run, version/help output

## Semantic Pipeline (11 tests)
- Full pipeline, serializer/validator/identity/diff
- SpriteIrLowering, SpriteSeedLowering
- PassManager topology, diagnostic reporting

## Header Self-Containment (8 tests)
- GsplContext lifecycle, SourceBuffer SDK compile
- ExpressionConfig defaults, ProviderRegistry basics
- UTF-8 validation, CacheConfig defaults
- Legacy detection, MigrateOptions defaults
