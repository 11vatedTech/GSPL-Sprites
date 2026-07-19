#include "gspl_sprites/gltf_export.hpp"
#include "gspl_sprites/gltf_verify.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace gspl::sprites {
namespace {
struct View {
  std::uint32_t offset{}, length{};
  std::optional<std::uint32_t> target;
  std::optional<std::uint32_t> byte_stride;
};
struct Accessor {
  std::uint32_t view{}, component{}, count{};
  std::string type;
  std::vector<double> minimum, maximum;
};
struct Primitive {
  std::uint32_t position{}, normal{}, indices{};
  std::optional<std::uint32_t> uv;
  std::optional<std::uint32_t> tangent;
  std::optional<std::uint32_t> joints, weights;
  std::vector<std::uint32_t> morphs;
  std::vector<std::string> morph_names;
  std::optional<std::uint32_t> material;
};
struct AnimationSamplerGlb {
  std::uint32_t input{}, output{};
};
struct AnimationChannelGlb {
  std::uint32_t sampler{}, node{};
  std::string path;
};
struct AnimationGlb {
  std::string id;
  bool looping{};
  std::vector<AnimationEvent3d> events;
  std::vector<AnimationSamplerGlb> samplers;
  std::vector<AnimationChannelGlb> channels;
};
void align4(std::vector<std::byte> &bytes) {
  while (bytes.size() % 4)
    bytes.push_back(std::byte{});
}
void u32(std::vector<std::byte> &out, std::uint32_t value) {
  for (int i = 0; i < 4; ++i)
    out.push_back(static_cast<std::byte>((value >> (i * 8)) & 0xff));
}
void u16(std::vector<std::byte> &out, std::uint16_t value) {
  out.push_back(static_cast<std::byte>(value & 0xff));
  out.push_back(static_cast<std::byte>((value >> 8) & 0xff));
}
void f32(std::vector<std::byte> &out, float value) {
  u32(out, std::bit_cast<std::uint32_t>(value));
}
std::string number(double value) {
  std::ostringstream out;
  out.precision(std::numeric_limits<double>::max_digits10);
  out << value;
  return out.str();
}
std::string array(std::span<const double> values) {
  std::string out = "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i)
      out += ',';
    out += number(values[i]);
  }
  return out + "]";
}
template <std::size_t N>
std::string array(const std::array<double, N> &values) {
  return array(std::span<const double>{values});
}
std::uint32_t add_view(std::vector<View> &views, std::vector<std::byte> &bin,
                       std::span<const std::byte> data,
                       std::optional<std::uint32_t> target = {},
                       std::optional<std::uint32_t> byte_stride = {}) {
  align4(bin);
  if (bin.size() > std::numeric_limits<std::uint32_t>::max() - data.size())
    throw std::length_error("GLB binary exceeds 32-bit range");
  const auto offset = static_cast<std::uint32_t>(bin.size());
  bin.insert(bin.end(), data.begin(), data.end());
  views.push_back(
      {offset, static_cast<std::uint32_t>(data.size()), target, byte_stride});
  return static_cast<std::uint32_t>(views.size() - 1);
}
template <class Writer>
std::uint32_t numeric_view(std::vector<View> &views,
                           std::vector<std::byte> &bin, std::size_t count,
                           Writer writer,
                           std::optional<std::uint32_t> target = {},
                           std::optional<std::uint32_t> byte_stride = {}) {
  std::vector<std::byte> data;
  data.reserve(count * 4);
  writer(data);
  return add_view(views, bin, data, target, byte_stride);
}
std::uint32_t accessor(std::vector<Accessor> &values, std::uint32_t view,
                       std::uint32_t component, std::uint32_t count,
                       std::string type, std::vector<double> minimum = {},
                       std::vector<double> maximum = {}) {
  values.push_back({view, component, count, std::move(type), std::move(minimum),
                    std::move(maximum)});
  return static_cast<std::uint32_t>(values.size() - 1);
}
std::string alpha(MaterialAlphaMode mode) {
  switch (mode) {
  case MaterialAlphaMode::opaque:
    return "OPAQUE";
  case MaterialAlphaMode::mask:
    return "MASK";
  case MaterialAlphaMode::blend:
    return "BLEND";
  }
  throw std::logic_error("invalid alpha mode");
}
struct Matrix {
  std::array<double, 16> v{};
};
Matrix multiply(const Matrix &a, const Matrix &b) {
  Matrix r;
  for (int c = 0; c < 4; ++c)
    for (int row = 0; row < 4; ++row)
      for (int k = 0; k < 4; ++k)
        r.v[c * 4 + row] += a.v[k * 4 + row] * b.v[c * 4 + k];
  return r;
}
Matrix local(const Joint3d &j) {
  const double x = j.rotation_xyzw_ppm[0] / 1e6,
               y = j.rotation_xyzw_ppm[1] / 1e6,
               z = j.rotation_xyzw_ppm[2] / 1e6,
               w = j.rotation_xyzw_ppm[3] / 1e6;
  Matrix m{{1 - 2 * y * y - 2 * z * z, 2 * x * y + 2 * w * z,
            2 * x * z - 2 * w * y, 0, 2 * x * y - 2 * w * z,
            1 - 2 * x * x - 2 * z * z, 2 * y * z + 2 * w * x, 0,
            2 * x * z + 2 * w * y, 2 * y * z - 2 * w * x,
            1 - 2 * x * x - 2 * y * y, 0, j.translation.x / 1e6,
            j.translation.y / 1e6, j.translation.z / 1e6, 1}};
  return m;
}
Matrix inverse_rigid(const Matrix &m) {
  Matrix r{{m.v[0], m.v[4], m.v[8], 0, m.v[1], m.v[5], m.v[9], 0, m.v[2],
            m.v[6], m.v[10], 0, 0, 0, 0, 1}};
  r.v[12] = -(r.v[0] * m.v[12] + r.v[4] * m.v[13] + r.v[8] * m.v[14]);
  r.v[13] = -(r.v[1] * m.v[12] + r.v[5] * m.v[13] + r.v[9] * m.v[14]);
  r.v[14] = -(r.v[2] * m.v[12] + r.v[6] * m.v[13] + r.v[10] * m.v[14]);
  return r;
}
std::uint32_t sample_morph(const MorphTrack3d &track, std::uint32_t tick) {
  const auto upper =
      std::ranges::lower_bound(track.keys, tick, {}, &MorphKeyframe3d::tick);
  if (upper == track.keys.begin())
    return upper->weight_per_million;
  if (upper == track.keys.end())
    return track.keys.back().weight_per_million;
  if (upper->tick == tick)
    return upper->weight_per_million;
  const auto &left = *(upper - 1);
  const auto span = static_cast<std::int64_t>(upper->tick - left.tick);
  const auto elapsed = static_cast<std::int64_t>(tick - left.tick);
  const auto delta = static_cast<std::int64_t>(upper->weight_per_million) -
                     left.weight_per_million;
  return static_cast<std::uint32_t>(
      static_cast<std::int64_t>(left.weight_per_million) +
      delta * elapsed / span);
}

