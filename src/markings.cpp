#include "gspl_sprites/markings.hpp"

#include <algorithm>
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

std::string_view marking_kind_name(MarkingKind kind) noexcept {
  switch (kind) {
    case MarkingKind::stripe: return "stripe";
    case MarkingKind::spot: return "spot";
    case MarkingKind::band: return "band";
    case MarkingKind::gradient_region: return "gradient_region";
    case MarkingKind::symbol: return "symbol";
    case MarkingKind::vein: return "vein";
    case MarkingKind::circuit: return "circuit";
    case MarkingKind::scar: return "scar";
    case MarkingKind::tattoo: return "tattoo";
    case MarkingKind::fur_marking: return "fur_marking";
    case MarkingKind::electrical: return "electrical";
    case MarkingKind::procedural: return "procedural";
  }
  return "unknown";
}

std::optional<MarkingKind> marking_kind_from_name(std::string_view name) noexcept {
  for (int i = 0; i <= static_cast<int>(MarkingKind::procedural); ++i) {
    const auto kind = static_cast<MarkingKind>(i);
    if (marking_kind_name(kind) == name) return kind;
  }
  return std::nullopt;
}

ValidationResult validate_markings(std::span<const Marking> markings,
                                   std::span<const std::string> part_ids,
                                   std::uint32_t max_markings) {
  ValidationResult result;
  if (markings.size() > max_markings) {
    result.diagnostics.push_back({"MARKING_LIMIT_EXCEEDED",
        "marking count " + std::to_string(markings.size()) + " exceeds max " + std::to_string(max_markings)});
  }
  for (std::size_t i = 0; i < markings.size(); ++i) {
    const Marking& m = markings[i];
    if (m.id.empty()) {
      result.diagnostics.push_back({"MARKING_EMPTY_ID", "marking at index " + std::to_string(i) + " has no id"});
    }
    if (std::none_of(part_ids.begin(), part_ids.end(),
                     [&](const std::string& pid) { return pid == m.part_id; })) {
      result.diagnostics.push_back({"MARKING_PART_NOT_FOUND",
          "marking '" + m.id + "' binds unknown part '" + m.part_id + "'"});
    }
    if (!(m.opacity >= 0.0 && m.opacity <= 1.0)) {
      result.diagnostics.push_back({"MARKING_OPACITY_INVALID",
          "marking '" + m.id + "' opacity " + std::to_string(m.opacity) + " outside [0,1]"});
    }
    for (const auto& v : m.path) {
      if (!std::isfinite(v.x) || !std::isfinite(v.y)) {
        result.diagnostics.push_back({"MARKING_NONFINITE", "marking '" + m.id + "' has non-finite path point"});
        break;
      }
    }
    for (const auto& v : m.spots) {
      if (!std::isfinite(v.x) || !std::isfinite(v.y)) {
        result.diagnostics.push_back({"MARKING_NONFINITE", "marking '" + m.id + "' has non-finite spot"});
        break;
      }
    }
  }
  return result;
}

std::string canonicalize_marking(const Marking& m) {
  std::ostringstream out;
  out << "{\"id\":\"" << escape_json(m.id)
      << "\",\"kind\":\"" << escape_json(marking_kind_name(m.kind))
      << "\",\"part\":\"" << escape_json(m.part_id)
      << "\",\"color\":" << m.color
      << ",\"role\":\"" << escape_json(color_role_name(m.color_role))
      << "\",\"opacity\":" << m.opacity
      << ",\"x\":" << m.x << ",\"y\":" << m.y
      << ",\"rot\":" << m.rotation_degrees
      << ",\"scale\":" << m.scale
      << ",\"band_width\":" << m.band_width
      << ",\"spot_radius\":" << m.spot_radius
      << ",\"seed\":" << m.seed << ",\"density\":" << m.density;
  out << ",\"path\":[";
  for (std::size_t i = 0; i < m.path.size(); ++i) {
    if (i) out << ",";
    out << m.path[i].x << "," << m.path[i].y;
  }
  out << "]";
  out << ",\"widths\":[";
  for (std::size_t i = 0; i < m.widths.size(); ++i) {
    if (i) out << ",";
    out << m.widths[i];
  }
  out << "]";
  out << ",\"spots\":[";
  for (std::size_t i = 0; i < m.spots.size(); ++i) {
    if (i) out << ",";
    out << m.spots[i].x << "," << m.spots[i].y;
  }
  out << "]}";
  return out.str();
}

std::string canonicalize_markings(std::span<const Marking> markings) {
  std::string out = "[";
  for (std::size_t i = 0; i < markings.size(); ++i) {
    if (i) out += ",";
    out += canonicalize_marking(markings[i]);
  }
  out += "]";
  return out;
}

} // namespace gspl::sprites::visual
