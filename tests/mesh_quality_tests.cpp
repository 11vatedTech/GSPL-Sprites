#include "gspl_sprites/mesh_quality.hpp"

#include <iostream>
#include <stdexcept>

using namespace gspl::sprites;
namespace {
void check(bool v, const char *m) {
  if (!v)
    throw std::runtime_error(m);
}
Vertex3d v(std::int64_t x, std::int64_t y, std::int32_t u, std::int32_t w) {
  return {{x, y, 0}, {0, 0, 1'000'000}, {u, w}, {}};
}
Mesh3d quad() {
  return {"quad",
          MeshPurpose::render,
          "mat",
          false,
          {v(0, 0, 0, 0), v(1, 0, 1'000'000, 0), v(1, 1, 1'000'000, 1'000'000),
           v(0, 1, 0, 1'000'000)},
          {0, 1, 2, 0, 2, 3}};
}
} // namespace
int main() try {
  const auto good = analyze_mesh_quality(quad());
  check(good.validation.ok() && good.uv_overlap_pairs == 0 &&
            good.degenerate_uv_triangles == 0 &&
            good.inconsistent_normal_triangles == 0 &&
            good.tangents.size() == 4 && good.tangents[0].x == 1'000'000,
        "valid mesh quality analysis failed");
  auto overlap = quad();
  overlap.vertices.insert(overlap.vertices.end(), overlap.vertices.begin(),
                          overlap.vertices.begin() + 3);
  overlap.triangle_indices.insert(overlap.triangle_indices.end(), {4, 5, 6});
  const auto bad = analyze_mesh_quality(overlap);
  check(!bad.validation.ok() && bad.uv_overlap_pairs == 1,
        "positive UV overlap not detected");
  auto degenerate = quad();
  degenerate.vertices[2].texture_coordinate = {1'000'000, 0};
  check(analyze_mesh_quality(degenerate).degenerate_uv_triangles > 0,
        "degenerate UV triangle not detected");
  MeshQualityPolicy bounded;
  bounded.maximum_uv_candidate_pairs = 1;
  bool limited = false;
  try {
    (void)analyze_mesh_quality(overlap, bounded);
  } catch (const std::length_error &) {
    limited = true;
  }
  check(limited, "UV candidate bound not enforced");
  std::cout << "all gspl sprites mesh quality tests passed\n";
  return 0;
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
