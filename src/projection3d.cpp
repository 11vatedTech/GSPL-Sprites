#include "gspl_sprites/projection3d.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
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
long double square(long double value) { return value * value; }
bool unit_normal(const Normal3Ppm &value) {
  const auto length =
      std::sqrt(square(value.x) + square(value.y) + square(value.z));
  return length >= 990'000.0L && length <= 1'010'000.0L;
}
bool unit_quaternion(const std::array<std::int32_t, 4> &value) {
  const auto length = std::sqrt(square(value[0]) + square(value[1]) +
                                square(value[2]) + square(value[3]));
  return length >= 990'000.0L && length <= 1'010'000.0L;
}
bool degenerate(const Vertex3d &a, const Vertex3d &b, const Vertex3d &c) {
  const long double abx = static_cast<long double>(b.position.x) - a.position.x;
  const long double aby = static_cast<long double>(b.position.y) - a.position.y;
  const long double abz = static_cast<long double>(b.position.z) - a.position.z;
  const long double acx = static_cast<long double>(c.position.x) - a.position.x;
  const long double acy = static_cast<long double>(c.position.y) - a.position.y;
  const long double acz = static_cast<long double>(c.position.z) - a.position.z;
  const auto x = aby * acz - abz * acy;
  const auto y = abz * acx - abx * acz;
  const auto z = abx * acy - aby * acx;
  return x == 0 && y == 0 && z == 0;
}
std::uint64_t triangles(const Mesh3d &mesh) {
  return mesh.triangle_indices.size() / 3;
}
} // namespace

