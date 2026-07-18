#include "gspl_sprites/animation3d.hpp"

#include <algorithm>
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
  p.id = "animated";
  p.materials = {{"mat"}};
  p.skeleton = Skeleton3d{"source", {{"root", {}}, {"tail", "root"}}};
  Vertex3d v{{0, 0, 0}, {0, 0, 1'000'000}, {0, 0}, {{"root", 1'000'000}}};
  p.meshes = {
      {"mesh", MeshPurpose::render, "mat", false, {v, v, v}, {0, 1, 2}}};
  p.meshes[0].vertices[1].position.x = 1;
  p.meshes[0].vertices[2].position.y = 1;
  p.morph_targets = {{"blink", "mesh", std::vector<Vector3Micrometers>(3)}};
  return p;
}
} // namespace
int main() try {
  const auto p = projection();
  AnimationClip3d clip{"idle",
                       60,
                       10,
                       true,
                       {{"root",
                         {{0, {}},
                          {10,
                           {{100, 0, 0},
                            {0, 0, 0, 1'000'000},
                            {1'000'000, 1'000'000, 1'000'000}}}}}},
                       {{"blink", {{0, 0}, {5, 1'000'000}, {10, 0}}}},
                       {{"loop", 10}}};
  check(validate_animation_clip3d(clip, p).ok(), "valid 3D animation rejected");
  auto reordered = clip;
  std::reverse(reordered.events.begin(), reordered.events.end());
  check(canonicalize_animation_clip3d(clip, p) ==
            canonicalize_animation_clip3d(reordered, p),
        "animation canonicalization depends on set order");
  auto bad = clip;
  bad.joint_tracks[0].keys[1].tick = 0;
  check(!validate_animation_clip3d(bad, p).ok(), "duplicate key time accepted");
  Skeleton3d target{"target", {{"base", {}}, {"appendage", "base"}}};
  RetargetMap3d map{"fox-to-drake",
                    "source",
                    "target",
                    1'000'000,
                    {{"root", "base"}, {"tail", "appendage"}}};
  check(validate_retarget_map3d(map, *p.skeleton, target).ok(),
        "valid retarget map rejected");
  auto broken = map;
  broken.joints[1].target_joint_id = "base";
  check(!validate_retarget_map3d(broken, *p.skeleton, target).ok(),
        "many-to-one retarget accepted");
  auto cyclic_target = target;
  cyclic_target.joints[0].parent_id = "appendage";
  check(!validate_retarget_map3d(map, *p.skeleton, cyclic_target).ok(),
        "cyclic target skeleton accepted");
  std::cout << "all gspl sprites 3D animation tests passed\n";
  return 0;
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
