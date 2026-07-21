## Why

The generalized compiler proved that arbitrary SpriteSeeds enter one deterministic synthesis pipeline. GSPL must now become a complete Generative Seed Programming Language with a formal grammar, deterministic lexer, typed AST, module system, imports, name resolution, type system, unit safety, bounded expressions, first-class Sprite Genes, a compiler-pass scheduler, incremental artifact compilation, provider abstraction, cross-platform support, a stable SDK, and a complete CLI.

## What Changes

### Language (new)
- GSPL 1.0 formal grammar (EBNF) with module, entity, gene, form, transformation declarations
- Deterministic lexer with full token classes and source spans
- Production parser generating typed AST nodes
- Source manager with identity and span tracking

### Module System (new)
- Module declarations, canonical paths, imports, aliases
- Import-cycle detection, duplicate detection, safe source roots
- Scoped name resolution with clear diagnostics

### Type System (new)
- Built-in types: Bool, Int, UInt, Fixed, String, Color, Vector2/3, Duration, Distance, Angle, Percentage, Ratio, etc.
- Dimensional/unit safety: reject invalid arithmetic across incompatible dimensions
- Bounded deterministic expressions with depth/node/step limits

### Sprite Genes (new)
- Typed gene contracts with stable identity, schema version, dependencies, conflicts
- Gene composition with inheritance, overrides, removal, and conflict detection
- Explicitly owned GeneRegistry (no mutable global state)

### Canonical Entity & Sprite IR (evolved)
- Complete immutable canonical entity with all resolved semantic fields
- Complete typed Sprite IR with serialization, validation, diffing, inspection CLI
- IR lowering pipeline from parsed AST through genes and constraints

### Compiler Pass Architecture (new)
- Typed passes with stable ID, version, typed input/output, dependencies
- Deterministic pass dependency graph with cycle rejection
- Topological scheduling, cancellation, failure propagation, incremental execution

### Artifact System (evolved)
- Content-addressed immutable artifact graph
- Cache integrity: atomic writes, corruption detection, stale-lock recovery
- Selective invalidation proven for palette, ability, morphology, behavior changes

### Provider Abstraction (new)
- Provider-independent synthesis interface
- Compiler core builds without ONNX Runtime
- Build profiles: GSPL_CORE_ONLY, GSPL_WITH_ONNX, GSPL_WITH_PREVIEW, GSPL_WITH_ALL_PROVIDERS
- Linux CI restored for portable core profile

### Resource Limits (completed)
- Boundary tests for all 30 ResourceLimits fields
- Below/exact/above tests for every limit
- Rejection before expensive allocation

### SDK (new)
- Stable C++ API with RAII, explicit ownership, documented thread safety
- Self-contained public headers, no global state
- API versioning policy

### CLI (new)
- Commands: parse, check, compile, migrate, explain, ir, graph, synthesize, package, verify, preview, benchmark
- JSON diagnostics, deterministic exit codes, structured output

### Tests
- Lexer/parser/module/type/expression fuzzing
- Genuine mutation testing
- Full acceptance suite for 4 reference entities
- Cross-platform CI validation

## Capabilities

### New Capabilities
- gspl-language-1.0: Grammar, lexer, parser, AST, source manager
- gspl-modules: Module system, imports, name resolution, cycle detection
- gspl-types: Type system, dimensional safety, bounded expressions
- gspl-genes: Typed gene contracts, composition, inheritance, registry
- gspl-compiler-passes: Pass graph, scheduling, incremental execution
- gspl-provider-abstraction: Provider-independent synthesis, build profiles
- gspl-sdk: Stable C++ API, self-contained headers
- gspl-cli: Complete command-line interface

### Modified Capabilities
- canonical-entity: Evolved to full immutable model
- sprite-ir: Complete IR tooling (dump, validate, diff, explain, dependencies)
- artifact-graph: Cache integrity, stal lock recovery, selective invalidation
- resource-limits-enforcement: Complete boundary coverage

## Impact

- `src/` — New files: lexer.cpp, parser.cpp, ast.cpp, source.cpp, modules.cpp, names.cpp, types.cpp, expressions.cpp, genes.cpp, constraints.cpp, passes.cpp, cache.cpp, providers.cpp, sdk.cpp, cli.cpp. Modified: all existing pipeline files to lower new AST through existing synthesis.
- `include/gspl_sprites/` — New headers for all language and compiler types
- `tests/` — New test files for every new subsystem
- `CMakeLists.txt` — Build profiles, provider selection, Linux CI support
- `.github/workflows/ci.yml` — Restored Linux matrix entry for portable core