ValidationResult
validate_projection3d(const Projection3dDefinition &projection) {
  ValidationResult result;
  if (!stable_id(projection.id))
    add(result, "SPRITE_3D_ID_INVALID", "3D projection ID is invalid");
  if (projection.limits.maximum_vertices == 0 ||
      projection.limits.maximum_triangles == 0 ||
      projection.limits.maximum_meshes == 0 ||
      projection.limits.maximum_materials == 0 ||
      projection.limits.maximum_joints == 0 ||
      projection.limits.maximum_morph_targets == 0)
    add(result, "SPRITE_3D_LIMIT_INVALID",
        "3D projection limits must be positive");
  if (projection.meshes.empty() ||
      projection.meshes.size() > projection.limits.maximum_meshes ||
      projection.materials.size() > projection.limits.maximum_materials ||
      projection.morph_targets.size() > projection.limits.maximum_morph_targets)
    add(result, "SPRITE_3D_COLLECTION_LIMIT",
        "3D projection collection limit exceeded");
  std::set<std::string> material_ids;
  for (const auto &material : projection.materials) {
    if (!stable_id(material.id) || !material_ids.insert(material.id).second ||
        material.metallic_per_million > 1'000'000 ||
        material.roughness_per_million > 1'000'000 ||
        material.alpha_cutoff_per_million > 1'000'000 ||
        (material.base_color_texture_id &&
         !stable_id(*material.base_color_texture_id)) ||
        (material.normal_texture_id &&
         !stable_id(*material.normal_texture_id)) ||
        (material.metallic_roughness_texture_id &&
         !stable_id(*material.metallic_roughness_texture_id)))
      add(result, "SPRITE_3D_MATERIAL_INVALID",
          "PBR material is invalid or duplicate");
  }
  std::set<std::string> joint_ids;
  if (projection.skeleton) {
    if (!stable_id(projection.skeleton->id) ||
        projection.skeleton->joints.empty() ||
        projection.skeleton->joints.size() > projection.limits.maximum_joints)
      add(result, "SPRITE_3D_SKELETON_INVALID",
          "skeleton ID or joint count is invalid");
    std::size_t roots{};
    std::map<std::string, std::optional<std::string>, std::less<>> parents;
    for (const auto &joint : projection.skeleton->joints) {
      if (!stable_id(joint.id) || !joint_ids.insert(joint.id).second ||
          (joint.parent_id && !stable_id(*joint.parent_id)) ||
          !unit_quaternion(joint.rotation_xyzw_ppm))
        add(result, "SPRITE_3D_JOINT_INVALID", "joint is invalid or duplicate");
      if (!joint.parent_id)
        ++roots;
      parents.emplace(joint.id, joint.parent_id);
    }
    if (roots != 1)
      add(result, "SPRITE_3D_SKELETON_ROOT_INVALID",
          "skeleton must have exactly one root");
    for (const auto &[id, parent] : parents) {
      if (parent && !parents.contains(*parent)) {
        add(result, "SPRITE_3D_JOINT_PARENT_UNKNOWN",
            "joint parent does not exist");
        continue;
      }
      std::set<std::string> path;
      auto current = std::optional<std::string>{id};
      while (current) {
        if (!path.insert(*current).second) {
          add(result, "SPRITE_3D_SKELETON_CYCLE", "skeleton contains a cycle");
          break;
        }
        const auto found = parents.find(*current);
        current = found == parents.end() ? std::nullopt : found->second;
      }
    }
  }
  std::set<std::string> mesh_ids;
  std::map<std::string, std::uint64_t, std::less<>> mesh_triangles;
  std::map<std::string, MeshPurpose, std::less<>> mesh_purposes;
  std::uint64_t total_vertices{}, total_triangles{};
  for (const auto &mesh : projection.meshes) {
    if (!stable_id(mesh.id) || !mesh_ids.insert(mesh.id).second ||
        mesh.vertices.empty() || mesh.triangle_indices.empty() ||
        mesh.triangle_indices.size() % 3 != 0)
      add(result, "SPRITE_3D_MESH_INVALID", "mesh is malformed or duplicate");
    if ((mesh.purpose == MeshPurpose::render &&
         (!mesh.material_id || !material_ids.contains(*mesh.material_id))) ||
        (mesh.purpose == MeshPurpose::collision && mesh.material_id))
      add(result, "SPRITE_3D_MESH_MATERIAL_INVALID",
          "mesh material contract is invalid");
    total_vertices += mesh.vertices.size();
    total_triangles += triangles(mesh);
    mesh_triangles.emplace(mesh.id, triangles(mesh));
    mesh_purposes.emplace(mesh.id, mesh.purpose);
    if (mesh.purpose == MeshPurpose::collision && !mesh.require_closed_manifold)
      add(result, "SPRITE_3D_COLLISION_OPEN",
          "collision meshes must require closed-manifold validation");
    for (const auto &vertex : mesh.vertices) {
      if (!unit_normal(vertex.normal) || vertex.texture_coordinate.u < 0 ||
          vertex.texture_coordinate.u > 1'000'000 ||
          vertex.texture_coordinate.v < 0 ||
          vertex.texture_coordinate.v > 1'000'000 ||
          vertex.influences.size() > 4)
        add(result, "SPRITE_3D_VERTEX_INVALID",
            "vertex normal, UV, or influence count is invalid");
      std::set<std::string> influences;
      std::uint64_t weight{};
      for (const auto &influence : vertex.influences) {
        if (!joint_ids.contains(influence.joint_id) ||
            influence.weight_per_million == 0 ||
            !influences.insert(influence.joint_id).second)
          add(result, "SPRITE_3D_WEIGHT_INVALID",
              "skin influence is invalid or duplicate");
        weight += influence.weight_per_million;
      }
      if ((!vertex.influences.empty() && weight != 1'000'000) ||
          (projection.skeleton && mesh.purpose == MeshPurpose::render &&
           vertex.influences.empty()) ||
          (!projection.skeleton && !vertex.influences.empty()))
        add(result, "SPRITE_3D_WEIGHT_SUM_INVALID",
            "skin weights do not match skeleton contract");
    }
    std::map<std::pair<std::uint32_t, std::uint32_t>,
             std::pair<std::uint32_t, std::int32_t>>
        edges;
    for (std::size_t i = 0; i + 2 < mesh.triangle_indices.size(); i += 3) {
      const auto a = mesh.triangle_indices[i], b = mesh.triangle_indices[i + 1],
                 c = mesh.triangle_indices[i + 2];
      if (a >= mesh.vertices.size() || b >= mesh.vertices.size() ||
          c >= mesh.vertices.size()) {
        add(result, "SPRITE_3D_INDEX_INVALID",
            "triangle index is out of bounds");
        continue;
      }
      if (a == b || b == c || a == c ||
          degenerate(mesh.vertices[a], mesh.vertices[b], mesh.vertices[c]))
        add(result, "SPRITE_3D_TRIANGLE_DEGENERATE",
            "mesh contains a degenerate triangle");
      for (const auto [from, to] :
           {std::pair{a, b}, std::pair{b, c}, std::pair{c, a}}) {
        const auto key = std::minmax(from, to);
        auto &edge = edges[key];
        ++edge.first;
        edge.second += from < to ? 1 : -1;
      }
    }
    if (mesh.require_closed_manifold)
      for (const auto &[edge, usage] : edges)
        if (usage.first != 2 || usage.second != 0)
          add(result, "SPRITE_3D_MANIFOLD_INVALID",
              "closed mesh edge is open, non-manifold, or inconsistently "
              "wound");
  }
  if (total_vertices > projection.limits.maximum_vertices ||
      total_triangles > projection.limits.maximum_triangles)
    add(result, "SPRITE_3D_GEOMETRY_BUDGET",
        "3D geometry exceeds vertex or triangle budget");
  std::set<std::string> morph_ids;
  for (const auto &morph : projection.morph_targets) {
    const auto mesh =
        std::ranges::find(projection.meshes, morph.mesh_id, &Mesh3d::id);
    if (!stable_id(morph.id) || !morph_ids.insert(morph.id).second ||
        mesh == projection.meshes.end() ||
        (mesh != projection.meshes.end() &&
         mesh->purpose != MeshPurpose::render) ||
        (mesh != projection.meshes.end() &&
         morph.position_deltas.size() != mesh->vertices.size()))
      add(result, "SPRITE_3D_MORPH_INVALID",
          "morph target is invalid or has wrong vertex count");
  }
  std::set<std::uint32_t> levels;
  std::uint64_t prior_triangles = std::numeric_limits<std::uint64_t>::max();
  std::uint32_t prior_coverage = 1'000'001;
  auto lods = projection.lods;
  std::ranges::sort(lods, {}, &LodLevel3d::level);
  for (std::size_t i = 0; i < lods.size(); ++i) {
    const auto &lod = lods[i];
    const auto found = mesh_triangles.find(lod.mesh_id);
    if (lod.level != i || !levels.insert(lod.level).second ||
        found == mesh_triangles.end() ||
        (found != mesh_triangles.end() &&
         mesh_purposes.at(lod.mesh_id) != MeshPurpose::render) ||
        lod.minimum_screen_coverage_per_million > 1'000'000 ||
        found->second >= prior_triangles ||
        lod.minimum_screen_coverage_per_million >= prior_coverage)
      add(result, "SPRITE_3D_LOD_INVALID",
          "LOD levels must be contiguous with decreasing geometry and "
          "coverage");
    if (found != mesh_triangles.end())
      prior_triangles = found->second;
    prior_coverage = lod.minimum_screen_coverage_per_million;
  }
  return result;
}

