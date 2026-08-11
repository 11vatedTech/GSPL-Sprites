#include "gspl_sprites/cinematic.hpp"
#include "gspl_sprites/core.hpp"

#include <cmath>
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

std::string_view shot_scale_name(ShotScale scale) noexcept {
  switch (scale) {
    case ShotScale::extreme_close_up: return "extreme_close_up";
    case ShotScale::close_up: return "close_up";
    case ShotScale::medium: return "medium";
    case ShotScale::full: return "full";
    case ShotScale::wide: return "wide";
    case ShotScale::establishing: return "establishing";
  }
  return "unknown";
}

std::optional<ShotScale> shot_scale_from_name(std::string_view name) noexcept {
  for (int i = 0; i <= static_cast<int>(ShotScale::establishing); ++i) {
    const auto scale = static_cast<ShotScale>(i);
    if (shot_scale_name(scale) == name) return scale;
  }
  return std::nullopt;
}

std::string_view camera_movement_name(CameraMovement move) noexcept {
  switch (move) {
    case CameraMovement::static_: return "static";
    case CameraMovement::pan: return "pan";
    case CameraMovement::track: return "track";
    case CameraMovement::dolly: return "dolly";
    case CameraMovement::tilt: return "tilt";
    case CameraMovement::orbit: return "orbit";
    case CameraMovement::handheld: return "handheld";
    case CameraMovement::push_in: return "push_in";
    case CameraMovement::pull_out: return "pull_out";
  }
  return "unknown";
}

std::optional<CameraMovement> camera_movement_from_name(std::string_view name) noexcept {
  for (int i = 0; i <= static_cast<int>(CameraMovement::pull_out); ++i) {
    const auto move = static_cast<CameraMovement>(i);
    if (camera_movement_name(move) == name) return move;
  }
  return std::nullopt;
}

ValidationResult validate_cinematic(const CinematicSpec& spec, const CinematicLimits& limits) {
  ValidationResult result;
  auto add = [&](std::string code, const std::string& msg) {
    result.diagnostics.push_back({std::move(code), msg});
  };
  if (spec.schema != "gspl.cinematic/0.1") {
    add("CINEMATIC_SCHEMA", "unexpected schema '" + spec.schema + "'");
  }
  if (spec.entity_id.empty()) {
    add("CINEMATIC_NO_ENTITY", "cinematic spec has no entity id");
  }
  const CameraSpec& c = spec.camera;
  if (!std::isfinite(c.position.x) || !std::isfinite(c.position.y) ||
      !std::isfinite(c.zoom) || !std::isfinite(c.rotation_degrees)) {
    add("CINEMATIC_NONFINITE", "camera transform is non-finite");
  }
  if (!(c.zoom > 0.0)) add("CINEMATIC_ZOOM", "camera zoom must be > 0");
  if (!(c.focal_priority >= 0.0 && c.focal_priority <= 1.0)) {
    add("CINEMATIC_RANGE", "focal_priority outside [0,1]");
  }
  if (!(c.perspective_ratio >= 0.0 && c.perspective_ratio <= 1.0)) {
    add("CINEMATIC_RANGE", "perspective_ratio outside [0,1]");
  }
  if (!(c.parallax_layers >= 1.0)) add("CINEMATIC_RANGE", "parallax_layers must be >= 1");
  if (spec.impact_staging.size() > limits.max_staging) {
    add("CINEMATIC_LIMIT", "impact staging exceeds max " + std::to_string(limits.max_staging));
  }
  for (const ImpactStaging& s : spec.impact_staging) {
    if (!std::isfinite(s.impact_position.x) || !std::isfinite(s.impact_position.y) ||
        !std::isfinite(s.impact_strength) || !std::isfinite(s.freeze_frames) ||
        !std::isfinite(s.shake_intensity)) {
      add("CINEMATIC_NONFINITE", "impact staging is non-finite");
    }
    if (!(s.impact_position.x >= 0.0 && s.impact_position.x <= 1.0 &&
          s.impact_position.y >= 0.0 && s.impact_position.y <= 1.0)) {
      add("CINEMATIC_RANGE", "impact position outside normalized [0,1]");
    }
    if (!(s.impact_strength >= 0.0 && s.impact_strength <= 1.0)) {
      add("CINEMATIC_RANGE", "impact_strength outside [0,1]");
    }
  }
  if (spec.focal_hierarchy.size() > limits.max_focal_hierarchy) {
    add("CINEMATIC_LIMIT", "focal hierarchy exceeds max " + std::to_string(limits.max_focal_hierarchy));
  }
  return result;
}

std::string canonicalize_cinematic(const CinematicSpec& spec) {
  std::ostringstream out;
  const CameraSpec& c = spec.camera;
  out << "{\"schema\":\"" << escape_json(spec.schema)
      << "\",\"entity\":\"" << escape_json(spec.entity_id)
      << "\",\"camera\":{\"x\":" << c.position.x << ",\"y\":" << c.position.y
      << ",\"zoom\":" << c.zoom << ",\"rot\":" << c.rotation_degrees
      << ",\"shot\":\"" << escape_json(shot_scale_name(c.shot_scale))
      << "\",\"move\":\"" << escape_json(camera_movement_name(c.movement))
      << "\",\"focal\":" << c.focal_priority
      << ",\"screen_dir\":" << c.screen_direction
      << ",\"perspective\":" << c.perspective_ratio
      << ",\"parallax\":" << c.parallax_layers
      << ",\"depth\":" << c.depth_scale << "}";
  out << ",\"staging\":[";
  for (std::size_t i = 0; i < spec.impact_staging.size(); ++i) {
    if (i) out << ",";
    const ImpactStaging& s = spec.impact_staging[i];
    out << "{\"x\":" << s.impact_position.x << ",\"y\":" << s.impact_position.y
        << ",\"strength\":" << s.impact_strength
        << ",\"freeze\":" << s.freeze_frames
        << ",\"shake\":" << s.shake_intensity
        << ",\"landmark\":\"" << escape_json(s.staged_landmark) << "\"}";
  }
  out << "]";
  out << ",\"focal_hierarchy\":[";
  for (std::size_t i = 0; i < spec.focal_hierarchy.size(); ++i) {
    if (i) out << ",";
    out << "\"" << escape_json(spec.focal_hierarchy[i]) << "\"";
  }
  out << "]}";
  return out.str();
}

std::string cinematic_identity(const CinematicSpec& spec) {
  return gspl::sprites::sha256(canonicalize_cinematic(spec));
}

} // namespace gspl::sprites::visual
