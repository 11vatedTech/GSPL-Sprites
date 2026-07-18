#include "gspl_sprites/lod_quality.hpp"

#include <iostream>
#include <stdexcept>

using namespace gspl::sprites;
namespace {
void check(bool v, const char *m) {
  if (!v)
    throw std::runtime_error(m);
}
Vertex3d v(std::int64_t x, std::int64_t y) {
  return {{x, y, 0}, {0, 0, 1'000'000}, {0, 0}, {}};
}
Projection3dDefinition projection() {
  Projection3dDefinition p;
  p.id = "lod";
  p.materials = {{"mat"}, {"other"}};
  Mesh3d high{
      "high",
      MeshPurpose::render,
      "mat",
      false,
      {v(0, 0), v(1'000'000, 0), v(1'000'000, 1'000'000), v(0, 1'000'000)},
      {0, 1, 2, 0, 2, 3}};
  Mesh3d low{"low", MeshPurpose::render, "mat",
             false, high.vertices,       {0, 1, 2}};
  p.meshes = {high, low};
  p.lods = {{0, "high", 800'000}, {1, "low", 100'000}};
  return p;
}
} // namespace
int main() try {
  const auto p = projection();
  const auto report = analyze_lod_quality(p);
  check(report.validation.ok() && report.levels.size() == 1 &&
            report.levels[0].maximum_geometric_error_micrometers > 700'000,
        "LOD geometric error analysis failed");
  LodQualityPolicy strict;
  strict.maximum_geometric_error_micrometers = 500'000;
  check(!analyze_lod_quality(p, strict).validation.ok(),
        "excessive LOD error accepted");
  auto material = p;
  material.meshes[1].material_id = "other";
  check(!analyze_lod_quality(material).validation.ok(),
        "LOD material mismatch accepted");
  LodQualityPolicy bounded;
  bounded.maximum_distance_evaluations = 1;
  bool limited = false;
  try {
    (void)analyze_lod_quality(p, bounded);
  } catch (const std::length_error &) {
    limited = true;
  }
  check(limited, "LOD work bound ignored");
  std::cout << "all gspl sprites LOD quality tests passed\n";
  return 0;
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
