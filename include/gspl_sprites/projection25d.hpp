#pragma once

#include "gspl_sprites/common.hpp"

#include <optional>
#include <span>

namespace gspl::sprites {

enum class RepresentationKind { two_d, two_point_five_d, three_d, hybrid };
enum class BillboardMode { fixed_axis, camera_facing, discrete_multi_angle };

struct DepthPlaneDefinition {
  std::string id;
  std::string visual_asset_id;
  std::optional<std::string> normal_asset_id;
  std::optional<std::string> material_asset_id;
  std::int32_t depth_millimeters{};
  std::int32_t parallax_per_million{};
  std::uint32_t camera_deformation_per_million{};
  bool receives_lighting{};
  std::optional<std::string> rig_node_id;
};

struct ViewPlaneProjection {
  std::string plane_id;
  bool visible{true};
  std::optional<std::string> visual_asset_override;
  std::optional<std::string> normal_asset_override;
};

struct AngularViewDefinition {
  std::string id;
  std::uint32_t yaw_millidegrees{};
  bool generated{};
  std::optional<std::string> source_view_id;
  std::vector<ViewPlaneProjection> planes;
};

struct HybridGeometryComponent {
  std::string id;
  std::string geometry_asset_id;
  std::string attachment_plane_id;
  std::string socket_id;
};

struct DepthCollisionVolume {
  std::string id;
  std::string plane_id;
  std::int32_t minimum_x_microunits{};
  std::int32_t minimum_y_microunits{};
  std::int32_t maximum_x_microunits{};
  std::int32_t maximum_y_microunits{};
  std::int32_t near_depth_millimeters{};
  std::int32_t far_depth_millimeters{};
};

struct Projection25dDefinition {
  std::string id;
  RepresentationKind representation{RepresentationKind::two_point_five_d};
  BillboardMode billboard{BillboardMode::discrete_multi_angle};
  std::vector<DepthPlaneDefinition> planes;
  std::vector<AngularViewDefinition> views;
  std::vector<HybridGeometryComponent> geometry;
  std::vector<DepthCollisionVolume> collisions;
};

struct ParallaxOffset {
  std::int64_t x_microunits{};
  std::int64_t y_microunits{};
};

[[nodiscard]] ValidationResult
validate_projection25d(const Projection25dDefinition &projection);
[[nodiscard]] std::string
canonicalize_projection25d(const Projection25dDefinition &projection);
[[nodiscard]] const AngularViewDefinition &
select_projection25d_view(const Projection25dDefinition &projection,
                          std::uint32_t yaw_millidegrees);
[[nodiscard]] ParallaxOffset
calculate_parallax_offset(const DepthPlaneDefinition &plane,
                          std::int64_t camera_x_microunits,
                          std::int64_t camera_y_microunits);

} // namespace gspl::sprites
