#pragma once

#include "gspl_sprites/projection3d.hpp"

#include <cstdint>
#include <vector>

namespace gspl::sprites {

struct Tangent4Ppm {
  std::int32_t x{};
  std::int32_t y{};
  std::int32_t z{};
  std::int32_t handedness{1};
};

struct MeshQualityPolicy {
  std::uint64_t maximum_triangles{1'000'000};
  std::uint64_t maximum_uv_candidate_pairs{50'000'000};
  bool require_non_overlapping_uvs{true};
  bool require_non_degenerate_uvs{true};
  bool require_consistent_normals{true};
};

struct MeshQualityReport {
  std::uint64_t triangle_count{};
  std::uint64_t uv_candidate_pairs{};
  std::uint64_t uv_overlap_pairs{};
  std::uint64_t degenerate_uv_triangles{};
  std::uint64_t inconsistent_normal_triangles{};
  std::vector<Tangent4Ppm> tangents;
  ValidationResult validation;
};

[[nodiscard]] MeshQualityReport
analyze_mesh_quality(const Mesh3d &mesh, const MeshQualityPolicy &policy = {});

} // namespace gspl::sprites
