#include "gspl_sprites/core.hpp"
#include "gspl_sprites/temporal_identity.hpp"

#include <algorithm>
#include <set>
#include <sstream>

namespace gspl::sprites::visual {
namespace {

std::string escape_json(std::string_view value) {
  std::string out;
  for (const unsigned char c : value) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"':  out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c < 0x20) {
          out += "\\u00";
          const char* hex = "0123456789abcdef";
          out += hex[(c >> 4) & 0xF];
          out += hex[c & 0xF];
        } else {
          out += static_cast<char>(c);
        }
    }
  }
  return out;
}

} // namespace

std::string_view persistent_kind_name(PersistentStructureKind kind) noexcept {
  switch (kind) {
    case PersistentStructureKind::contour_segment: return "contour_segment";
    case PersistentStructureKind::marking: return "marking";
    case PersistentStructureKind::highlight: return "highlight";
    case PersistentStructureKind::shadow_region: return "shadow_region";
    case PersistentStructureKind::material_region: return "material_region";
    case PersistentStructureKind::effect_emitter: return "effect_emitter";
    case PersistentStructureKind::landmark: return "landmark";
    case PersistentStructureKind::pixel_cluster: return "pixel_cluster";
  }
  return "unknown";
}

std::optional<PersistentStructureKind> persistent_kind_from_name(std::string_view name) noexcept {
  for (int i = 0; i <= static_cast<int>(PersistentStructureKind::pixel_cluster); ++i) {
    const auto kind = static_cast<PersistentStructureKind>(i);
    if (persistent_kind_name(kind) == name) return kind;
  }
  return std::nullopt;
}

ValidationResult register_temporal_frame(
    TemporalIdentityRegistry& registry,
    std::uint64_t frame_index,
    std::span<const PersistentStructure> structures,
    const TemporalLimits& limits) {
  ValidationResult result;
  auto add = [&](std::string code, const std::string& msg) {
    result.diagnostics.push_back({std::move(code), msg});
  };
  if (structures.size() > limits.max_structures_per_frame) {
    add("TEMPORAL_STRUCTURE_LIMIT", "frame " + std::to_string(frame_index) + " has " +
        std::to_string(structures.size()) + " structures, max " +
        std::to_string(limits.max_structures_per_frame));
  }
  std::set<std::string> seen;
  for (const PersistentStructure& s : structures) {
    if (s.id.empty()) add("TEMPORAL_EMPTY_ID", "a persistent structure has an empty id");
    if (!seen.insert(s.id).second) {
      add("TEMPORAL_DUPLICATE_ID", "duplicate persistent structure id '" + s.id + "'");
    }
    if (!(s.stability >= 0.0 && s.stability <= 1.0)) {
      add("TEMPORAL_STABILITY", "structure '" + s.id + "' stability outside [0,1]");
    }
  }

  TemporalIdentityFrame frame;
  frame.frame_index = frame_index;
  // Deterministic ordering by id (already span order; enforce sorted by id
  // for canonical stability across callers).
  std::vector<PersistentStructure> sorted(structures.begin(), structures.end());
  std::stable_sort(sorted.begin(), sorted.end(),
                   [](const PersistentStructure& a, const PersistentStructure& b) { return a.id < b.id; });
  frame.structures = std::move(sorted);

  // Match against previous frame: same id + kind + anchor -> stability 1.0.
  if (!registry.frames.empty() && registry.frames.back().frame_index == frame_index) {
    registry.frames.back() = std::move(frame);
  } else {
    if (registry.frames.size() >= limits.max_frames) {
      registry.frames.erase(registry.frames.begin());
    }
    registry.frames.push_back(std::move(frame));
  }
  registry.current_frame = frame_index;

  // Rebuild live set: structures of the latest frame, with matched ids kept.
  std::map<std::string, PersistentStructure, std::less<>> next_live;
  for (const PersistentStructure& s : registry.frames.back().structures) {
    PersistentStructure live_s = s;
    live_s.last_frame = frame_index;
    if (frame_index > 0) {
      const auto it = registry.live.find(s.id);
      if (it != registry.live.end() && it->second.kind == s.kind &&
          it->second.anchor_part == s.anchor_part) {
        live_s.first_frame = it->second.first_frame;
        live_s.stability = 1.0;
      } else {
        live_s.first_frame = frame_index;
        live_s.stability = 0.0;
      }
    } else {
      live_s.first_frame = 0;
      live_s.stability = 0.0;
    }
    next_live.emplace(s.id, std::move(live_s));
  }
  if (next_live.size() > limits.max_live_structures) {
    add("TEMPORAL_LIVE_LIMIT", "live structure count exceeds max " +
        std::to_string(limits.max_live_structures));
    // Fail closed: do not install an over-limit live set.
    return result;
  }
  registry.live = std::move(next_live);
  return result;
}

