# ADR-0030: Make GLB target claims independently verifiable

- Status: Accepted
- Date: 2026-07-17

## Context

Khronos validation proves glTF conformance but cannot prove that GSPL feature
claims match the emitted content. A checksummed compatibility report can still
be internally false.

## Decision

Embed the normalized glb-2.0 requirements and derived compatibility report in
`asset.extras`. Add a bounded standalone verifier for container framing,
canonical GSPL metadata, report reconstruction, verified-source evidence, and
required glTF structures. Self-verify every export before returning it.

## Consequences

GLB output is both standards-valid and GSPL-contract-valid. Hash-preserving or
length-preserving edits to report claims or required structures fail closed.
The internal verifier complements rather than replaces Khronos validation.
