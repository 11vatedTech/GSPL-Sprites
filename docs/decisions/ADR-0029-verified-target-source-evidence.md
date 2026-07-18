# ADR-0029: Bind targets to verified source packages

- Status: Accepted
- Date: 2026-07-17

## Context

A valid engine project or interchange asset can otherwise preserve geometry
while losing the package, seed, provenance, and compatibility identities that
establish its GSPL origin and fidelity decisions.

## Decision

Issue a non-forgeable target-source evidence value only after independent
package verification. Preserve its canonical representation in every governed
target and include it in target identity. Use glTF application extras for GLB
interchange and a manifest-covered artifact for project adapters.

## Consequences

Targets can be traced byte-for-byte to their source governance boundary, and a
modified package cannot mint evidence. Standards-neutral consumers remain
compatible. This is content-addressed linkage rather than cryptographic signer
authentication.
