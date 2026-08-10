#include "gspl_sprites/performance.hpp"
#include "gspl_sprites/core.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace gspl::sprites::visual {
namespace {

[[nodiscard]] bool contains_part(std::span<const std::string> ids, std::string_view id) {
  return std::any_of(ids.begin(), ids.end(), [&](const std::string& s) { return s == id; });
}

} // namespace

ValidationResult validate_performance(const PerformanceState& perf, std::span<const std::string> part_ids) {
  ValidationResult result;
  if (perf.schema != "gspl.performance/0.1") {
    result.diagnostics.push_back({"PERFORMANCE_SCHEMA", "unexpected performance schema"});
  }
  if (perf.facing != "left" && perf.facing != "right") {
    result.diagnostics.push_back({"PERFORMANCE_FACING", "facing must be 'left' or 'right'"});
  }
  if (!(perf.impact >= 0.0 && perf.impact <= 1.0)) {
    result.diagnostics.push_back({"PERFORMANCE_IMPACT", "impact outside [0,1]"});
  }
  if (!(perf.secondary_motion >= 0.0 && perf.secondary_motion <= 1.0)) {
    result.diagnostics.push_back({"PERFORMANCE_SECONDARY", "secondary_motion outside [0,1]"});
  }
  if (!std::isfinite(perf.velocity_intent.x) || !std::isfinite(perf.velocity_intent.y)) {
    result.diagnostics.push_back({"PERFORMANCE_VELOCITY", "velocity_intent is non-finite"});
  }
  for (const auto& m : perf.motions) {
    if (m.part_id.empty()) {
      result.diagnostics.push_back({"PERFORMANCE_MOTION_ID", "a motion has an empty part id"});
      continue;
    }
    if (!contains_part(part_ids, m.part_id)) {
      result.diagnostics.push_back({"PERFORMANCE_PART_NOT_FOUND",
          "motion references unknown part '" + m.part_id + "'"});
    }
    if (!std::isfinite(m.dx) || !std::isfinite(m.dy) || !std::isfinite(m.rotation_degrees) ||
        !std::isfinite(m.scale_x) || !std::isfinite(m.scale_y)) {
      result.diagnostics.push_back({"PERFORMANCE_MOTION_NONFINITE",
          "motion for '" + m.part_id + "' is non-finite"});
    }
    if (m.scale_x <= 0.0 || m.scale_y <= 0.0) {
      result.diagnostics.push_back({"PERFORMANCE_MOTION_SCALE",
          "motion for '" + m.part_id + "' scale must be > 0"});
    }
    if (!(m.opacity >= 0.0 && m.opacity <= 1.0)) {
      result.diagnostics.push_back({"PERFORMANCE_MOTION_OPACITY",
          "motion for '" + m.part_id + "' opacity outside [0,1]"});
    }
  }
  return result;
}

std::string canonicalize_performance(const PerformanceState& perf) {
  std::ostringstream out;
  out << "{\"schema\":\"" << perf.schema << "\""
      << ",\"phase\":\"" << perf.motion_phase << "\""
      << ",\"action\":\"" << perf.action_phase << "\""
      << ",\"vx\":" << perf.velocity_intent.x
      << ",\"vy\":" << perf.velocity_intent.y
      << ",\"facing\":\"" << perf.facing << "\""
      << ",\"expression\":\"" << perf.expression_intent << "\""
      << ",\"secondary\":" << perf.secondary_motion
      << ",\"impact\":" << perf.impact;
  out << ",\"motions\":[";
  bool first = true;
  for (const auto& m : perf.motions) {
    if (!first) out << ",";
    first = false;
    out << "{\"id\":\"" << m.part_id << "\",\"dx\":" << m.dx
        << ",\"dy\":" << m.dy << ",\"rot\":" << m.rotation_degrees
        << ",\"sx\":" << m.scale_x << ",\"sy\":" << m.scale_y
        << ",\"opacity\":" << m.opacity << "}";
  }
  out << "]}";
  return out.str();
}

std::string performance_identity(const PerformanceState& perf) {
  return gspl::sprites::sha256(canonicalize_performance(perf));
}

} // namespace gspl::sprites::visual
