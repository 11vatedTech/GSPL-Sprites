#pragma once

#include "gspl_sprites/projection3d.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace gspl::sprites {

struct LodQualityPolicy {
  std::uint64_t maximum_distance_evaluations{64ULL * 1024ULL * 1024ULL};
  std::uint64_t maximum_geometric_error_micrometers{1'000'000};
  bool require_material_match{true};
};

struct LodLevelQuality {
  std::uint32_t level{};
  std::string mesh_id;
  std::uint64_t distance_evaluations{};
  std::uint64_t maximum_geometric_error_micrometers{};
};

struct LodQualityReport {
  std::uint64_t total_distance_evaluations{};
  std::vector<LodLevelQuality> levels;
  ValidationResult validation;
};

[[nodiscard]] LodQualityReport
analyze_lod_quality(const Projection3dDefinition &projection,
                    const LodQualityPolicy &policy = {});

} // namespace gspl::sprites