std::vector<TargetRequirement>
projection_requirements(const Projection3dDefinition &projection,
                        std::span<const AnimationClip3d> animations) {
  std::vector<TargetRequirement> result{{TargetFeature::mesh_3d, true}};
  if (!projection.materials.empty())
    result.push_back({TargetFeature::pbr_materials_3d, true});
  if (projection.skeleton)
    result.push_back({TargetFeature::skeleton_3d, true});
  if (!projection.morph_targets.empty())
    result.push_back({TargetFeature::morph_targets_3d, true});
  if (!animations.empty())
    result.push_back({TargetFeature::animation_3d, true});
  if (!projection.lods.empty())
    result.push_back({TargetFeature::lod_3d, true});
  return result;
}
} // namespace

std::vector<std::byte> export_projection3d_glb(
    const Projection3dDefinition &projection,
    std::span<const AnimationClip3d> supplied_animations,
    std::span<const GltfTextureAsset> supplied, const GltfExportLimits &limits,
    const std::optional<TargetSourceEvidence> &source_evidence) {
  const auto validation = validate_projection3d(projection);
  if (!validation.ok())
    throw std::invalid_argument(validation.diagnostics.front().message);
  const auto lod_quality = analyze_lod_quality(projection, limits.lod_quality);
  if (!lod_quality.validation.ok())
    throw std::invalid_argument(
        lod_quality.validation.diagnostics.front().message);
  if (limits.maximum_glb_bytes < 20 ||
      limits.maximum_glb_bytes > std::numeric_limits<std::uint32_t>::max() ||
      limits.maximum_texture_bytes == 0)
    throw std::invalid_argument("GLB export limits are invalid");
  auto materials = projection.materials;
  auto meshes = projection.meshes;
  auto morphs = projection.morph_targets;
  auto animations = std::vector<AnimationClip3d>(supplied_animations.begin(),
                                                 supplied_animations.end());
  std::ranges::sort(materials, {}, &Material3d::id);
  std::ranges::sort(meshes, {}, &Mesh3d::id);
  std::ranges::sort(morphs, {}, &MorphTarget3d::id);
  std::map<std::string, LodLevel3d, std::less<>> lod_by_mesh;
  for (const auto &lod : projection.lods)
    lod_by_mesh.emplace(lod.mesh_id, lod);
  std::ranges::sort(animations, {}, &AnimationClip3d::id);
  std::set<std::string> animation_ids;
  for (const auto &clip : animations) {
    const auto clip_validation = validate_animation_clip3d(clip, projection);
    if (!clip_validation.ok() || !animation_ids.insert(clip.id).second)
      throw std::invalid_argument(
          clip_validation.ok() ? "duplicate GLB animation ID"
                               : clip_validation.diagnostics.front().message);
    if (projection.skeleton) {
      const auto deformation = analyze_deformation_quality(
          projection, clip, limits.deformation_quality);
      if (!deformation.validation.ok())
        throw std::invalid_argument(
            deformation.validation.diagnostics.front().message);
    }
  }
  std::map<std::string, GltfTextureAsset, std::less<>> textures;
  std::uint64_t texture_bytes{};
  for (const auto &t : supplied) {
    if (t.id.empty() || t.mime_type != "image/png" || t.encoded_bytes.empty() ||
        !textures.emplace(t.id, t).second)
      throw std::invalid_argument("GLB texture asset is invalid or duplicate");
    (void)decode_png(t.encoded_bytes, limits.texture_image);
    texture_bytes += t.encoded_bytes.size();
    if (texture_bytes > limits.maximum_texture_bytes)
      throw std::length_error("GLB textures exceed byte limit");
  }
  std::set<std::string> required;
  for (const auto &m : materials) {
    if (m.base_color_texture_id)
      required.insert(*m.base_color_texture_id);
    if (m.normal_texture_id)
      required.insert(*m.normal_texture_id);
    if (m.metallic_roughness_texture_id)
      required.insert(*m.metallic_roughness_texture_id);
  }
  for (const auto &id : required)
    if (!textures.contains(id))
      throw std::invalid_argument("required GLB texture is missing: " + id);
  if (textures.size() != required.size())
    throw std::invalid_argument("GLB texture set contains unreferenced assets");
  std::vector<View> views;
  std::vector<Accessor> accessors;
  std::vector<std::byte> bin;
  std::map<std::string, std::uint32_t, std::less<>> texture_indices;
  std::vector<std::pair<std::string, std::uint32_t>> image_views;
  for (const auto &[id, t] : textures) {
    const auto v = add_view(views, bin, t.encoded_bytes);
    texture_indices.emplace(id, static_cast<std::uint32_t>(image_views.size()));
    image_views.emplace_back(t.mime_type, v);
  }
  std::map<std::string, std::uint32_t, std::less<>> material_indices;
  for (std::uint32_t i = 0; i < materials.size(); ++i)
    material_indices.emplace(materials[i].id, i);
  std::map<std::string, std::uint32_t, std::less<>> joint_indices;
  std::vector<Joint3d> joints;
  if (projection.skeleton) {
    joints = projection.skeleton->joints;
    std::ranges::sort(joints, {}, &Joint3d::id);
    for (std::uint32_t i = 0; i < joints.size(); ++i)
      joint_indices.emplace(joints[i].id, i);
  }
  std::vector<Primitive> primitives;
  for (const auto &mesh : meshes) {
    Primitive p;
    std::vector<double> minv(3, std::numeric_limits<double>::infinity()),
        maxv(3, -std::numeric_limits<double>::infinity());
    auto pv = numeric_view(
        views, bin, mesh.vertices.size() * 3,
        [&](auto &o) {
          for (const auto &v : mesh.vertices) {
            const double q[3]{v.position.x / 1e6, v.position.y / 1e6,
                              v.position.z / 1e6};
            for (int k = 0; k < 3; ++k) {
              f32(o, static_cast<float>(q[k]));
              minv[k] = std::min(minv[k], q[k]);
              maxv[k] = std::max(maxv[k], q[k]);
            }
          }
        },
        34962, 12);
    p.position = accessor(accessors, pv, 5126,
                          static_cast<std::uint32_t>(mesh.vertices.size()),
                          "VEC3", minv, maxv);
    auto nv = numeric_view(
        views, bin, mesh.vertices.size() * 3,
        [&](auto &o) {
          for (const auto &v : mesh.vertices) {
            f32(o, v.normal.x / 1e6f);
            f32(o, v.normal.y / 1e6f);
            f32(o, v.normal.z / 1e6f);
          }
        },
        34962, 12);
    p.normal =
        accessor(accessors, nv, 5126,
                 static_cast<std::uint32_t>(mesh.vertices.size()), "VEC3");
    bool needs_uv = false;
    bool needs_tangent = false;
    if (mesh.material_id) {
      const auto material =
          std::ranges::find(materials, *mesh.material_id, &Material3d::id);
      needs_uv = material->base_color_texture_id.has_value() ||
                 material->normal_texture_id.has_value() ||
                 material->metallic_roughness_texture_id.has_value();
      needs_tangent = material->normal_texture_id.has_value();
    }
    if (needs_uv) {
      const auto quality = analyze_mesh_quality(mesh, limits.mesh_quality);
      if (!quality.validation.ok())
        throw std::invalid_argument(
            quality.validation.diagnostics.front().message);
      auto uv = numeric_view(
          views, bin, mesh.vertices.size() * 2,
          [&](auto &o) {
            for (const auto &v : mesh.vertices) {
              f32(o, v.texture_coordinate.u / 1e6f);
              f32(o, v.texture_coordinate.v / 1e6f);
            }
          },
          34962, 8);
      p.uv = accessor(accessors, uv, 5126,
                      static_cast<std::uint32_t>(mesh.vertices.size()), "VEC2");
      if (needs_tangent) {
        auto tangent = numeric_view(
            views, bin, quality.tangents.size() * 4,
            [&](auto &out) {
              for (const auto &value : quality.tangents) {
                f32(out, value.x / 1e6f);
                f32(out, value.y / 1e6f);
                f32(out, value.z / 1e6f);
                f32(out, static_cast<float>(value.handedness));
              }
            },
            34962, 16);
        p.tangent = accessor(
            accessors, tangent, 5126,
            static_cast<std::uint32_t>(quality.tangents.size()), "VEC4");
      }
    }
    if (projection.skeleton && mesh.purpose == MeshPurpose::render) {
      auto jv = numeric_view(
          views, bin, mesh.vertices.size() * 2,
          [&](auto &o) {
            for (const auto &v : mesh.vertices) {
              std::array<std::uint16_t, 4> ids{};
              for (std::size_t k = 0; k < v.influences.size(); ++k)
                ids[k] = static_cast<std::uint16_t>(
                    joint_indices.at(v.influences[k].joint_id));
              for (auto id : ids)
                u16(o, id);
            }
          },
          34962, 8);
      p.joints =
          accessor(accessors, jv, 5123,
                   static_cast<std::uint32_t>(mesh.vertices.size()), "VEC4");
      auto wv = numeric_view(
          views, bin, mesh.vertices.size() * 4,
          [&](auto &o) {
            for (const auto &v : mesh.vertices) {
              std::array<float, 4> w{};
              for (std::size_t k = 0; k < v.influences.size(); ++k)
                w[k] = v.influences[k].weight_per_million / 1e6f;
              for (auto q : w)
                f32(o, q);
            }
          },
          34962, 16);
      p.weights =
          accessor(accessors, wv, 5126,
                   static_cast<std::uint32_t>(mesh.vertices.size()), "VEC4");
    }
    std::uint32_t imin = std::numeric_limits<std::uint32_t>::max(), imax{};
    auto iv = numeric_view(
        views, bin, mesh.triangle_indices.size(),
        [&](auto &o) {
          for (auto i : mesh.triangle_indices) {
            u32(o, i);
            imin = std::min(imin, i);
            imax = std::max(imax, i);
          }
        },
        34963);
    p.indices = accessor(
        accessors, iv, 5125,
        static_cast<std::uint32_t>(mesh.triangle_indices.size()), "SCALAR",
        {static_cast<double>(imin)}, {static_cast<double>(imax)});
    if (mesh.material_id)
      p.material = material_indices.at(*mesh.material_id);
    for (const auto &m : morphs)
      if (m.mesh_id == mesh.id) {
        std::vector<double> mn(3, std::numeric_limits<double>::infinity()),
            mx(3, -std::numeric_limits<double>::infinity());
        auto mv = numeric_view(
            views, bin, m.position_deltas.size() * 3,
            [&](auto &o) {
              for (const auto &d : m.position_deltas) {
                const double q[3]{d.x / 1e6, d.y / 1e6, d.z / 1e6};
                for (int k = 0; k < 3; ++k) {
                  f32(o, static_cast<float>(q[k]));
                  mn[k] = std::min(mn[k], q[k]);
                  mx[k] = std::max(mx[k], q[k]);
                }
              }
            },
            34962, 12);
        p.morphs.push_back(
            accessor(accessors, mv, 5126,
                     static_cast<std::uint32_t>(m.position_deltas.size()),
                     "VEC3", mn, mx));
        p.morph_names.push_back(m.id);
      }
    primitives.push_back(std::move(p));
  }
  std::optional<std::uint32_t> inverse_accessor;
  if (projection.skeleton) {
    std::map<std::string, Matrix, std::less<>> globals;
    std::function<Matrix(const Joint3d &)> global = [&](const Joint3d &j) {
      if (auto it = globals.find(j.id); it != globals.end())
        return it->second;
      auto g = local(j);
      if (j.parent_id) {
        const auto &parent =
            *std::ranges::find(joints, *j.parent_id, &Joint3d::id);
        g = multiply(global(parent), g);
      }
      globals.emplace(j.id, g);
      return g;
    };
    auto bv = numeric_view(views, bin, joints.size() * 16, [&](auto &o) {
      for (const auto &j : joints)
        for (double q : inverse_rigid(global(j)).v)
          f32(o, static_cast<float>(q));
    });
    inverse_accessor = accessor(
        accessors, bv, 5126, static_cast<std::uint32_t>(joints.size()), "MAT4");
  }
  std::vector<AnimationGlb> animation_payloads;
  std::map<std::string, std::pair<std::uint32_t, std::uint32_t>, std::less<>>
      morph_locations;
  for (std::uint32_t mesh_index = 0; mesh_index < meshes.size(); ++mesh_index)
    for (std::uint32_t morph_index = 0;
         morph_index < primitives[mesh_index].morph_names.size(); ++morph_index)
      morph_locations.emplace(primitives[mesh_index].morph_names[morph_index],
                              std::pair{mesh_index, morph_index});
  for (const auto &clip : animations) {
    AnimationGlb payload{clip.id, clip.looping, clip.events, {}, {}};
    auto add_times = [&](const auto &keys) {
      auto view = numeric_view(views, bin, keys.size(), [&](auto &out) {
        for (const auto &key : keys)
          f32(out, static_cast<float>(key.tick) / clip.ticks_per_second);
      });
      return accessor(
          accessors, view, 5126, static_cast<std::uint32_t>(keys.size()),
          "SCALAR", {0.0},
          {static_cast<double>(keys.back().tick) / clip.ticks_per_second});
    };
    for (const auto &track : clip.joint_tracks) {
      const auto input = add_times(track.keys);
      const auto node = joint_indices.at(track.joint_id);
      auto add_output = [&](std::string path, std::string type,
                            std::size_t components, auto writer) {
        auto view = numeric_view(views, bin, track.keys.size() * components,
                                 [&](auto &out) {
                                   for (const auto &key : track.keys)
                                     writer(out, key);
                                 });
        const auto output = accessor(
            accessors, view, 5126,
            static_cast<std::uint32_t>(track.keys.size()), std::move(type));
        const auto sampler =
            static_cast<std::uint32_t>(payload.samplers.size());
        payload.samplers.push_back({input, output});
        payload.channels.push_back({sampler, node, std::move(path)});
      };
      add_output("translation", "VEC3", 3, [](auto &out, const auto &key) {
        f32(out, key.pose.translation.x / 1e6f);
        f32(out, key.pose.translation.y / 1e6f);
        f32(out, key.pose.translation.z / 1e6f);
      });
      add_output("rotation", "VEC4", 4, [](auto &out, const auto &key) {
        for (auto value : key.pose.rotation_xyzw_ppm)
          f32(out, value / 1e6f);
      });
      add_output("scale", "VEC3", 3, [](auto &out, const auto &key) {
        for (auto value : key.pose.scale_xyz_ppm)
          f32(out, value / 1e6f);
      });
    }
    std::map<std::uint32_t, std::vector<const MorphTrack3d *>> tracks_by_mesh;
    for (const auto &track : clip.morph_tracks)
      tracks_by_mesh[morph_locations.at(track.morph_target_id).first].push_back(
          &track);
    for (auto &[mesh_index, tracks] : tracks_by_mesh) {
      std::set<std::uint32_t> tick_set;
      for (const auto *track : tracks)
        for (const auto &key : track->keys)
          tick_set.insert(key.tick);
      std::vector<MorphKeyframe3d> timeline;
      for (auto tick : tick_set)
        timeline.push_back({tick, 0});
      const auto input = add_times(timeline);
      std::map<std::string, const MorphTrack3d *, std::less<>> by_id;
      for (const auto *track : tracks)
        by_id.emplace(track->morph_target_id, track);
      const auto target_count = primitives[mesh_index].morph_names.size();
      auto view = numeric_view(
          views, bin, timeline.size() * target_count, [&](auto &out) {
            for (const auto &time : timeline)
              for (const auto &name : primitives[mesh_index].morph_names) {
                const auto found = by_id.find(name);
                f32(out, found == by_id.end()
                             ? 0.0f
                             : sample_morph(*found->second, time.tick) / 1e6f);
              }
          });
      const auto output = accessor(
          accessors, view, 5126,
          static_cast<std::uint32_t>(timeline.size() * target_count), "SCALAR");
      const auto sampler = static_cast<std::uint32_t>(payload.samplers.size());
      payload.samplers.push_back({input, output});
      payload.channels.push_back(
          {sampler, static_cast<std::uint32_t>(joints.size() + mesh_index),
           "weights"});
    }
    animation_payloads.push_back(std::move(payload));
  }
  std::ostringstream json;
  const auto target_requirements =
      projection_requirements(projection, animations);
  const auto target_report =
      canonicalize_target_compatibility(evaluate_target_compatibility(
          builtin_target_adapter("glb-2.0"), target_requirements));
  json << "{\"asset\":{\"generator\":\"11vatedTech GSPL "
          "Sprites\",\"version\":\"2.0\",\"extras\":{"
          "\"gsplSourceEvidence\":"
       << canonicalize_target_source_evidence(source_evidence)
       << ",\"gsplTargetRequirements\":"
       << canonicalize_target_requirements(target_requirements)
       << ",\"gsplTargetReport\":" << target_report << "}},\"scene\":0,";
  json << "\"buffers\":[{\"byteLength\":" << bin.size()
       << "}],\"bufferViews\":[";
  for (std::size_t i = 0; i < views.size(); ++i) {
    if (i)
      json << ',';
    json << "{\"buffer\":0,\"byteOffset\":" << views[i].offset
         << ",\"byteLength\":" << views[i].length;
    if (views[i].target)
      json << ",\"target\":" << *views[i].target;
    if (views[i].byte_stride)
      json << ",\"byteStride\":" << *views[i].byte_stride;
    json << '}';
  }
  json << "],\"accessors\":[";
  for (std::size_t i = 0; i < accessors.size(); ++i) {
    if (i)
      json << ',';
    const auto &a = accessors[i];
    json << "{\"bufferView\":" << a.view << ",\"componentType\":" << a.component
         << ",\"count\":" << a.count << ",\"type\":\"" << a.type << '"';
    if (!a.minimum.empty())
      json << ",\"min\":" << array(a.minimum);
    if (!a.maximum.empty())
      json << ",\"max\":" << array(a.maximum);
    json << '}';
  }
  json << ']';
  if (!image_views.empty()) {
    json << ",\"images\":[";
    for (std::size_t i = 0; i < image_views.size(); ++i) {
      if (i)
        json << ',';
      json << "{\"bufferView\":" << image_views[i].second << ",\"mimeType\":\""
           << image_views[i].first << "\"}";
    }
    json << "],\"textures\":[";
    for (std::size_t i = 0; i < image_views.size(); ++i) {
      if (i)
        json << ',';
      json << "{\"source\":" << i << '}';
    }
    json << ']';
  }
  json << ",\"materials\":[";
  for (std::size_t i = 0; i < materials.size(); ++i) {
    if (i)
      json << ',';
    const auto &m = materials[i];
    const double r = ((m.base_color_rgba >> 24) & 255) / 255.0,
                 g = ((m.base_color_rgba >> 16) & 255) / 255.0,
                 b = ((m.base_color_rgba >> 8) & 255) / 255.0,
                 a = (m.base_color_rgba & 255) / 255.0;
    json << "{\"name\":\"" << m.id
         << "\",\"pbrMetallicRoughness\":{\"baseColorFactor\":"
         << array(std::array<double, 4>{r, g, b, a})
         << ",\"metallicFactor\":" << number(m.metallic_per_million / 1e6)
         << ",\"roughnessFactor\":" << number(m.roughness_per_million / 1e6);
    if (m.base_color_texture_id)
      json << ",\"baseColorTexture\":{\"index\":"
           << texture_indices.at(*m.base_color_texture_id) << '}';
    if (m.metallic_roughness_texture_id)
      json << ",\"metallicRoughnessTexture\":{\"index\":"
           << texture_indices.at(*m.metallic_roughness_texture_id) << '}';
    json << "},\"alphaMode\":\"" << alpha(m.alpha_mode)
         << "\",\"doubleSided\":" << (m.double_sided ? "true" : "false");
    if (m.alpha_mode == MaterialAlphaMode::mask)
      json << ",\"alphaCutoff\":" << number(m.alpha_cutoff_per_million / 1e6);
    if (m.normal_texture_id)
      json << ",\"normalTexture\":{\"index\":"
           << texture_indices.at(*m.normal_texture_id) << '}';
    json << '}';
  }
  json << ']';
  json << ",\"meshes\":[";
  for (std::size_t i = 0; i < meshes.size(); ++i) {
    if (i)
      json << ',';
    const auto &p = primitives[i];
    json << "{\"name\":\"" << meshes[i].id
         << "\",\"primitives\":[{\"attributes\":{\"POSITION\":" << p.position
         << ",\"NORMAL\":" << p.normal;
    if (p.uv)
      json << ",\"TEXCOORD_0\":" << *p.uv;
    if (p.tangent)
      json << ",\"TANGENT\":" << *p.tangent;
    if (p.joints)
      json << ",\"JOINTS_0\":" << *p.joints << ",\"WEIGHTS_0\":" << *p.weights;
    json << "},\"indices\":" << p.indices;
    if (p.material)
      json << ",\"material\":" << *p.material;
    if (!p.morphs.empty()) {
      json << ",\"targets\":[";
      for (std::size_t k = 0; k < p.morphs.size(); ++k) {
        if (k)
          json << ',';
        json << "{\"POSITION\":" << p.morphs[k] << '}';
      }
      json << ']';
    }
    json << "}]";
    if (!p.morph_names.empty()) {
      json << ",\"weights\":[";
      for (std::size_t k = 0; k < p.morph_names.size(); ++k) {
        if (k)
          json << ',';
        json << '0';
      }
      json << ']';
    }
    json << ",\"extras\":{\"gsplPurpose\":\""
         << (meshes[i].purpose == MeshPurpose::render ? "render" : "collision")
         << '"';
    if (!p.morph_names.empty()) {
      json << ",\"targetNames\":[";
      for (std::size_t k = 0; k < p.morph_names.size(); ++k) {
        if (k)
          json << ',';
        json << '"' << p.morph_names[k] << '"';
      }
      json << ']';
    }
    json << "}}";
  }
  json << ']';
  json << ",\"nodes\":[";
  bool first = true;
  for (const auto &j : joints) {
    if (!first)
      json << ',';
    first = false;
    json << "{\"name\":\"" << j.id << "\",\"translation\":"
         << array(std::array<double, 3>{j.translation.x / 1e6,
                                        j.translation.y / 1e6,
                                        j.translation.z / 1e6})
         << ",\"rotation\":"
         << array(std::array<double, 4>{
                j.rotation_xyzw_ppm[0] / 1e6, j.rotation_xyzw_ppm[1] / 1e6,
                j.rotation_xyzw_ppm[2] / 1e6, j.rotation_xyzw_ppm[3] / 1e6});
    std::vector<std::uint32_t> children;
    for (std::uint32_t k = 0; k < joints.size(); ++k)
      if (joints[k].parent_id && *joints[k].parent_id == j.id)
        children.push_back(k);
    if (!children.empty()) {
      json << ",\"children\":[";
      for (std::size_t k = 0; k < children.size(); ++k) {
        if (k)
          json << ',';
        json << children[k];
      }
      json << ']';
    }
    json << '}';
  }
  for (std::size_t i = 0; i < meshes.size(); ++i) {
    if (!first)
      json << ',';
    first = false;
    json << "{\"name\":\"" << meshes[i].id << ".node\",\"mesh\":" << i;
    if (projection.skeleton && meshes[i].purpose == MeshPurpose::render)
      json << ",\"skin\":0";
    if (const auto found = lod_by_mesh.find(meshes[i].id);
        found != lod_by_mesh.end())
      json << ",\"extras\":{\"gsplLodLevel\":" << found->second.level
           << ",\"gsplMinimumScreenCoveragePerMillion\":"
           << found->second.minimum_screen_coverage_per_million << '}';
    json << '}';
  }
  json << ']';
  if (projection.skeleton) {
    std::uint32_t root{};
    for (std::uint32_t i = 0; i < joints.size(); ++i)
      if (!joints[i].parent_id)
        root = i;
    json << ",\"skins\":[{\"inverseBindMatrices\":" << *inverse_accessor
         << ",\"skeleton\":" << root << ",\"joints\":[";
    for (std::size_t i = 0; i < joints.size(); ++i) {
      if (i)
        json << ',';
      json << i;
    }
    json << "]}]";
  }
  if (!animation_payloads.empty()) {
    json << ",\"animations\":[";
    for (std::size_t i = 0; i < animation_payloads.size(); ++i) {
      if (i)
        json << ',';
      const auto &animation = animation_payloads[i];
      json << "{\"name\":\"" << animation.id << "\",\"samplers\":[";
      for (std::size_t k = 0; k < animation.samplers.size(); ++k) {
        if (k)
          json << ',';
        json << "{\"input\":" << animation.samplers[k].input
             << ",\"output\":" << animation.samplers[k].output
             << ",\"interpolation\":\"LINEAR\"}";
      }
      json << "],\"channels\":[";
      for (std::size_t k = 0; k < animation.channels.size(); ++k) {
        if (k)
          json << ',';
        json << "{\"sampler\":" << animation.channels[k].sampler
             << ",\"target\":{\"node\":" << animation.channels[k].node
             << ",\"path\":\"" << animation.channels[k].path << "\"}}";
      }
      auto events = animation.events;
      std::ranges::sort(events, [](const auto &left, const auto &right) {
        return std::tuple{left.tick, left.id} <
               std::tuple{right.tick, right.id};
      });
      json << "],\"extras\":{\"gsplLooping\":"
           << (animation.looping ? "true" : "false") << ",\"gsplEvents\":[";
      for (std::size_t k = 0; k < events.size(); ++k) {
        if (k)
          json << ',';
        json << "{\"id\":\"" << events[k].id << "\",\"tick\":" << events[k].tick
             << '}';
      }
      json << "]}}";
    }
    json << ']';
  }
  json << ",\"scenes\":[{\"nodes\":[";
  bool comma = false;
  if (projection.skeleton) {
    for (std::uint32_t i = 0; i < joints.size(); ++i)
      if (!joints[i].parent_id) {
        json << i;
        comma = true;
      }
  }
  for (std::size_t i = 0; i < meshes.size(); ++i) {
    if (comma)
      json << ',';
    comma = true;
    json << joints.size() + i;
  }
  json << "]}]}";
  auto text = json.str();
  while (text.size() % 4)
    text.push_back(' ');
  align4(bin);
  const std::uint64_t total =
      12 + 8 + text.size() + (bin.empty() ? 0 : 8 + bin.size());
  if (total > limits.maximum_glb_bytes ||
      total > std::numeric_limits<std::uint32_t>::max())
    throw std::length_error("GLB exceeds output limit");
  std::vector<std::byte> out;
  out.reserve(static_cast<std::size_t>(total));
  u32(out, 0x46546c67);
  u32(out, 2);
  u32(out, static_cast<std::uint32_t>(total));
  u32(out, static_cast<std::uint32_t>(text.size()));
  u32(out, 0x4e4f534a);
  for (char c : text)
    out.push_back(static_cast<std::byte>(c));
  if (!bin.empty()) {
    u32(out, static_cast<std::uint32_t>(bin.size()));
    u32(out, 0x004e4942);
    out.insert(out.end(), bin.begin(), bin.end());
  }
  const auto verification =
      verify_projection3d_glb(out, limits.maximum_glb_bytes);
  if (!verification.ok())
    throw std::runtime_error(
        "generated GLB failed self-verification: " +
        verification.validation.diagnostics.front().code + ": " +
        verification.validation.diagnostics.front().message);
  return out;
}

std::vector<std::byte>
export_projection3d_glb(const Projection3dDefinition &projection,
                        std::span<const GltfTextureAsset> supplied,
                        const GltfExportLimits &limits) {
  return export_projection3d_glb(projection, std::span<const AnimationClip3d>{},
                                 supplied, limits, {});
}
} // namespace gspl::sprites
