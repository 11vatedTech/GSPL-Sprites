#include "gspl_sprites/lod_quality.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace gspl::sprites {
namespace {
struct Point {
  long double x{}, y{}, z{};
};
Point point(const Vector3Micrometers &p) {
  return {static_cast<long double>(p.x), static_cast<long double>(p.y),
          static_cast<long double>(p.z)};
}
Point sub(Point a, Point b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Point addp(Point a, Point b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Point scale(Point a, long double s) { return {a.x * s, a.y * s, a.z * s}; }
long double dot(Point a, Point b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
long double distance2(Point a, Point b) { return dot(sub(a, b), sub(a, b)); }
long double point_triangle_distance2(Point p, Point a, Point b, Point c) {
  const auto ab = sub(b, a), ac = sub(c, a), ap = sub(p, a);
  const auto d1 = dot(ab, ap), d2 = dot(ac, ap);
  if (d1 <= 0 && d2 <= 0)
    return distance2(p, a);
  const auto bp = sub(p, b);
  const auto d3 = dot(ab, bp), d4 = dot(ac, bp);
  if (d3 >= 0 && d4 <= d3)
    return distance2(p, b);
  const auto vc = d1 * d4 - d3 * d2;
  if (vc <= 0 && d1 >= 0 && d3 <= 0) {
    const auto v = d1 / (d1 - d3);
    return distance2(p, addp(a, scale(ab, v)));
  }
  const auto cp = sub(p, c);
  const auto d5 = dot(ab, cp), d6 = dot(ac, cp);
  if (d6 >= 0 && d5 <= d6)
    return distance2(p, c);
  const auto vb = d5 * d2 - d1 * d6;
  if (vb <= 0 && d2 >= 0 && d6 <= 0) {
    const auto w = d2 / (d2 - d6);
    return distance2(p, addp(a, scale(ac, w)));
  }
  const auto va = d3 * d6 - d5 * d4;
  if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) {
    const auto w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
    return distance2(p, addp(b, scale(sub(c, b), w)));
  }
  const auto denominator = 1 / (va + vb + vc), v = vb * denominator,
             w = vc * denominator;
  return distance2(p, addp(a, addp(scale(ab, v), scale(ac, w))));
}
std::vector<Point> samples(const Mesh3d &mesh) {
  std::vector<Point> result;
  result.reserve(mesh.vertices.size() + mesh.triangle_indices.size() / 3);
  for (const auto &v : mesh.vertices)
    result.push_back(point(v.position));
  for (std::size_t i = 0; i < mesh.triangle_indices.size(); i += 3) {
    const auto a = point(mesh.vertices[mesh.triangle_indices[i]].position),
               b = point(mesh.vertices[mesh.triangle_indices[i + 1]].position),
               c = point(mesh.vertices[mesh.triangle_indices[i + 2]].position);
    result.push_back(scale(addp(a, addp(b, c)), 1.0L / 3));
  }
  return result;
}
std::uint64_t directed_error(const std::vector<Point> &source,
                             const Mesh3d &target, std::uint64_t &evaluations,
                             std::uint64_t maximum) {
  long double worst{};
  for (const auto &p : source) {
    long double nearest = std::numeric_limits<long double>::infinity();
    for (std::size_t i = 0; i < target.triangle_indices.size(); i += 3) {
      if (evaluations == maximum)
        throw std::length_error("LOD distance evaluation limit exceeded");
      ++evaluations;
      const auto a = point(
                     target.vertices[target.triangle_indices[i]].position),
                 b = point(
                     target.vertices[target.triangle_indices[i + 1]].position),
                 c = point(
                     target.vertices[target.triangle_indices[i + 2]].position);
      nearest = std::min(nearest, point_triangle_distance2(p, a, b, c));
    }
    worst = std::max(worst, nearest);
  }
  const auto root = std::sqrt(worst);
  if (root > std::numeric_limits<std::uint64_t>::max())
    throw std::overflow_error("LOD geometric error exceeds integer range");
  return static_cast<std::uint64_t>(std::llround(root));
}
void add(ValidationResult &r, std::string code, std::string message) {
  r.diagnostics.push_back({std::move(code), std::move(message)});
}
} // namespace

LodQualityReport analyze_lod_quality(const Projection3dDefinition &projection,
                                     const LodQualityPolicy &policy) {
  const auto validation = validate_projection3d(projection);
  if (!validation.ok())
    throw std::invalid_argument(validation.diagnostics.front().message);
  if (policy.maximum_distance_evaluations == 0 ||
      policy.maximum_geometric_error_micrometers == 0)
    throw std::invalid_argument("LOD quality policy is invalid");
  LodQualityReport report;
  if (projection.lods.size() < 2)
    return report;
  auto lods = projection.lods;
  std::ranges::sort(lods, {}, &LodLevel3d::level);
  const auto &base =
      *std::ranges::find(projection.meshes, lods.front().mesh_id, &Mesh3d::id);
  const auto base_samples = samples(base);
  for (std::size_t i = 1; i < lods.size(); ++i) {
    const auto &mesh =
        *std::ranges::find(projection.meshes, lods[i].mesh_id, &Mesh3d::id);
    const auto before = report.total_distance_evaluations;
    const auto lower_samples = samples(mesh);
    const auto forward =
        directed_error(lower_samples, base, report.total_distance_evaluations,
                       policy.maximum_distance_evaluations);
    const auto reverse =
        directed_error(base_samples, mesh, report.total_distance_evaluations,
                       policy.maximum_distance_evaluations);
    const auto error = std::max(forward, reverse);
    report.levels.push_back({lods[i].level, lods[i].mesh_id,
                             report.total_distance_evaluations - before,
                             error});
    if (error > policy.maximum_geometric_error_micrometers)
      add(report.validation, "SPRITE_3D_LOD_GEOMETRIC_ERROR",
          "LOD geometric error exceeds policy");
    if (policy.require_material_match && mesh.material_id != base.material_id)
      add(report.validation, "SPRITE_3D_LOD_MATERIAL_MISMATCH",
          "LOD material does not match level zero");
  }
  return report;
}
} // namespace gspl::sprites
