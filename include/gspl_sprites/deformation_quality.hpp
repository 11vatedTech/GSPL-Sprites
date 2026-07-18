#pragma once

#include "gspl_sprites/animation3d.hpp"

namespace gspl::sprites {

struct DeformationQualityPolicy {
  std::uint32_t maximum_sampled_ticks{4'096};
  std::uint64_t maximum_vertex_evaluations{64ULL * 1024ULL * 1024ULL};
  std::uint32_t minimum_triangle_area_ratio_per_million{10'000};
  std::uint64_t maximum_vertex_displacement_micrometers{100'000'000};
};

struct DeformationQualityReport {
  std::uint32_t sampled_ticks{};
  std::uint64_t vertex_evaluations{};
  std::uint64_t collapsed_triangle_samples{};
  std::uint32_t minimum_triangle_area_ratio_per_million{1'000'000};
  std::uint64_t maximum_vertex_displacement_micrometers{};
  ValidationResult validation;
};

[[nodiscard]] DeformationQualityReport
analyze_deformation_quality(const Projection3dDefinition &projection,
                            const AnimationClip3d &clip,
                            const DeformationQualityPolicy &policy = {});

} // namespace gspl::sprites
