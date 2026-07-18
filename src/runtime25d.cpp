#include "gspl_sprites/runtime25d.hpp"

#include <algorithm>
#include <map>
#include <stdexcept>
#include <tuple>

namespace gspl::sprites {
namespace {
std::int32_t signed_camera_deformation(const DepthPlaneDefinition &plane,
                                       std::uint32_t camera_yaw,
                                       std::uint32_t view_yaw,
                                       BillboardMode mode) {
  if (mode == BillboardMode::camera_facing)
    return 0;
  std::int64_t difference = static_cast<std::int64_t>(camera_yaw) - view_yaw;
  if (difference > 180'000)
    difference -= 360'000;
  if (difference < -180'000)
    difference += 360'000;
  return static_cast<std::int32_t>(
      difference * plane.camera_deformation_per_million / 180'000);
}
} // namespace

Projection25dFrame
evaluate_projection25d(const Projection25dDefinition &projection,
                       const Camera25dState &camera) {
  const auto validation = validate_projection25d(projection);
  if (!validation.ok())
    throw std::invalid_argument(validation.diagnostics.front().message);
  if (camera.yaw_millidegrees >= 360'000)
    throw std::invalid_argument("2.5D camera yaw is out of range");
  const auto &view =
      projection.billboard == BillboardMode::discrete_multi_angle
          ? select_projection25d_view(projection, camera.yaw_millidegrees)
          : projection.views.front();
  std::map<std::string, const ViewPlaneProjection *, std::less<>> view_planes;
  for (const auto &item : view.planes)
    view_planes.emplace(item.plane_id, &item);
  Projection25dFrame frame;
  frame.view_id = view.id;
  frame.view_yaw_millidegrees = view.yaw_millidegrees;
  std::map<std::string, const PlaneInstance25d *, std::less<>> instances;
  for (const auto &plane : projection.planes) {
    const auto &selected = *view_planes.at(plane.id);
    if (!selected.visible)
      continue;
    const auto offset = calculate_parallax_offset(plane, camera.x_microunits,
                                                  camera.y_microunits);
    frame.planes.push_back(
        {plane.id,
         selected.visual_asset_override.value_or(plane.visual_asset_id),
         selected.normal_asset_override ? selected.normal_asset_override
                                        : plane.normal_asset_id,
         plane.material_asset_id, plane.rig_node_id, plane.depth_millimeters,
         offset.x_microunits, offset.y_microunits,
         signed_camera_deformation(plane, camera.yaw_millidegrees,
                                   view.yaw_millidegrees, projection.billboard),
         plane.receives_lighting});
  }
  std::ranges::sort(frame.planes, [](const auto &left, const auto &right) {
    return std::tuple{left.depth_millimeters, left.plane_id} <
           std::tuple{right.depth_millimeters, right.plane_id};
  });
  for (const auto &plane : frame.planes)
    instances.emplace(plane.plane_id, &plane);
  auto geometry = projection.geometry;
  std::ranges::sort(geometry, {}, &HybridGeometryComponent::id);
  for (const auto &component : geometry) {
    const auto found = instances.find(component.attachment_plane_id);
    if (found == instances.end())
      continue;
    const auto &plane = *found->second;
    frame.geometry.push_back({component.id, component.geometry_asset_id,
                              component.attachment_plane_id,
                              component.socket_id, plane.offset_x_microunits,
                              plane.offset_y_microunits,
                              plane.camera_deformation_per_million});
  }
  frame.collisions = projection.collisions;
  std::ranges::sort(frame.collisions, {}, &DepthCollisionVolume::id);
  return frame;
}
} // namespace gspl::sprites
