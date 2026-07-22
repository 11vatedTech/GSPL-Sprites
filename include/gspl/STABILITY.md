# GSPL SDK API Stability Policy

**Version**: 1.0.0  
**Last Updated**: 2026-07-21

## Scope

This policy applies to the public C++ API surface in `include/gspl/*.hpp` (the "SDK").  
Internal implementation headers in `include/gspl_sprites/` and all `src/` files are **not** covered.

## Stability Guarantees

1. **Major version (X.0.0)**: Breaking changes permitted. Public APIs may be removed or
   renamed. Backward compatibility is not guaranteed.

2. **Minor version (0.X.0)**: Additive changes only. New public APIs may be introduced.
   Existing public APIs will not be removed or have their signatures changed in a
   backward-incompatible way. Deprecation warnings may precede removal in a future
   major version.

3. **Patch version (0.0.X)**: Bug fixes only. No API changes. No behavioral changes
   except to correct observed defects.

## Compatibility Commitments

- All `#pragma once` headers in `include/gspl/` are part of the stable SDK.
- A compilation unit that includes any `include/gspl/*.hpp` header and compiles
  successfully under version X.Y.Z will compile successfully under X.Y.(Z+1) and
  X.(Y+1).0 (with the possible addition of newly required includes for new features).
- Behavioral compatibility: documented preconditions and postconditions of public
  functions will not be narrowed in patch or minor releases.
- ABI compatibility is not guaranteed across any version change.

## Deprecation Process

1. An API scheduled for removal is marked `[[deprecated]]` with a message indicating
   the planned removal version.
2. The deprecated API remains available for at least one minor version cycle.
3. Removal occurs in the next major version.

## Stability-Tested Components

- `GsplContext` RAII context
- `SourceBuffer`, `SourceManager`
- `Token`, `Lexer`
- `ModulePath`, `ModuleResolver`, `NameResolver`
- `TypeSystem`, `TypeRef`, `Type`
- `ExpressionConfig`, `ExpressionEvaluator`
- `GeneRegistry`, `GeneDescriptor`, `GeneInstance`
- `PassManager`, `CompilerPass`, `CompilationContext`
- `CanonicalEntity`, `Canonicalizer`
- `SpriteIr`, `IrSerializer`
- `ArtifactCache`, `CacheConfig`
- `Provider`, `ProviderRegistry`
- `LegacySpriteParser`, `migrate_file`, `migrate_directory`
- `Cli`, `CliOptions`, `Cli::parse`, `Cli::run`
- `DiagnosticResult`, `Diagnostic`

## Versioning

SDK version follows `include/gspl/sdk.hpp` `GsplContext::version()`.
