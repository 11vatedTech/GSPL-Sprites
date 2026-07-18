# Authoring command-line workflow

The CLI exposes the durable authoring boundary without editing canonical seeds in place:

```text
authoring-inspect <project>
authoring-revise <project> <expected-identity> <output> <path> <selected-or-keep> <lock|unlock|keep> [...]
authoring-lower <project> <output-seed-json> [variant]
authoring-build <project> <output-package> [variant]
```

`inspect` emits canonical project JSON together with its revision identity. `revise` applies one or more field edits under optimistic concurrency and publishes a new immutable project file; it never updates the input. Unlocking and value replacement remain separate revisions because the domain API rejects a selected-value edit against a currently locked field.

`lower` applies an optional named variant, validates all mandatory choices, rights, types, and seed semantics, then transactionally publishes canonical seed JSON after byte-for-byte staging verification. `build` follows the same lowering path, publishes through a CLI-owned staging directory, independently verifies the portable package, and only then renames it to the destination. Existing output or staging paths fail rather than being overwritten.

Diagnostics use a distinct `GSPL_SPRITES_AUTHORING` prefix. Unsupported commands and malformed argument groups fail without writing outputs.
