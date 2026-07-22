## Context

GSPL Sprites has proven a working generalized compiler for the .sprite format, supporting deterministic 2D/2.5D/3D synthesis, resource limits, frame identity, and headless evidence. The next phase transforms the ad-hoc parser into a complete programming language with formal grammar, typed AST, module system, type system, first-class Sprite Genes, compiler-pass architecture, incremental compilation, provider abstraction, a stable SDK, and a complete CLI.

## Goals / Non-Goals

**Goals:**
- GSPL 1.0 formal grammar matching a production parser
- Deterministic lexer, typed AST, source spans
- Module system with imports, aliases, cycle rejection
- Scoped name resolution with stable diagnostics
- Type system with dimensional safety (ticks vs seconds, pixels vs world units)
- Bounded deterministic expressions
- First-class typed Sprite Genes with composition and inheritance
- Complete immutable canonical entity with serialization
- Complete typed Sprite IR with diffing/inspection CLI
- Compiler-pass graph with deterministic scheduling and incremental execution
- Content-addressed artifact cache with integrity and selective invalidation
- Provider abstraction decoupling core from Windows-only ONNX Runtime
- Portable Linux compiler-core CI
- Stable C++ SDK with self-contained headers
- Complete CLI (parse, check, compile, migrate, ir, graph, synthesize, package, verify, benchmark)
- Complete resource-boundary test coverage
- All existing Voltfox and generic-sprite tests continue to pass

**Non-Goals:**
- GPU acceleration
- Runtime loading optimizations
- Network-distributed compilation
- WebAssembly target
- Full IDE integration

## Decisions

- **EBNF grammar before implementation**: The formal grammar guides the parser; the parser must match it.
- **Typed AST with explicit node types**: No unrestricted string maps for AST representation.
- **Module identity from content hash**: Stable deterministic module identification.
- **Dimensional safety via wrapper types**: Distinct C++ types for Duration, Distance, Angle, etc. with compile-time operator checking.
- **Gene contracts as typed structs**: Not strings; each gene family gets a typed C++ struct with validation.
- **Pass graph as explicit DAG**: Each pass declares typed inputs/outputs; scheduling uses topological sort.
- **Provider abstraction via interface**: Synthesis and inference providers implement a pure virtual interface; core code never imports ONNX headers.
- **SDK as header-only public layer**: Public API headers in include/gspl/ with no internal implementation leakage.
- **CLI uses subcommand pattern**: One binary with subcommands, shared compiler context.

## Risks / Trade-offs

- [Scope] Full GSPL language is ~80% of the work; synthesis, runtime, and packaging are ~20%. The existing proven synthesis path is preserved.
- [Complexity] A typed AST and pass graph introduce compile-time overhead; mitigated by incremental compilation.
- [ONNX coupling] Decoupling providers is essential for Linux CI but adds abstraction surface.
- [Backward compatibility] Legacy .sprite format must continue working; implemented as a compatibility front-end that lowers into the same canonical entity model.