std::vector<std::pair<std::string, std::string>>
correspond_structures(const TemporalIdentityRegistry& registry,
                      std::uint64_t prev_frame, std::uint64_t next_frame) {
  std::vector<std::pair<std::string, std::string>> out;
  const TemporalIdentityFrame* a = nullptr;
  const TemporalIdentityFrame* b = nullptr;
  for (const auto& f : registry.frames) {
    if (f.frame_index == prev_frame) a = &f;
    if (f.frame_index == next_frame) b = &f;
  }
  if (!a || !b) return out;
  // Same id + kind + anchor = correspondence (deterministic, sorted).
  std::size_t i = 0, j = 0;
  while (i < a->structures.size() && j < b->structures.size()) {
    const PersistentStructure& sa = a->structures[i];
    const PersistentStructure& sb = b->structures[j];
    if (sa.id < sb.id) { ++i; continue; }
    if (sb.id < sa.id) { ++j; continue; }
    if (sa.kind == sb.kind && sa.anchor_part == sb.anchor_part) {
      out.emplace_back(sa.id, sb.id);
    }
    ++i; ++j;
  }
  return out;
}

std::vector<std::string> structures_anchored_to(
    const TemporalIdentityRegistry& registry, std::string_view part_id) {
  std::vector<std::string> out;
  for (const auto& [id, s] : registry.live) {
    (void)id;
    if (s.anchor_part == part_id) out.push_back(s.id);
  }
  return out;
}

double temporal_inconsistency(const TemporalIdentityRegistry& registry,
                              std::uint64_t a, std::uint64_t b) {
  const TemporalIdentityFrame* fa = nullptr;
  const TemporalIdentityFrame* fb = nullptr;
  for (const auto& f : registry.frames) {
    if (f.frame_index == a) fa = &f;
    if (f.frame_index == b) fb = &f;
  }
  if (!fa || !fb) return 1.0;
  std::set<std::string> ids_a, ids_b;
  for (const auto& s : fa->structures) ids_a.insert(s.id);
  for (const auto& s : fb->structures) ids_b.insert(s.id);
  std::size_t common = 0;
  for (const auto& id : ids_a) {
    if (ids_b.count(id)) ++common;
  }
  const std::size_t total = ids_a.size() + ids_b.size();
  if (total == 0) return 0.0;
  return 1.0 - (2.0 * static_cast<double>(common)) / static_cast<double>(total);
}

ValidationResult validate_temporal_registry(const TemporalIdentityRegistry& registry) {
  ValidationResult result;
  if (registry.schema != "gspl.temporal-identity/0.1") {
    result.diagnostics.push_back({"TEMPORAL_SCHEMA", "unexpected schema '" + registry.schema + "'"});
  }
  if (registry.entity_id.empty()) {
    result.diagnostics.push_back({"TEMPORAL_NO_ENTITY", "temporal registry has no entity id"});
  }
  for (const auto& f : registry.frames) {
    std::set<std::string> seen;
    for (const PersistentStructure& s : f.structures) {
      if (s.id.empty()) {
        result.diagnostics.push_back({"TEMPORAL_EMPTY_ID", "frame " +
            std::to_string(f.frame_index) + " contains an empty structure id"});
      }
      if (!seen.insert(s.id).second) {
        result.diagnostics.push_back({"TEMPORAL_DUPLICATE_ID",
            "frame " + std::to_string(f.frame_index) + " duplicates structure id '" + s.id + "'"});
      }
    }
  }
  return result;
}

std::string canonicalize_temporal_registry(const TemporalIdentityRegistry& registry) {
  std::ostringstream out;
  out << "{\"schema\":\"" << escape_json(registry.schema)
      << "\",\"entity\":\"" << escape_json(registry.entity_id)
      << "\",\"frame\":" << registry.current_frame
      << ",\"frames\":[";
  for (std::size_t i = 0; i < registry.frames.size(); ++i) {
    if (i) out << ",";
    const auto& f = registry.frames[i];
    out << "{\"index\":" << f.frame_index << ",\"structures\":[";
    for (std::size_t j = 0; j < f.structures.size(); ++j) {
      if (j) out << ",";
      const PersistentStructure& s = f.structures[j];
      out << "{\"id\":\"" << escape_json(s.id)
          << "\",\"kind\":\"" << escape_json(persistent_kind_name(s.kind))
          << "\",\"anchor\":\"" << escape_json(s.anchor_part)
          << "\",\"surface\":\"" << escape_json(s.surface_role)
          << "\",\"first\":" << s.first_frame
          << ",\"last\":" << s.last_frame
          << ",\"stability\":" << s.stability << "}";
    }
    out << "]}";
  }
  out << "]}";
  return out.str();
}

std::string temporal_registry_identity(const TemporalIdentityRegistry& registry) {
  return gspl::sprites::sha256(canonicalize_temporal_registry(registry));
}

} // namespace gspl::sprites::visual
