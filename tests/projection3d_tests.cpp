#include "gspl_sprites/projection3d.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace gspl::sprites;
namespace {
void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
Vertex3d vertex(std::int64_t x, std::int64_t y, std::int64_t z) {
  return {{x, y, z}, {0, 0, 1'000'000}, {0, 0}, {{"root", 1'000'000}}};
}
Projection3dDefinition fixture() {
  Projection3dDefinition p;
  p.id = "entity.fox.3d";
  p.materials = {{"fur",
                  0xff8040ff,
                  0,
                  800'000,
                  MaterialAlphaMode::opaque,
                  500'000,
                  false,
                  "texture.fur",
                  "texture.fur.normal",
                  {}}};
  p.skeleton = Skeleton3d{"fox.rig", {{"root", {}, {}, {0, 0, 0, 1'000'000}}}};
  Mesh3d high{"body.high",
              MeshPurpose::render,
              "fur",
              true,
              {vertex(0, 0, 0), vertex(1'000'000, 0, 0),
               vertex(0, 1'000'000, 0), vertex(0, 0, 1'000'000)},
              {0, 2, 1, 0, 1, 3, 1, 2, 3, 2, 0, 3}};
  Mesh3d low = high;
  low.id = "body.low";
  low.require_closed_manifold = false;
  low.triangle_indices = {0, 2, 1};
  Mesh3d collision = high;
  collision.id = "collision.body";
  collision.purpose = MeshPurpose::collision;
  collision.material_id.reset();
  p.meshes = {high, low, collision};
  p.morph_targets = {
      {"expression.snarl", "body.high", std::vector<Vector3Micrometers>(4)}};
  p.lods = {{0, "body.high", 800'000}, {1, "body.low", 100'000}};
  return p;
}
} // namespace
int main() try {
  const auto p = fixture();
  check(validate_projection3d(p).ok(), "valid 3D projection rejected");
  auto reordered = p;
  std::reverse(reordered.materials.begin(), reordered.materials.end());
  std::reverse(reordered.meshes.begin(), reordered.meshes.end());
  check(canonicalize_projection3d(p) == canonicalize_projection3d(reordered),
        "3D canonicalization depends on declaration order");
  auto bad = p;
  bad.meshes[0].triangle_indices[1] = bad.meshes[0].triangle_indices[0];
  check(!validate_projection3d(bad).ok(), "degenerate triangle accepted");
  auto weights = p;
  weights.meshes[0].vertices[0].influences[0].weight_per_million = 999'999;
  check(!validate_projection3d(weights).ok(), "unnormalized weights accepted");
  auto lod = p;
  lod.lods[1].minimum_screen_coverage_per_million = 900'000;
  check(!validate_projection3d(lod).ok(), "nonmonotonic LOD accepted");
  auto open = p;
  open.meshes[0].triangle_indices.pop_back();
  check(!validate_projection3d(open).ok(), "malformed index buffer accepted");
  auto collision = p;
  collision.meshes[2].require_closed_manifold = false;
  check(!validate_projection3d(collision).ok(), "open collision mesh accepted");
  std::cout << "all gspl sprites 3D projection tests passed\n";
  return 0;
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
