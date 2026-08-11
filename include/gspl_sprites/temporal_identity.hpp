#pragma once

#include "gspl_sprites/common.hpp"
#include "gspl_sprites/visual_geometry.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gspl::sprites::visual {

/* ── Temporal Visual Identity ──
 * Every persistent visual structure (contour segment, marking, highlight,
 * shadow region, material region, effect emitter, landmark) carries a
 * stable semantic identity across frames. Independent frame generation
 * must never redefine persistent structures arbitrarily; this registry
 * is the deterministic correspondence authority between frames.
 *
 * It is presentation/runtime state, never package or semantic authority.
 * See docs/architecture/TEMPORAL_VISUAL_IDENTITY.md. */

enum class PersistentStructureKind {
  contour_segment, marking, highlight, shadow_region, material_region,
  effect_emitter, landmark, pixel_cluster
};
[[nodiscard]] std::string_view persistent_kind_name(PersistentStructureKind kind) noexcept;
[[nodiscard]] std::optional<PersistentStructureKind> persistent_kind_from_name(std::string_view name) noexcept;

struct PersistentStructure {
  std::string id;
  PersistentStructureKind kind{PersistentStructureKind::contour_segment};
  std::string anchor_part;          // semantic surface/part this is anchored to
  std::string surface_role;         // UV/surface-space feature binding if known
  std::uint64_t first_frame{0};
  std::uint64_t last_frame{0};
  double stability{1.0};            // 0..1 correspondence confidence
};

struct TemporalIdentityFrame {
  std::uint64_t frame_index{0};
  std::vector<PersistentStructure> structures;
};

struct TemporalIdentityRegistry {
  std::string schema{"gspl.temporal-identity/0.1"};
  std::string entity_id;
  std::uint64_t current_frame{0};
  std::vector<TemporalIdentityFrame> frames;   // bounded ring of recent frames
  std::map<std::string, PersistentStructure, std::less<>> live;  // id -> structure
};

struct TemporalLimits {
  std::uint32_t max_frames{64};
  std::uint32_t max_structures_per_frame{512};
  std::uint32_t max_live_structures{1024};
};

/* Register a frame's persistent structures. Structures from the previous
 * frame with the same id + kind + anchor are matched (stability 1.0);
 * new ids start fresh. Deterministic given identical inputs. */
[[nodiscard]] ValidationResult register_temporal_frame(
    TemporalIdentityRegistry& registry,
    std::uint64_t frame_index,
    std::span<const PersistentStructure> structures,
    const TemporalLimits& limits = {});

/* Deterministic structure correspondence between two registry frames:
 * returns pairs (prev_id, next_id) that share kind+anchor semantics. */
[[nodiscard]] std::vector<std::pair<std::string, std::string>>
correspond_structures(const TemporalIdentityRegistry& registry,
                      std::uint64_t prev_frame, std::uint64_t next_frame);

/* Anchored structure lookup: all live structures anchored to a part. */
[[nodiscard]] std::vector<std::string> structures_anchored_to(
    const TemporalIdentityRegistry& registry, std::string_view part_id);

/* Jitter metric 0..1: 1 = structure sets are maximally inconsistent
 * between the two frames (ids present in one but missing in the other). */
[[nodiscard]] double temporal_inconsistency(const TemporalIdentityRegistry& registry,
                                            std::uint64_t a, std::uint64_t b);

[[nodiscard]] ValidationResult validate_temporal_registry(const TemporalIdentityRegistry& registry);
[[nodiscard]] std::string canonicalize_temporal_registry(const TemporalIdentityRegistry& registry);
[[nodiscard]] std::string temporal_registry_identity(const TemporalIdentityRegistry& registry);

} // namespace gspl::sprites::visual
