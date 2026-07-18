#pragma once

#include "gspl_sprites/common.hpp"

#include <array>
#include <map>
#include <optional>

namespace gspl::sprites {

struct Vector3Micrometers {
  std::int64_t x{};
  std::int64_t y{};
  std::int64_t z{};
};
struct Normal3Ppm {
  std::int32_t x{};
  std::int32_t y{};
  std::int32_t z{1'000'000};
};
struct TextureCoordinatePpm {
  std::int32_t u{};
  std::int32_t v{};
};
struct JointInfluence3d {
  std::string joint_id;
  std::uint32_t weight_per_million{};
};

struct Vertex3d {
  Vector3Micrometers position;
  Normal3Ppm normal;
  TextureCoordinatePpm texture_coordinate;
  std::vector<JointInfluence3d> influences;
};

enum class MeshPurpose { render, collision };
struct Mesh3d {
  std::string id;
  MeshPurpose purpose{MeshPurpose::render};
  std::optional<std::string> material_id;
  bool require_closed_manifold{};
  std::vector<Vertex3d> vertices;
  std::vector<std::uint32_t> triangle_indices;
};

enum class MaterialAlphaMode { opaque, mask, blend };
struct Material3d {
  std::string id;
  std::uint32_t base_color_rgba{0xffffffffU};
  std::uint32_t metallic_per_million{};
  std::uint32_t roughness_per_million{1'000'000};
  MaterialAlphaMode alpha_mode{MaterialAlphaMode::opaque};
  std::uint32_t alpha_cutoff_per_million{500'000};
  bool double_sided{};
  std::optional<std::string> base_color_texture_id;
  std::optional<std::string> normal_texture_id;
  std::optional<std::string> metallic_roughness_texture_id;
};

struct Joint3d {
  std::string id;
  std::optional<std::string> parent_id;
  Vector3Micrometers translation;
  std::array<std::int32_t, 4> rotation_xyzw_ppm{0, 0, 0, 1'000'000};
};
struct Skeleton3d {
  std::string id;
  std::vector<Joint3d> joints;
};

struct MorphTarget3d {
  std::string id;
  std::string mesh_id;
  std::vector<Vector3Micrometers> position_deltas;
};

struct LodLevel3d {
  std::uint32_t level{};
  std::string mesh_id;
  std::uint32_t minimum_screen_coverage_per_million{};
};

struct Projection3dLimits {
  std::uint64_t maximum_vertices{4'000'000};
  std::uint64_t maximum_triangles{8'000'000};
  std::uint32_t maximum_meshes{4'096};
  std::uint32_t maximum_materials{4'096};
  std::uint32_t maximum_joints{1'024};
  std::uint32_t maximum_morph_targets{4'096};
};

struct Projection3dDefinition {
  std::string id;
  std::vector<Material3d> materials;
  std::vector<Mesh3d> meshes;
  std::optional<Skeleton3d> skeleton;
  std::vector<MorphTarget3d> morph_targets;
  std::vector<LodLevel3d> lods;
  Projection3dLimits limits;
};

[[nodiscard]] ValidationResult
validate_projection3d(const Projection3dDefinition &projection);
[[nodiscard]] std::string
canonicalize_projection3d(const Projection3dDefinition &projection);

} // namespace gspl::sprites
