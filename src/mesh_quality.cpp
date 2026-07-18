#include "gspl_sprites/mesh_quality.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <span>
#include <stdexcept>

namespace gspl::sprites {
namespace {
struct Point {
  long double x{}, y{};
};
struct TriangleUv {
  std::array<Point, 3> p;
  long double min_x{}, max_x{}, min_y{}, max_y{};
};
long double cross(Point a, Point b, Point c) {
  return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}
long double area(std::span<const Point> polygon) {
  long double value{};
  for (std::size_t i = 0; i < polygon.size(); ++i) {
    const auto &a = polygon[i];
    const auto &b = polygon[(i + 1) % polygon.size()];
    value += a.x * b.y - a.y * b.x;
  }
  return std::abs(value) / 2;
}
std::vector<Point> clip(std::vector<Point> polygon, const TriangleUv &clipper) {
  const auto sign =
      cross(clipper.p[0], clipper.p[1], clipper.p[2]) >= 0 ? 1.0L : -1.0L;
  for (int edge = 0; edge < 3 && !polygon.empty(); ++edge) {
    const auto q0 = clipper.p[edge], q1 = clipper.p[(edge + 1) % 3];
    std::vector<Point> output;
    for (std::size_t i = 0; i < polygon.size(); ++i) {
      const auto a = polygon[i], b = polygon[(i + 1) % polygon.size()];
      const auto da = cross(q0, q1, a) * sign, db = cross(q0, q1, b) * sign;
      const bool inside_a = da >= 0, inside_b = db >= 0;
      if (inside_a)
        output.push_back(a);
      if (inside_a != inside_b) {
        const auto denominator = da - db;
        if (denominator != 0) {
          const auto t = da / denominator;
          output.push_back({a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t});
        }
      }
    }
    polygon = std::move(output);
  }
  return polygon;
}
void add(ValidationResult &r, std::string code, std::string message) {
  r.diagnostics.push_back({std::move(code), std::move(message)});
}
} // namespace

MeshQualityReport analyze_mesh_quality(const Mesh3d &mesh,
                                       const MeshQualityPolicy &policy) {
  if (policy.maximum_triangles == 0 || policy.maximum_uv_candidate_pairs == 0)
    throw std::invalid_argument("mesh quality limits must be positive");
  if (mesh.triangle_indices.size() % 3 != 0)
    throw std::invalid_argument(
        "mesh quality requires a complete triangle index buffer");
  MeshQualityReport report;
  report.triangle_count = mesh.triangle_indices.size() / 3;
  if (report.triangle_count > policy.maximum_triangles)
    throw std::length_error("mesh quality triangle limit exceeded");
  std::vector<TriangleUv> triangles;
  triangles.reserve(report.triangle_count);
  std::vector<std::array<long double, 3>> tangent_sum(mesh.vertices.size());
  std::vector<std::array<long double, 3>> bitangent_sum(mesh.vertices.size());
  for (std::size_t i = 0; i < mesh.triangle_indices.size(); i += 3) {
    const auto ia = mesh.triangle_indices[i], ib = mesh.triangle_indices[i + 1],
               ic = mesh.triangle_indices[i + 2];
    if (ia >= mesh.vertices.size() || ib >= mesh.vertices.size() ||
        ic >= mesh.vertices.size())
      throw std::invalid_argument(
          "mesh quality triangle index is out of range");
    const auto &a = mesh.vertices[ia];
    const auto &b = mesh.vertices[ib];
    const auto &c = mesh.vertices[ic];
    TriangleUv uv{{Point{static_cast<long double>(a.texture_coordinate.u),
                         static_cast<long double>(a.texture_coordinate.v)},
                   Point{static_cast<long double>(b.texture_coordinate.u),
                         static_cast<long double>(b.texture_coordinate.v)},
                   Point{static_cast<long double>(c.texture_coordinate.u),
                         static_cast<long double>(c.texture_coordinate.v)}}};
    uv.min_x = std::min({uv.p[0].x, uv.p[1].x, uv.p[2].x});
    uv.max_x = std::max({uv.p[0].x, uv.p[1].x, uv.p[2].x});
    uv.min_y = std::min({uv.p[0].y, uv.p[1].y, uv.p[2].y});
    uv.max_y = std::max({uv.p[0].y, uv.p[1].y, uv.p[2].y});
    triangles.push_back(uv);
    const long double e1x =
                          static_cast<long double>(b.position.x) - a.position.x,
                      e1y =
                          static_cast<long double>(b.position.y) - a.position.y,
                      e1z =
                          static_cast<long double>(b.position.z) - a.position.z,
                      e2x =
                          static_cast<long double>(c.position.x) - a.position.x,
                      e2y =
                          static_cast<long double>(c.position.y) - a.position.y,
                      e2z =
                          static_cast<long double>(c.position.z) - a.position.z;
    const long double nx = e1y * e2z - e1z * e2y, ny = e1z * e2x - e1x * e2z,
                      nz = e1x * e2y - e1y * e2x;
    const long double authored_x = static_cast<long double>(a.normal.x) +
                                   b.normal.x + c.normal.x,
                      authored_y = static_cast<long double>(a.normal.y) +
                                   b.normal.y + c.normal.y,
                      authored_z = static_cast<long double>(a.normal.z) +
                                   b.normal.z + c.normal.z;
    if (nx * authored_x + ny * authored_y + nz * authored_z <= 0)
      ++report.inconsistent_normal_triangles;
    const long double du1 = uv.p[1].x - uv.p[0].x, dv1 = uv.p[1].y - uv.p[0].y,
                      du2 = uv.p[2].x - uv.p[0].x, dv2 = uv.p[2].y - uv.p[0].y,
                      det = du1 * dv2 - du2 * dv1;
    if (det == 0) {
      ++report.degenerate_uv_triangles;
      continue;
    }
    const auto inverse = 1.0L / det;
    const std::array<long double, 3> tangent{(e1x * dv2 - e2x * dv1) * inverse,
                                             (e1y * dv2 - e2y * dv1) * inverse,
                                             (e1z * dv2 - e2z * dv1) * inverse};
    const std::array<long double, 3> bitangent{
        (e2x * du1 - e1x * du2) * inverse, (e2y * du1 - e1y * du2) * inverse,
        (e2z * du1 - e1z * du2) * inverse};
    for (auto index : {ia, ib, ic})
      for (int k = 0; k < 3; ++k) {
        tangent_sum[index][k] += tangent[k];
        bitangent_sum[index][k] += bitangent[k];
      }
  }
  std::vector<std::size_t> order(triangles.size());
  for (std::size_t i = 0; i < order.size(); ++i)
    order[i] = i;
  std::ranges::sort(order, [&](auto l, auto r) {
    return triangles[l].min_x < triangles[r].min_x;
  });
  std::vector<std::size_t> active;
  for (auto index : order) {
    const auto &current = triangles[index];
    active.erase(std::remove_if(active.begin(), active.end(),
                                [&](auto prior) {
                                  return triangles[prior].max_x <=
                                         current.min_x;
                                }),
                 active.end());
    for (auto prior : active) {
      const auto &other = triangles[prior];
      if (other.max_y <= current.min_y || current.max_y <= other.min_y)
        continue;
      if (++report.uv_candidate_pairs > policy.maximum_uv_candidate_pairs)
        throw std::length_error("mesh quality UV candidate limit exceeded");
      std::vector<Point> polygon{current.p.begin(), current.p.end()};
      if (area(clip(std::move(polygon), other)) > 0.5L)
        ++report.uv_overlap_pairs;
    }
    active.push_back(index);
  }
  report.tangents.reserve(mesh.vertices.size());
  for (std::size_t i = 0; i < mesh.vertices.size(); ++i) {
    const auto &n = mesh.vertices[i].normal;
    const long double nx = n.x / 1e6L, ny = n.y / 1e6L, nz = n.z / 1e6L;
    auto tx = tangent_sum[i][0], ty = tangent_sum[i][1], tz = tangent_sum[i][2];
    const auto projection = tx * nx + ty * ny + tz * nz;
    tx -= projection * nx;
    ty -= projection * ny;
    tz -= projection * nz;
    const auto length = std::sqrt(tx * tx + ty * ty + tz * tz);
    if (length == 0) {
      report.tangents.push_back({});
      continue;
    }
    tx /= length;
    ty /= length;
    tz /= length;
    const auto cx = ny * tz - nz * ty, cy = nz * tx - nx * tz,
               cz = nx * ty - ny * tx;
    const auto handedness =
        (cx * bitangent_sum[i][0] + cy * bitangent_sum[i][1] +
         cz * bitangent_sum[i][2]) < 0
            ? -1
            : 1;
    report.tangents.push_back(
        {static_cast<std::int32_t>(std::llround(tx * 1e6L)),
         static_cast<std::int32_t>(std::llround(ty * 1e6L)),
         static_cast<std::int32_t>(std::llround(tz * 1e6L)), handedness});
  }
  if (policy.require_non_overlapping_uvs && report.uv_overlap_pairs)
    add(report.validation, "SPRITE_3D_UV_OVERLAP",
        "mesh contains positive-area UV overlap");
  if (policy.require_non_degenerate_uvs && report.degenerate_uv_triangles)
    add(report.validation, "SPRITE_3D_UV_DEGENERATE",
        "mesh contains degenerate UV triangles");
  if (policy.require_consistent_normals && report.inconsistent_normal_triangles)
    add(report.validation, "SPRITE_3D_NORMAL_ORIENTATION",
        "mesh normals oppose geometric face orientation");
  return report;
}
} // namespace gspl::sprites
