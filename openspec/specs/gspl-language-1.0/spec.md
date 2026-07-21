# gspl-language-1.0 — GSPL 1.0 Language Core
## Purpose
Define the GSPL 1.0 language grammar, source model, lexer, parser, and AST.
## Requirements
### Requirement: Formal grammar
The GSPL 1.0 grammar SHALL be specified in EBNF and SHALL match the parser implementation.
### Requirement: Source manager
The source manager SHALL support SourceId, SourceFile, SourceBuffer, SourceLocation, SourceSpan, and deterministic identity.
### Requirement: Lexer
The lexer SHALL produce typed tokens with source spans and SHALL reject malformed input with deterministic diagnostics.
### Requirement: Parser
The parser SHALL produce a typed AST matching the grammar and SHALL recover at governed synchronization boundaries.
### Requirement: AST
The AST SHALL use explicit node types (not string maps) and SHALL preserve source spans.
