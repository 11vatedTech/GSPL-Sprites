#include "gspl_sprites/deformation_quality.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>

namespace gspl::sprites {
namespace {
struct Matrix {
  std::array<long double, 16> v{};
};
struct Point3 {
  long double x{}, y{}, z{};
};
Matrix multiply(const Matrix &a, const Matrix &b) {
  Matrix r;
  for (int c = 0; c < 4; ++c)
    for (int row = 0; row < 4; ++row)
      for (int k = 0; k < 4; ++k)
        r.v[c * 4 + row] += a.v[k * 4 + row] * b.v[c * 4 + k];
  return r;
}
Matrix transform(const Vector3Micrometers &t,
                 const std::array<std::int32_t, 4> &q,
                 const std::array<std::int32_t, 3> &s) {
  const long double x = q[0] / 1e6L, y = q[1] / 1e6L, z = q[2] / 1e6L,
                    w = q[3] / 1e6L, sx = s[0] / 1e6L, sy = s[1] / 1e6L,
                    sz = s[2] / 1e6L;
  return {{(1 - 2 * y * y - 2 * z * z) * sx, (2 * x * y + 2 * w * z) * sx,
           (2 * x * z - 2 * w * y) * sx, 0, (2 * x * y - 2 * w * z) * sy,
           (1 - 2 * x * x - 2 * z * z) * sy, (2 * y * z + 2 * w * x) * sy, 0,
           (2 * x * z + 2 * w * y) * sz, (2 * y * z - 2 * w * x) * sz,
           (1 - 2 * x * x - 2 * y * y) * sz, 0, static_cast<long double>(t.x),
           static_cast<long double>(t.y), static_cast<long double>(t.z), 1}};
}
Matrix inverse_rigid(const Matrix &m) {
  Matrix r{{m.v[0], m.v[4], m.v[8], 0, m.v[1], m.v[5], m.v[9], 0, m.v[2],
            m.v[6], m.v[10], 0, 0, 0, 0, 1}};
  r.v[12] = -(r.v[0] * m.v[12] + r.v[4] * m.v[13] + r.v[8] * m.v[14]);
  r.v[13] = -(r.v[1] * m.v[12] + r.v[5] * m.v[13] + r.v[9] * m.v[14]);
  r.v[14] = -(r.v[2] * m.v[12] + r.v[6] * m.v[13] + r.v[10] * m.v[14]);
  return r;
}
Point3 apply(const Matrix &m, const Vector3Micrometers &p) {
  const auto x = static_cast<long double>(p.x);
  const auto y = static_cast<long double>(p.y);
  const auto z = static_cast<long double>(p.z);
  return {m.v[0] * x + m.v[4] * y + m.v[8] * z + m.v[12],
          m.v[1] * x + m.v[5] * y + m.v[9] * z + m.v[13],
          m.v[2] * x + m.v[6] * y + m.v[10] * z + m.v[14]};
}
long double triangle_area(Point3 a, Point3 b, Point3 c) {
  const auto x1 = b.x - a.x, y1 = b.y - a.y, z1 = b.z - a.z, x2 = c.x - a.x,
             y2 = c.y - a.y, z2 = c.z - a.z;
  const auto x = y1 * z2 - z1 * y2, y = z1 * x2 - x1 * z2,
             z = x1 * y2 - y1 * x2;
  return std::sqrt(x * x + y * y + z * z) / 2;
}
JointPose3d sample(const JointTrack3d &track, std::uint32_t tick) {
  const auto upper =
      std::ranges::lower_bound(track.keys, tick, {}, &JointKeyframe3d::tick);
  if (upper == track.keys.begin())
    return upper->pose;
  if (upper == track.keys.end())
    return track.keys.back().pose;
  if (upper->tick == tick)
    return upper->pose;
  const auto &left = *(upper - 1);
  const long double alpha =
      static_cast<long double>(tick - left.tick) / (upper->tick - left.tick);
  JointPose3d pose;
  auto linear = [&](auto a, auto b) {
    const auto left_value = static_cast<long double>(a);
    const auto right_value = static_cast<long double>(b);
    return static_cast<std::int64_t>(
        std::llround(left_value + (right_value - left_value) * alpha));
  };
  pose.translation = {
      linear(left.pose.translation.x, upper->pose.translation.x),
      linear(left.pose.translation.y, upper->pose.translation.y),
      linear(left.pose.translation.z, upper->pose.translation.z)};
  long double dot{};
  for (int i = 0; i < 4; ++i)
    dot += static_cast<long double>(left.pose.rotation_xyzw_ppm[i]) *
           upper->pose.rotation_xyzw_ppm[i];
  long double length{};
  for (int i = 0; i < 4; ++i) {
    const auto right = dot < 0 ? -upper->pose.rotation_xyzw_ppm[i]
                               : upper->pose.rotation_xyzw_ppm[i];
    const auto value = left.pose.rotation_xyzw_ppm[i] +
                       (right - left.pose.rotation_xyzw_ppm[i]) * alpha;
    pose.rotation_xyzw_ppm[i] = static_cast<std::int32_t>(std::llround(value));
    length += value * value;
  }
  length = std::sqrt(length);
  for (auto &value : pose.rotation_xyzw_ppm)
    value = static_cast<std::int32_t>(std::llround(value * 1e6L / length));
  for (int i = 0; i < 3; ++i)
    pose.scale_xyz_ppm[i] = static_cast<std::int32_t>(
        linear(left.pose.scale_xyz_ppm[i], upper->pose.scale_xyz_ppm[i]));
  return pose;
}
void add(ValidationResult &r, std::string code, std::string message) {
  r.diagnostics.push_back({std::move(code), std::move(message)});
}
} // namespace

DeformationQualityReport
analyze_deformation_quality(const Projection3dDefinition &projection,
                            const AnimationClip3d &clip,
                            const DeformationQualityPolicy &policy) {
  const auto pv = validate_projection3d(projection);
  if (!pv.ok())
    throw std::invalid_argument(pv.diagnostics.front().message);
  const auto av = validate_animation_clip3d(clip, projection);
  if (!av.ok())
    throw std::invalid_argument(av.diagnostics.front().message);
  if (!projection.skeleton)
    throw std::invalid_argument("deformation quality requires a skeleton");
  if (policy.maximum_sampled_ticks < 2 ||
      policy.maximum_vertex_evaluations == 0 ||
      policy.minimum_triangle_area_ratio_per_million > 1'000'000 ||
      policy.maximum_vertex_displacement_micrometers == 0)
    throw std::invalid_argument("deformation quality policy is invalid");
  auto joints = projection.skeleton->joints;
  std::ranges::sort(joints, {}, &Joint3d::id);
  std::map<std::string, std::size_t, std::less<>> joint_index;
  for (std::size_t i = 0; i < joints.size(); ++i)
    joint_index.emplace(joints[i].id, i);
  std::map<std::string, const JointTrack3d *, std::less<>> tracks;
  for (const auto &track : clip.joint_tracks)
    tracks.emplace(track.joint_id, &track);
  std::vector<Matrix> inverse_bind(joints.size());
  std::vector<Matrix> rest_global(joints.size());
  std::vector<bool> rest_ready(joints.size());
  std::function<Matrix(std::size_t)> rest = [&](std::size_t i) {
    if (rest_ready[i])
      return rest_global[i];
    const std::array<std::int32_t, 3> scale{1'000'000, 1'000'000, 1'000'000};
    auto value =
        transform(joints[i].translation, joints[i].rotation_xyzw_ppm, scale);
    if (joints[i].parent_id)
      value = multiply(rest(joint_index.at(*joints[i].parent_id)), value);
    rest_ready[i] = true;
    return rest_global[i] = value;
  };
  for (std::size_t i = 0; i < joints.size(); ++i)
    inverse_bind[i] = inverse_rigid(rest(i));
  std::set<std::uint32_t> ticks{0, clip.duration_ticks};
  for (const auto &track : clip.joint_tracks)
    for (const auto &key : track.keys)
      ticks.insert(key.tick);
  if (ticks.size() > policy.maximum_sampled_ticks)
    throw std::length_error("deformation key ticks exceed sample limit");
  const auto remaining = policy.maximum_sampled_ticks - ticks.size();
  for (std::uint32_t i = 1; i <= remaining; ++i)
    ticks.insert(static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(i) * clip.duration_ticks) /
        (remaining + 1)));
  std::uint64_t render_vertices{};
  for (const auto &mesh : projection.meshes)
    if (mesh.purpose == MeshPurpose::render)
      render_vertices += mesh.vertices.size();
  if (render_vertices != 0 &&
      ticks.size() > policy.maximum_vertex_evaluations / render_vertices)
    throw std::length_error("deformation vertex evaluation limit exceeded");
  DeformationQualityReport report;
  report.sampled_ticks = static_cast<std::uint32_t>(ticks.size());
  report.vertex_evaluations = render_vertices * ticks.size();
  for (auto tick : ticks) {
    std::vector<Matrix> globals(joints.size());
    std::vector<bool> ready(joints.size());
    std::function<Matrix(std::size_t)> global = [&](std::size_t i) {
      if (ready[i])
        return globals[i];
      JointPose3d pose;
      if (const auto found = tracks.find(joints[i].id); found != tracks.end())
        pose = sample(*found->second, tick);
      else {
        pose.translation = joints[i].translation;
        pose.rotation_xyzw_ppm = joints[i].rotation_xyzw_ppm;
      }
      auto value = transform(pose.translation, pose.rotation_xyzw_ppm,
                             pose.scale_xyz_ppm);
      if (joints[i].parent_id)
        value = multiply(global(joint_index.at(*joints[i].parent_id)), value);
      ready[i] = true;
      return globals[i] = value;
    };
    std::vector<Matrix> skin(joints.size());
    for (std::size_t i = 0; i < joints.size(); ++i)
      skin[i] = multiply(global(i), inverse_bind[i]);
    for (const auto &mesh : projection.meshes)
      if (mesh.purpose == MeshPurpose::render) {
        std::vector<Point3> deformed;
        deformed.reserve(mesh.vertices.size());
        for (const auto &vertex : mesh.vertices) {
          Point3 point{};
          for (const auto &influence : vertex.influences) {
            const auto q = apply(skin[joint_index.at(influence.joint_id)],
                                 vertex.position);
            const auto weight = influence.weight_per_million / 1e6L;
            point.x += q.x * weight;
            point.y += q.y * weight;
            point.z += q.z * weight;
          }
          deformed.push_back(point);
          const auto dx = point.x - vertex.position.x,
                     dy = point.y - vertex.position.y,
                     dz = point.z - vertex.position.z;
          const auto displacement = static_cast<std::uint64_t>(
              std::llround(std::sqrt(dx * dx + dy * dy + dz * dz)));
          report.maximum_vertex_displacement_micrometers = std::max(
              report.maximum_vertex_displacement_micrometers, displacement);
        }
        for (std::size_t i = 0; i < mesh.triangle_indices.size(); i += 3) {
          const auto a = mesh.triangle_indices[i],
                     b = mesh.triangle_indices[i + 1],
                     c = mesh.triangle_indices[i + 2];
          const Point3 ba{
              static_cast<long double>(mesh.vertices[a].position.x),
              static_cast<long double>(mesh.vertices[a].position.y),
              static_cast<long double>(mesh.vertices[a].position.z)},
              bb{static_cast<long double>(mesh.vertices[b].position.x),
                 static_cast<long double>(mesh.vertices[b].position.y),
                 static_cast<long double>(mesh.vertices[b].position.z)},
              bc{static_cast<long double>(mesh.vertices[c].position.x),
                 static_cast<long double>(mesh.vertices[c].position.y),
                 static_cast<long double>(mesh.vertices[c].position.z)};
          const auto base = triangle_area(ba, bb, bc),
                     posed =
                         triangle_area(deformed[a], deformed[b], deformed[c]);
          const auto ratio = static_cast<std::uint32_t>(std::min<long double>(
              1'000'000, std::floor(posed / base * 1e6L)));
          report.minimum_triangle_area_ratio_per_million =
              std::min(report.minimum_triangle_area_ratio_per_million, ratio);
          if (ratio < policy.minimum_triangle_area_ratio_per_million)
            ++report.collapsed_triangle_samples;
        }
      }
  }
  if (report.collapsed_triangle_samples)
    add(report.validation, "SPRITE_3D_DEFORMATION_COLLAPSE",
        "animation collapses triangle area below policy");
  if (report.maximum_vertex_displacement_micrometers >
      policy.maximum_vertex_displacement_micrometers)
    add(report.validation, "SPRITE_3D_DEFORMATION_DISPLACEMENT",
        "animation exceeds vertex displacement policy");
  return report;
}
} // namespace gspl::sprites
