#include "gspl_sprites/projection25d.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace gspl::sprites {
namespace {
bool stable_id(std::string_view value) {
  return !value.empty() && value.size() <= 128 &&
         std::ranges::all_of(value, [](unsigned char c) {
           return std::isalnum(c) || c == '.' || c == '_' || c == '-';
         });
}
void add(ValidationResult &result, std::string code, std::string message) {
  result.diagnostics.push_back({std::move(code), std::move(message)});
}
std::uint32_t angular_distance(std::uint32_t left, std::uint32_t right) {
  const auto direct = left > right ? left - right : right - left;
  return std::min(direct, 360'000U - direct);
}
} // namespace

ValidationResult
validate_projection25d(const Projection25dDefinition &projection) {
  ValidationResult result;
  if (!stable_id(projection.id))
    add(result, "SPRITE_25D_ID_INVALID", "2.5D projection ID is invalid");
  if (projection.representation != RepresentationKind::two_point_five_d &&
      projection.representation != RepresentationKind::hybrid)
    add(result, "SPRITE_25D_KIND_INVALID",
        "2.5D projection must be 2.5D or hybrid");
  if (projection.planes.empty() || projection.planes.size() > 1'024 ||
      projection.views.empty() || projection.views.size() > 360 ||
      projection.geometry.size() > 1'024 ||
      projection.collisions.size() > 4'096)
    add(result, "SPRITE_25D_LIMIT_INVALID",
        "2.5D projection collection limits are invalid");
  if (projection.billboard == BillboardMode::discrete_multi_angle &&
      projection.views.size() < 2)
    add(result, "SPRITE_25D_MULTI_ANGLE_REQUIRED",
        "multi-angle billboard requires at least two views");
  if (projection.representation != RepresentationKind::hybrid &&
      !projection.geometry.empty())
    add(result, "SPRITE_25D_GEOMETRY_KIND_INVALID",
        "selective geometry requires hybrid representation");
  std::set<std::string> plane_ids;
  for (const auto &plane : projection.planes) {
    if (!stable_id(plane.id) || !stable_id(plane.visual_asset_id) ||
        (plane.normal_asset_id && !stable_id(*plane.normal_asset_id)) ||
        (plane.material_asset_id && !stable_id(*plane.material_asset_id)) ||
        (plane.rig_node_id && !stable_id(*plane.rig_node_id)) ||
        !plane_ids.insert(plane.id).second ||
        plane.parallax_per_million < -4'000'000 ||
        plane.parallax_per_million > 4'000'000 ||
        plane.camera_deformation_per_million > 1'000'000)
      add(result, "SPRITE_25D_PLANE_INVALID",
          "depth plane is malformed, duplicate, or out of range");
    if (plane.receives_lighting && !plane.normal_asset_id)
      add(result, "SPRITE_25D_NORMAL_REQUIRED",
          "a lit depth plane requires a normal map");
  }
  std::set<std::string> view_ids;
  std::set<std::uint32_t> yaws;
  std::map<std::string, bool, std::less<>> generated_views;
  for (const auto &view : projection.views) {
    generated_views.emplace(view.id, view.generated);
    if (!stable_id(view.id) || view.yaw_millidegrees >= 360'000 ||
        !view_ids.insert(view.id).second ||
        !yaws.insert(view.yaw_millidegrees).second ||
        view.planes.size() != projection.planes.size())
      add(result, "SPRITE_25D_VIEW_INVALID",
          "angular view is malformed, duplicate, or incomplete");
    if (view.generated != view.source_view_id.has_value() ||
        (view.source_view_id &&
         (!stable_id(*view.source_view_id) || *view.source_view_id == view.id)))
      add(result, "SPRITE_25D_VIEW_PROVENANCE_INVALID",
          "generated view source is invalid");
    std::set<std::string> covered;
    for (const auto &item : view.planes) {
      if (!plane_ids.contains(item.plane_id) ||
          !covered.insert(item.plane_id).second ||
          (item.visual_asset_override &&
           !stable_id(*item.visual_asset_override)) ||
          (item.normal_asset_override &&
           !stable_id(*item.normal_asset_override)))
        add(result, "SPRITE_25D_VIEW_PLANE_INVALID",
            "view plane reference is invalid or duplicate");
    }
    if (covered != plane_ids)
      add(result, "SPRITE_25D_VIEW_COVERAGE_INVALID",
          "view must explicitly cover every depth plane");
  }
  for (const auto &view : projection.views)
    if (view.source_view_id && (!view_ids.contains(*view.source_view_id) ||
                                generated_views.at(*view.source_view_id)))
      add(result, "SPRITE_25D_VIEW_SOURCE_UNKNOWN",
          "generated view source must reference an authored view");
  std::set<std::string> component_ids;
  for (const auto &component : projection.geometry)
    if (!stable_id(component.id) || !stable_id(component.geometry_asset_id) ||
        !plane_ids.contains(component.attachment_plane_id) ||
        !stable_id(component.socket_id) ||
        !component_ids.insert(component.id).second)
      add(result, "SPRITE_25D_GEOMETRY_INVALID",
          "hybrid geometry component is invalid");
  std::set<std::string> collision_ids;
  for (const auto &collision : projection.collisions)
    if (!stable_id(collision.id) ||
        !collision_ids.insert(collision.id).second ||
        !plane_ids.contains(collision.plane_id) ||
        collision.minimum_x_microunits >= collision.maximum_x_microunits ||
        collision.minimum_y_microunits >= collision.maximum_y_microunits ||
        collision.near_depth_millimeters >= collision.far_depth_millimeters)
      add(result, "SPRITE_25D_COLLISION_INVALID",
          "depth collision volume is invalid");
  return result;
}

std::string
canonicalize_projection25d(const Projection25dDefinition &projection) {
  const auto validation = validate_projection25d(projection);
  if (!validation.ok())
    throw std::invalid_argument(validation.diagnostics.front().message);
  auto planes = projection.planes;
  auto views = projection.views;
  auto geometry = projection.geometry;
  auto collisions = projection.collisions;
  std::ranges::sort(planes, {}, &DepthPlaneDefinition::id);
  std::ranges::sort(views, {}, &AngularViewDefinition::id);
  std::ranges::sort(geometry, {}, &HybridGeometryComponent::id);
  std::ranges::sort(collisions, {}, &DepthCollisionVolume::id);
  std::ostringstream out;
  out << "schema=gspl.projection25d/0.1\nid=" << projection.id
      << "\nrepresentation=" << static_cast<int>(projection.representation)
      << "\nbillboard=" << static_cast<int>(projection.billboard) << '\n';
  for (const auto &p : planes)
    out << "plane=" << p.id << '|' << p.visual_asset_id << '|'
        << p.normal_asset_id.value_or("") << '|'
        << p.material_asset_id.value_or("") << '|' << p.depth_millimeters << '|'
        << p.parallax_per_million << '|' << p.camera_deformation_per_million
        << '|' << (p.receives_lighting ? 1 : 0) << '|'
        << p.rig_node_id.value_or("") << '\n';
  for (auto &v : views) {
    std::ranges::sort(v.planes, {}, &ViewPlaneProjection::plane_id);
    out << "view=" << v.id << '|' << v.yaw_millidegrees << '|'
        << (v.generated ? 1 : 0) << '|' << v.source_view_id.value_or("")
        << '\n';
    for (const auto &p : v.planes)
      out << "view_plane=" << v.id << '|' << p.plane_id << '|'
          << (p.visible ? 1 : 0) << '|' << p.visual_asset_override.value_or("")
          << '|' << p.normal_asset_override.value_or("") << '\n';
  }
  for (const auto &g : geometry)
    out << "geometry=" << g.id << '|' << g.geometry_asset_id << '|'
        << g.attachment_plane_id << '|' << g.socket_id << '\n';
  for (const auto &c : collisions)
    out << "collision=" << c.id << '|' << c.plane_id << '|'
        << c.minimum_x_microunits << '|' << c.minimum_y_microunits << '|'
        << c.maximum_x_microunits << '|' << c.maximum_y_microunits << '|'
        << c.near_depth_millimeters << '|' << c.far_depth_millimeters << '\n';
  return out.str();
}

const AngularViewDefinition &
select_projection25d_view(const Projection25dDefinition &projection,
                          std::uint32_t yaw_millidegrees) {
  const auto validation = validate_projection25d(projection);
  if (!validation.ok() || yaw_millidegrees >= 360'000)
    throw std::invalid_argument("invalid 2.5D view selection input");
  return *std::ranges::min_element(projection.views, [&](const auto &left,
                                                         const auto &right) {
    return std::tuple{angular_distance(left.yaw_millidegrees, yaw_millidegrees),
                      left.id} <
           std::tuple{
               angular_distance(right.yaw_millidegrees, yaw_millidegrees),
               right.id};
  });
}

ParallaxOffset calculate_parallax_offset(const DepthPlaneDefinition &plane,
                                         std::int64_t camera_x_microunits,
                                         std::int64_t camera_y_microunits) {
  if (plane.parallax_per_million < -4'000'000 ||
      plane.parallax_per_million > 4'000'000)
    throw std::invalid_argument("parallax factor is out of range");
  constexpr auto scale = 1'000'000LL;
  const auto factor = static_cast<std::int64_t>(plane.parallax_per_million);
  const auto magnitude = std::max<std::int64_t>(1, std::abs(factor));
  const auto limit = std::numeric_limits<std::int64_t>::max() / magnitude;
  if (camera_x_microunits > limit || camera_x_microunits < -limit ||
      camera_y_microunits > limit || camera_y_microunits < -limit)
    throw std::overflow_error("parallax calculation overflow");
  return {camera_x_microunits * factor / scale,
          camera_y_microunits * factor / scale};
}
} // namespace gspl::sprites
