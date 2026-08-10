#pragma once

#include "gspl_sprites/common.hpp"

#include <cmath>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace gspl::sprites::visual {

/* Native vector geometry foundation.
 * All geometry is deterministic and bounded; every flattening and
 * tessellation routine fails closed on invalid input. This module is
 * the geometry authority for the Native Visual Core: renderer backends
 * consume flattened polylines plus the signed-distance helpers below
 * and never invent geometry themselves. */

struct Vec2 {
  double x{};
  double y{};
  constexpr Vec2() = default;
  constexpr Vec2(double px, double py) : x(px), y(py) {}
  constexpr Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
  constexpr Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
  constexpr Vec2 operator*(double s) const { return {x * s, y * s}; }
  constexpr Vec2 operator/(double s) const { return {x / s, y / s}; }
  constexpr Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
  constexpr Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
  [[nodiscard]] constexpr double dot(const Vec2& o) const { return x * o.x + y * o.y; }
  [[nodiscard]] constexpr double cross(const Vec2& o) const { return x * o.y - y * o.x; }
  [[nodiscard]] double length() const { return std::sqrt(x * x + y * y); }
  [[nodiscard]] double length_sq() const { return x * x + y * y; }
  [[nodiscard]] Vec2 normalized() const {
    const double l = length();
    if (!(l > 0.0) || !std::isfinite(l)) return {0.0, 0.0};
    return {x / l, y / l};
  }
};

struct Bounds {
  double min_x{0.0};
  double min_y{0.0};
  double max_x{-1.0};
  double max_y{-1.0};

  [[nodiscard]] static Bounds empty() { return {}; }
  [[nodiscard]] bool is_empty() const { return max_x < min_x || max_y < min_y; }
  void include(double px, double py) {
    if (is_empty()) { min_x = max_x = px; min_y = max_y = py; return; }
    min_x = (px < min_x) ? px : min_x;
    max_x = (px > max_x) ? px : max_x;
    min_y = (py < min_y) ? py : min_y;
    max_y = (py > max_y) ? py : max_y;
  }
  void include(const Vec2& p) { include(p.x, p.y); }
  void inflate(double d) {
    if (is_empty()) return;
    min_x -= d; min_y -= d; max_x += d; max_y += d;
  }
  [[nodiscard]] bool contains(double px, double py) const {
    return !is_empty() && px >= min_x && px <= max_x && py >= min_y && py <= max_y;
  }
  [[nodiscard]] double width() const { return is_empty() ? 0.0 : max_x - min_x; }
  [[nodiscard]] double height() const { return is_empty() ? 0.0 : max_y - min_y; }
};

/* Affine 2D transform: x' = a*x + c*y + e, y' = b*x + d*y + f. */
struct AffineTransform {
  double a{1.0};
  double b{0.0};
  double c{0.0};
  double d{1.0};
  double e{0.0};
  double f{0.0};

  [[nodiscard]] static AffineTransform identity() { return {}; }
  [[nodiscard]] static AffineTransform translation(double tx, double ty);
  [[nodiscard]] static AffineTransform rotation_degrees(double degrees);
  [[nodiscard]] static AffineTransform scaling(double sx, double sy);
  [[nodiscard]] Vec2 apply(const Vec2& p) const {
    return {a * p.x + c * p.y + e, b * p.x + d * p.y + f};
  }
  [[nodiscard]] AffineTransform composed_with(const AffineTransform& other) const;
  [[nodiscard]] std::optional<AffineTransform> inverse() const;
};

enum class SegmentKind { move_to, line_to, quad_to, cubic_to, close };

struct PathSegment {
  SegmentKind kind{SegmentKind::move_to};
  Vec2 c1;
  Vec2 c2;
  Vec2 p;
};

struct Path {
  std::vector<PathSegment> segments;
  [[nodiscard]] bool ends_with_close() const;
};

struct FlattenOptions {
  double tolerance{0.25};
  std::uint32_t max_segments{4096};
  std::uint32_t max_subdivisions{12};
};

struct FlattenResult {
  std::vector<Vec2> points;
  std::vector<std::uint32_t> subpath_starts;
  bool ok{true};
  std::string error;
};

[[nodiscard]] FlattenResult flatten_path(const Path& path, const FlattenOptions& options = {});

[[nodiscard]] double polygon_signed_area(std::span<const Vec2> polygon);
[[nodiscard]] bool point_in_polygon_even_odd(const Vec2& p, std::span<const Vec2> polygon);
[[nodiscard]] double distance_point_segment(const Vec2& p, const Vec2& a, const Vec2& b,
                                            Vec2* out_closest = nullptr, double* out_param = nullptr);

struct ClosestPolylineResult {
  double distance{};
  double t{};
  std::uint32_t segment_index{};
  Vec2 closest;
};

[[nodiscard]] ClosestPolylineResult closest_on_polyline(const Vec2& p, std::span<const Vec2> polyline);
[[nodiscard]] ClosestPolylineResult closest_on_closed_polyline(const Vec2& p, std::span<const Vec2> polygon);
[[nodiscard]] double interpolate_width(std::span<const double> widths, double t);

[[nodiscard]] std::vector<Vec2> ellipse_polyline(double rx, double ry, std::uint32_t segments = 64);
[[nodiscard]] std::vector<Vec2> capsule_polyline(double half_len, double radius, std::uint32_t arc_segments = 24);
[[nodiscard]] std::vector<Vec2> rounded_rect_polyline(double half_w, double half_h, double radius,
                                                      std::uint32_t arc_segments = 16);

/* Deterministic LCG for procedural visual generation (markings, textures). */
struct Lcg32 {
  std::uint32_t state{1};
  explicit Lcg32(std::uint64_t seed) : state(static_cast<std::uint32_t>(seed ^ 0x9E3779B9ULL)) {
    if (state == 0) state = 0x1234567u;
  }
  [[nodiscard]] std::uint32_t next() {
    state = state * 1664525u + 1013904223u;
    return state;
  }
  [[nodiscard]] double unit() { return static_cast<double>(next() >> 8) / static_cast<double>(0xFFFFFFu); }
};

} // namespace gspl::sprites::visual
