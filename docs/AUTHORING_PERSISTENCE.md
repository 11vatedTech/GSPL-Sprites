# Authoring project persistence

Authoring projects use the bounded canonical `gspl.sprite-authoring/0.1` text profile. The format retains revision ancestry, alternatives by stable semantic path, selected alternative indices, locks, abilities, and variant operations. Serialization sorts semantically unordered collections.

All free-form bytes use uppercase percent encoding. Structural delimiters, whitespace, control bytes, and UTF-8 bytes are encoded, while only ASCII alphanumeric characters plus `.`, `_`, and `-` remain literal. Lowercase, unnecessary, truncated, or malformed escapes are rejected. Parsing accepts only the serializer's exact canonical bytes, preventing multiple identities for equivalent state.

Inputs are limited to 1 MiB, 16,384 lines, and 16 KiB per line. Collection limits are enforced again by the authoring-domain validator. File loading rejects missing, non-regular, and symlink inputs. Saving writes and rereads a sibling staging file before atomic rename, refuses existing destinations and staging paths, and removes failed staging files.

The format is a durable compiler input, not a UI cache. Canonical JSON remains the identity representation, so source-format changes can be migrated without redefining semantic revision identity.
