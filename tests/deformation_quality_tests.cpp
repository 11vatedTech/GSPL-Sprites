#include "gspl_sprites/deformation_quality.hpp"

#include <iostream>
#include <stdexcept>

using namespace gspl::sprites;
namespace {
void check(bool v, const char *m) {
  if (!v)
    throw std::runtime_error(m);
}
Projection3dDefinition projection() {
  Projection3dDefinition p;
  p.id = "deform";
  p.materials = {{"mat"}};
  p.skeleton = Skeleton3d{"rig", {{"root", {}}}};
  Vertex3d a{{0, 0, 0}, {0, 0, 1'000'000}, {0, 0}, {{"root", 1'000'000}}},
      b = a, c = a;
  b.position.x = 1'000'000;
  c.position.y = 1'000'000;
  p.meshes = {
      {"mesh", MeshPurpose::render, "mat", false, {a, b, c}, {0, 1, 2}}};
  return p;
}
AnimationClip3d clip(std::int32_t scale = 1'000'000) {
  return {
      "move",
      60,
      10,
      false,
      {{"root",
        {{0, {}},
         {10,
          {{1'000'000, 0, 0}, {0, 0, 0, 1'000'000}, {scale, scale, scale}}}}}},
      {},
      {}};
}
} // namespace
int main() try {
  const auto p = projection();
  const auto good = analyze_deformation_quality(p, clip());
  check(good.validation.ok() &&
            good.maximum_vertex_displacement_micrometers == 1'000'000 &&
            good.collapsed_triangle_samples == 0,
        "valid deformation analysis failed");
  const auto collapsed = analyze_deformation_quality(p, clip(1));
  check(!collapsed.validation.ok() && collapsed.collapsed_triangle_samples > 0,
        "collapsed deformation accepted");
  DeformationQualityPolicy displacement;
  displacement.maximum_vertex_displacement_micrometers = 500'000;
  check(!analyze_deformation_quality(p, clip(), displacement).validation.ok(),
        "excessive displacement accepted");
  DeformationQualityPolicy bounded;
  bounded.maximum_vertex_evaluations = 2;
  bool limited = false;
  try {
    (void)analyze_deformation_quality(p, clip(), bounded);
  } catch (const std::length_error &) {
    limited = true;
  }
  check(limited, "deformation work bound ignored");
  std::cout << "all gspl sprites deformation quality tests passed\n";
  return 0;
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
