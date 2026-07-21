## 1. Governance Correction
- [x] 1.1 Fix AGENTS.md HEAD reference, task status, CI status, next steps
- [x] 1.2 Fix release-closure-report.md HEAD, commit history, task status
- [x] 1.3 Commit and push governance correction

## 2. GSPL Language Core (Headers)
- [ ] 2.1 Define GSPL 1.0 EBNF grammar in grammar.md
- [ ] 2.2 Implement SourceId, SourceFile, SourceBuffer, SourceLocation, SourceSpan types
- [ ] 2.3 Implement SourceManager with identity and span tracking
- [ ] 2.4 Implement Token type with all token classes
- [ ] 2.5 Implement deterministic Lexer
- [ ] 2.6 Implement AST node types (ModuleDecl, EntityDecl, GeneDecl, etc.)
- [ ] 2.7 Implement Parser
- [ ] 2.8 Implement UTF-8 handling and invalid-UTF-8 policy
- [ ] 2.9 Implement source loading (in-memory + filesystem via governed roots)

## 3. Module System
- [ ] 3.1 Implement ModulePath, ModuleIdentity, ModuleDeclaration
- [ ] 3.2 Implement import resolution and cycle detection
- [ ] 3.3 Implement name resolution with scopes
- [ ] 3.4 Implement safe source roots and path traversal rejection
- [ ] 3.5 Add module tests (imports, cycles, duplicates, roots)

## 4. Type System
- [ ] 4.1 Define built-in type representations (Bool, Int, UInt, Fixed, String, etc.)
- [ ] 4.2 Implement dimensional wrapper types (Duration, Distance, Angle, etc.)
- [ ] 4.3 Implement type checking for built-in and dimensional types
- [ ] 4.4 Reject invalid cross-dimension arithmetic
- [ ] 4.5 Add type system tests

## 5. Bounded Expressions
- [ ] 5.1 Implement expression node types
- [ ] 5.2 Implement deterministic expression evaluator
- [ ] 5.3 Implement entropy channels and isolation
- [ ] 5.4 Enforce expression depth/node/step limits
- [ ] 5.5 Add expression tests and entropy isolation proof

## 6. Sprite Genes
- [ ] 6.1 Implement typed GeneDescriptor, GenePayload, GeneKind (35 families)
- [ ] 6.2 Implement GeneRegistry with owned registration
- [ ] 6.3 Implement gene composition with inheritance and overrides
- [ ] 6.4 Implement conflict detection
- [ ] 6.5 Implement canonical serialization
- [ ] 6.6 Add gene tests

## 7. Canonical Entity & Sprite IR
- [ ] 7.1 Implement complete canonical entity with all resolved fields
- [ ] 7.2 Implement canonical serialization and identity
- [ ] 7.3 Implement complete typed Sprite IR with all node families
- [ ] 7.4 Implement IR serialization and deserialization
- [ ] 7.5 Implement IR validation, diffing, inspection
- [ ] 7.6 Implement IR CLI (dump, validate, diff, explain, dependencies)
- [ ] 7.7 Add IR tests

## 8. Compiler Pass Architecture
- [ ] 8.1 Define typed pass interface (pass ID, version, input/output, dependencies)
- [ ] 8.2 Implement pass dependency graph with cycle detection
- [ ] 8.3 Implement topological scheduler with cancellation
- [ ] 8.4 Implement failure propagation
- [ ] 8.5 Implement incremental pass execution
- [ ] 8.6 Add pass system tests

## 9. Artifact Cache
- [ ] 9.1 Implement content-addressed artifact storage
- [ ] 9.2 Implement atomic writes and lock recovery
- [ ] 9.3 Implement corruption detection
- [ ] 9.4 Implement selective invalidation
- [ ] 9.5 Add cache tests

## 10. Provider Abstraction
- [ ] 10.1 Define provider interface
- [ ] 10.2 Decouple compiler core from ONNX Runtime
- [ ] 10.3 Implement build profiles (CORE_ONLY, WITH_ONNX, WITH_PREVIEW)
- [ ] 10.4 Restore Linux CI for portable core profile
- [ ] 10.5 Add provider tests

## 11. Legacy Compatibility
- [ ] 11.1 Implement legacy .sprite format as compatibility front-end
- [ ] 11.2 Implement gspl migrate command
- [ ] 11.3 Add round-trip and equivalence tests

## 12. SDK Stabilization
- [ ] 12.1 Define public API surface
- [ ] 12.2 Implement RAII contexts and explicit ownership
- [ ] 12.3 Add header self-containment tests
- [ ] 12.4 Document API stability policy

## 13. CLI Completion
- [ ] 13.1 Implement parse command
- [ ] 13.2 Implement check command
- [ ] 13.3 Implement compile command
- [ ] 13.4 Implement migrate command
- [ ] 13.5 Implement ir command with subcommands
- [ ] 13.6 Implement graph command
- [ ] 13.7 Implement package and verify commands
- [ ] 13.8 Implement preview command
- [ ] 13.9 Add CLI tests

## 14. Resource Limits Completion
- [ ] 14.1 Add boundary tests for all 30 ResourceLimits fields
- [ ] 14.2 Add below/exact/above tests
- [ ] 14.3 Produce coverage matrix

## 15. Diagnostics System
- [ ] 15.1 Implement stable diagnostic codes
- [ ] 15.2 Implement JSON diagnostic output
- [ ] 15.3 Ensure deterministic ordering

## 16. Testing
- [ ] 16.1 Lexer fuzzing
- [ ] 16.2 Parser fuzzing
- [ ] 16.3 Package fuzzing
- [ ] 16.4 Genuine mutation testing
- [ ] 16.5 Reference entity acceptance tests (modern + legacy)
- [ ] 16.6 Existing test non-regression verification

## 17. Final Gates
- [ ] 17.1 All 80 completion gates verified
- [ ] 17.2 OpenSpec validation passes
- [ ] 17.3 CI green for all profiles
- [ ] 17.4 HEAD == origin/main, working tree clean
- [ ] 17.5 Archive change
- [ ] 17.6 Final report produced
