#pragma once

#include "gspl_sprites/common.hpp"
#include "gspl_sprites/visual_geometry.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gspl::sprites::visual {

/* Performance semantics foundation: the minimal visual realization
 * inputs for a pose/performance state. Not the canonical animation
 * ontology; the Visual Semantic Compiler consumes this to bake a pose. */

struct PartMotion {
  std::string part_id;
  double dx{};
  double dy{};
  double rotation_degrees{};
  double scale_x{1.0};
  double scale_y{1.0};
  double opacity{0.0};  // 0 = unchanged
};

struct PerformanceState {
  std::string schema{"gspl.performance/0.1"};
  std::vector<PartMotion> motions;
  std::string motion_phase{"idle"};
  std::string action_phase;
  visual::Vec2 velocity_intent;
  std::string facing{"right"};
  std::string expression_intent;
  double secondary_motion{0.0};
  double impact{0.0};
};

[[nodiscard]] ValidationResult validate_performance(const PerformanceState& perf,
                                                    std::span<const std::string> part_ids);
[[nodiscard]] std::string canonicalize_performance(const PerformanceState& perf);
[[nodiscard]] std::string performance_identity(const PerformanceState& perf);

} // namespace gspl::sprites::visual
