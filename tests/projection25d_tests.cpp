#include "gspl_sprites/projection25d.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace gspl::sprites;
namespace {
void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
Projection25dDefinition fixture() {
  return {
      "entity.fox.25d",
      RepresentationKind::hybrid,
      BillboardMode::discrete_multi_angle,
      {{"body", "visual.body", "normal.body", "material.body", 0, 250'000,
        50'000, true, {}},
       {"tail", "visual.tail", "normal.tail", {}, -40, 400'000, 100'000, true, {}}},
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
  check(validate_projection25d(projection).ok(),
        "valid 2.5D projection rejected");
  auto reordered = projection;
  std::reverse(reordered.planes.begin(), reordered.planes.end());
  for (auto &view : reordered.views)
    std::reverse(view.planes.begin(), view.planes.end());
  check(canonicalize_projection25d(projection) ==
            canonicalize_projection25d(reordered),
        "2.5D canonicalization depends on declaration order");
  check(select_projection25d_view(projection, 80'000).id == "east",
        "nearest angular view is incorrect");
  const auto offset =
      calculate_parallax_offset(projection.planes[0], 2'000'000, -1'000'000);
  check(offset.x_microunits == 500'000 && offset.y_microunits == -250'000,
        "parallax offset is incorrect");
  auto missing = projection;
  missing.views[0].planes.pop_back();
  check(!validate_projection25d(missing).ok(), "incomplete view accepted");
  auto unlit = projection;
  unlit.planes[0].normal_asset_id.reset();
  check(!validate_projection25d(unlit).ok(),
        "lit plane without normal map accepted");
  auto flat = projection;
  flat.representation = RepresentationKind::two_point_five_d;
  check(!validate_projection25d(flat).ok(),
        "selective geometry on non-hybrid projection accepted");
  auto generated_chain = projection;
  generated_chain.views.push_back(
      {"west", 270'000, true, "east", generated_chain.views[0].planes});
  check(!validate_projection25d(generated_chain).ok(),
        "generated view derived from another generated view accepted");
  std::cout << "all gspl sprites 2.5D projection tests passed\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
