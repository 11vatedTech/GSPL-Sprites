# gspl-language-1.0 - GSPL 1.0 Language Core
## Purpose
Define the GSPL 1.0 language grammar, source model, lexer, parser, and AST.
## ADDED Requirements
### Requirement: Formal grammar
The GSPL 1.0 grammar SHALL be specified in EBNF and SHALL match the parser implementation.
#### Scenario: Grammar is defined
Given the grammar.md file, When parsed, Then it SHALL contain valid EBNF productions.
### Requirement: Source manager
The source manager SHALL support SourceId, SourceFile, SourceBuffer, SourceLocation, SourceSpan, and deterministic identity.
#### Scenario: Source buffer loads content
Given a SourceBuffer, When created from a string, Then the content SHALL be accessible and identity SHALL be deterministic.
### Requirement: Lexer
The lexer SHALL produce typed tokens with source spans and SHALL reject malformed input with deterministic diagnostics.
#### Scenario: Valid input produces tokens
Given valid GSPL source, When lexed, Then typed tokens SHALL be produced with source spans.
#### Scenario: Malformed input produces error
Given invalid GSPL source, When lexed, Then a diagnostic SHALL be produced and the error SHALL be deterministic.
### Requirement: Parser
The parser SHALL produce a typed AST matching the grammar and SHALL recover at governed synchronization boundaries.
#### Scenario: Valid source produces AST
Given valid GSPL source, When parsed, Then a typed AST SHALL be produced matching the grammar.
### Requirement: AST
The AST SHALL use explicit node types (not string maps) and SHALL preserve source spans.
#### Scenario: AST nodes have types
Given a parsed ModuleDecl, When inspected, Then its node types SHALL be explicit and spans SHALL be preserved.
