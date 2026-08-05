# GSPL Sprites — Identity Taxonomy

Seven distinct identity kinds coexist in the platform. They are deliberately NOT collapsed into one hash.

| # | Kind | Binds | Immutable | Changes/Form | Changes/Projection | Changes/Runtime | Serialized |
|---|---|---|---|---|---|---|---|
| 1 | Semantic Entity | CanonicalEntity stable_id + semantic fields | Yes | No | No | No | Yes |
| 2 | Seed Snapshot | sha256(canonicalize(SpriteSeed)) | Yes | No | No | No | Yes |
| 3 | Canonical IR | gspl::sprites::SpriteIr seed_identity | Yes | No | No | No | Yes |
| 4 | Runtime State | EntityStateIdentity + deterministic_event_hash | No | Yes | No | Yes | Yes |
| 5 | Visual Manifestation | Frame/channel/atlas semantic preimages | Yes | Yes | Yes | No | Yes |
| 6 | Artifact | Per-artifact SHA-256 (content-addressed) | Yes | No | Yes | No | Yes |
| 7 | Package | LivingVisualPackageManifest.package_identity | Yes | No | No | No | Yes |

API: `include/gspl_sprites/identity.hpp` — `IdentityKind` enum, `IdentityDescriptor`, `kIdentityTaxonomy`.