std::string
canonicalize_projection3d(const Projection3dDefinition &projection) {
  const auto validation = validate_projection3d(projection);
  if (!validation.ok())
    throw std::invalid_argument(validation.diagnostics.front().message);
  auto materials = projection.materials;
  auto meshes = projection.meshes;
  auto morphs = projection.morph_targets;
  auto lods = projection.lods;
  std::ranges::sort(materials, {}, &Material3d::id);
  std::ranges::sort(meshes, {}, &Mesh3d::id);
  std::ranges::sort(morphs, {}, &MorphTarget3d::id);
  std::ranges::sort(lods, {}, &LodLevel3d::level);
  std::ostringstream out;
  out << "schema=gspl.projection3d/0.1\nid=" << projection.id << '\n';
  out << "limits=" << projection.limits.maximum_vertices << '|'
      << projection.limits.maximum_triangles << '|'
      << projection.limits.maximum_meshes << '|'
      << projection.limits.maximum_materials << '|'
      << projection.limits.maximum_joints << '|'
      << projection.limits.maximum_morph_targets << '\n';
  for (const auto &m : materials)
    out << "material=" << m.id << '|' << m.base_color_rgba << '|'
        << m.metallic_per_million << '|' << m.roughness_per_million << '|'
        << static_cast<int>(m.alpha_mode) << '|' << m.alpha_cutoff_per_million
        << '|' << (m.double_sided ? 1 : 0) << '|'
        << m.base_color_texture_id.value_or("") << '|'
        << m.normal_texture_id.value_or("") << '|'
        << m.metallic_roughness_texture_id.value_or("") << '\n';
  if (projection.skeleton) {
    auto joints = projection.skeleton->joints;
    std::ranges::sort(joints, {}, &Joint3d::id);
    out << "skeleton=" << projection.skeleton->id << '\n';
    for (const auto &j : joints)
      out << "joint=" << j.id << '|' << j.parent_id.value_or("") << '|'
          << j.translation.x << '|' << j.translation.y << '|' << j.translation.z
          << '|' << j.rotation_xyzw_ppm[0] << '|' << j.rotation_xyzw_ppm[1]
          << '|' << j.rotation_xyzw_ppm[2] << '|' << j.rotation_xyzw_ppm[3]
          << '\n';
  }
  for (const auto &mesh : meshes) {
    out << "mesh=" << mesh.id << '|' << static_cast<int>(mesh.purpose) << '|'
        << mesh.material_id.value_or("") << '|'
        << (mesh.require_closed_manifold ? 1 : 0) << '\n';
    for (const auto &v : mesh.vertices) {
      out << "vertex=" << mesh.id << '|' << v.position.x << '|' << v.position.y
          << '|' << v.position.z << '|' << v.normal.x << '|' << v.normal.y
          << '|' << v.normal.z << '|' << v.texture_coordinate.u << '|'
          << v.texture_coordinate.v;
      auto influences = v.influences;
      std::ranges::sort(influences, {}, &JointInfluence3d::joint_id);
      for (const auto &w : influences)
        out << '|' << w.joint_id << ':' << w.weight_per_million;
      out << '\n';
    }
    out << "indices=" << mesh.id;
    for (const auto index : mesh.triangle_indices)
      out << '|' << index;
    out << '\n';
  }
  for (const auto &morph : morphs) {
    out << "morph=" << morph.id << '|' << morph.mesh_id;
    for (const auto &d : morph.position_deltas)
      out << '|' << d.x << ':' << d.y << ':' << d.z;
    out << '\n';
  }
  for (const auto &lod : lods)
    out << "lod=" << lod.level << '|' << lod.mesh_id << '|'
        << lod.minimum_screen_coverage_per_million << '\n';
  return out.str();
}
} // namespace gspl::sprites
