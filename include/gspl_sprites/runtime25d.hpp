#pragma once

#include "gspl_sprites/projection25d.hpp"

namespace gspl::sprites {

struct Camera25dState {
  std::int64_t x_microunits{};
  std::int64_t y_microunits{};
  std::uint32_t yaw_millidegrees{};
};

struct PlaneInstance25d {
  std::string plane_id;
  std::string visual_asset_id;
  std::optional<std::string> normal_asset_id;
  std::optional<std::string> material_asset_id;
  std::optional<std::string> rig_node_id;
  std::int32_t depth_millimeters{};
  std::int64_t offset_x_microunits{};
  std::int64_t offset_y_microunits{};
  std::int32_t camera_deformation_per_million{};
  bool receives_lighting{};
};

struct GeometryInstance25d {
  std::string component_id;
  std::string geometry_asset_id;
  std::string attachment_plane_id;
  std::string socket_id;
  std::int64_t offset_x_microunits{};
  std::int64_t offset_y_microunits{};
  std::int32_t camera_deformation_per_million{};
};

struct Projection25dFrame {
  std::string view_id;
  std::uint32_t view_yaw_millidegrees{};
  std::vector<PlaneInstance25d> planes;
  std::vector<GeometryInstance25d> geometry;
  std::vector<DepthCollisionVolume> collisions;
};

[[nodiscard]] Projection25dFrame
evaluate_projection25d(const Projection25dDefinition &projection,
                       const Camera25dState &camera);

} // namespace gspl::sprites
