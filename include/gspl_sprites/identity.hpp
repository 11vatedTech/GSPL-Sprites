#pragma once

#include "gspl_sprites/package.hpp"

#include <array>
#include <string_view>

namespace gspl::sprites {

/* ═══ IDENTITY TAXONOMY ═══
 *
 * Governing vocabulary for the seven distinct identity kinds that coexist in
 * the platform. They are deliberately NOT collapsed into one hash: each binds
 * different inputs and changes at different boundaries. See
 * docs/IDENTITY_TAXONOMY.md for the full specification.
 */

enum class IdentityKind {
  semantic_entity,      // canonical entity identity (stable across all projections/forms)
  seed_snapshot,        // canonical SpriteSeed bytes (sha256(canonicalize(seed)))
  canonical_ir,         // production gspl::sprites::SpriteIr identity
  runtime_state,        // EntityStateIdentity + deterministic_event_hash (changes during runtime)
  visual_manifestation, // frame-set / channel / atlas semantic identities
  artifact,             // per-artifact SHA-256 in the package inventory
  package               // package_identity (binds seed + artifacts + provenance)
};

struct IdentityDescriptor {
  IdentityKind kind{};
  std::string_view name;
  std::string_view binds;              // what inputs this identity is computed from
  std::string_view canonical_domain;   // provenance identity domain (where applicable)
  bool immutable{};                    // never changes once created
  bool changes_across_forms{};         // changes when the entity transforms
  bool changes_across_projections{};   // differs between 2D / 2.5D / 3D manifestations
  bool changes_during_runtime{};       // ticked simulation mutates it
  bool serialized{};                   // persisted in the Living Visual Package
};

inline constexpr std::array<IdentityDescriptor, 7> kIdentityTaxonomy{{
  {IdentityKind::semantic_entity,
   "semantic-entity", "CanonicalEntity stable_id + semantic fields",
   kDomainCanonicalEntity, true, false, false, false, true},
  {IdentityKind::seed_snapshot,
   "seed-snapshot", "sha256(canonicalize(SpriteSeed))",
   {}, true, false, false, false, true},
  {IdentityKind::canonical_ir,
   "canonical-ir", "gspl::sprites::SpriteIr seed_identity (seed snapshot)",
   {}, true, false, false, false, true},
  {IdentityKind::runtime_state,
   "runtime-state", "EntityStateIdentity fields + deterministic_event_hash",
   {}, false, true, false, true, true},
  {IdentityKind::visual_manifestation,
   "visual-manifestation", "frame / channel / atlas semantic preimages",
   {}, true, true, true, false, true},
  {IdentityKind::artifact,
   "artifact", "per-artifact SHA-256 (content-addressed)",
   {}, true, false, true, false, true},
  {IdentityKind::package,
   "package", "LivingVisualPackageManifest.package_identity",
   {}, true, false, false, false, true},
}};

[[nodiscard]] inline std::string_view identity_kind_name(IdentityKind kind) noexcept {
  for (const auto& descriptor : kIdentityTaxonomy)
    if (descriptor.kind == kind) return descriptor.name;
  return "unknown";
}

[[nodiscard]] inline const IdentityDescriptor*
identity_descriptor(IdentityKind kind) noexcept {
  for (const auto& descriptor : kIdentityTaxonomy)
    if (descriptor.kind == kind) return &descriptor;
  return nullptr;
}

/* Legacy-path classification vocabulary (see docs/IR_AUTHORITY.md). */
enum class PipelinePathRole {
  canonical,        // authoritative production path
  compatibility,    // supported adapter for legacy callers
  legacy,           // preserved but superseded
  fixture,          // example / acceptance fixture, never a production dependency
  deprecation_candidate // scheduled for removal behind versioned migration
};

[[nodiscard]] inline std::string_view pipeline_path_role_name(PipelinePathRole role) noexcept {
  switch (role) {
    case PipelinePathRole::canonical: return "canonical";
    case PipelinePathRole::compatibility: return "compatibility";
    case PipelinePathRole::legacy: return "legacy";
    case PipelinePathRole::fixture: return "fixture";
    case PipelinePathRole::deprecation_candidate: return "deprecation-candidate";
  }
  return "unknown";
}

} // namespace gspl::sprites
