#include "gspl_sprites/runtime25d.hpp"

#include <iostream>
#include <stdexcept>

using namespace gspl::sprites;
namespace {
void check(bool v, const char *m) {
  if (!v)
    throw std::runtime_error(m);
}
Projection25dDefinition fixture() {
  return {"entity.fox.25d",
          RepresentationKind::hybrid,
          BillboardMode::discrete_multi_angle,
          {{"body", "visual.body", "normal.body", "material.body", 0, 250'000,
            50'000, true, "rig.body"},
           {"tail",
            "visual.tail",
            "normal.tail",
            {},
            -40,
            400'000,
            100'000,
            true,
            "rig.tail"}},
          {{"south",
            0,
            false,
            {},
            {{"body", true, {}, {}}, {"tail", true, {}, {}}}},
           {"east",
            90'000,
            true,
            "south",
            {{"body", true, "visual.body.east", {}},
             {"tail", true, "visual.tail.east", {}}}}},
          {{"blade", "mesh.blade.glb", "body", "socket.hand"}},
          {{"body.hit", "body", -500'000, -1'000'000, 500'000, 1'000'000, -100,
            100}}};
}
} // namespace
int main() try {
  const auto projection = fixture();
  const auto frame =
      evaluate_projection25d(projection, {2'000'000, -1'000'000, 80'000});
  check(frame.view_id == "east" && frame.planes.size() == 2 &&
            frame.planes[0].plane_id == "tail" &&
            frame.planes[1].visual_asset_id == "visual.body.east" &&
            frame.planes[1].offset_x_microunits == 500'000 &&
            frame.planes[1].offset_y_microunits == -250'000 &&
            frame.planes[1].camera_deformation_per_million == -2'777,
        "2.5D runtime frame is incorrect");
  check(frame.geometry.size() == 1 &&
            frame.geometry[0].offset_x_microunits == 500'000 &&
            frame.collisions.size() == 1 &&
            frame.collisions[0].minimum_x_microunits == -500'000,
        "2.5D runtime authority projection is incorrect");
  auto hidden = projection;
  hidden.views[1].planes[0].visible = false;
  const auto hidden_frame = evaluate_projection25d(hidden, {0, 0, 90'000});
  check(hidden_frame.planes.size() == 1 && hidden_frame.geometry.empty(),
        "hidden attachment plane still emitted geometry");
  auto fixed = projection;
  fixed.billboard = BillboardMode::fixed_axis;
  check(!validate_projection25d(fixed).ok(),
        "fixed-axis projection accepted multiple views");

  auto camera_facing = projection;
  camera_facing.billboard = BillboardMode::camera_facing;
  camera_facing.views.erase(camera_facing.views.begin() + 1);
  const auto facing_frame =
      evaluate_projection25d(camera_facing, {0, 0, 180'000});
  check(facing_frame.planes[0].camera_deformation_per_million == 0,
        "camera-facing projection applied angular deformation");

  bool rejected_camera = false;
  try {
    static_cast<void>(evaluate_projection25d(projection, {0, 0, 360'000}));
  } catch (const std::invalid_argument &) {
    rejected_camera = true;
  }
  check(rejected_camera, "2.5D runtime accepted an out-of-range camera yaw");
  std::cout << "all gspl sprites 2.5D runtime tests passed\n";
  return 0;
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
