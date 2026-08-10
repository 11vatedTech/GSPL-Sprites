#include "gspl_sprites/visual_geometry.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace gspl::sprites::visual {
namespace {

constexpr double kPi = 3.14159265358979323846264338327950288;
constexpr double kDegToRad = kPi / 180.0;
constexpr double kEps = 1e-12;

[[nodiscard]] double dist_point_to_line(const Vec2& p, const Vec2& a, const Vec2& b) {
  const Vec2 ab = b - a;
  const double len2 = ab.length_sq();
  if (len2 <= kEps) return (p - a).length();
  const double t = std::clamp(((p - a).dot(ab)) / len2, 0.0, 1.0);
  return (p - (a + ab * t)).length();
}

void subdivide_quad(const Vec2& p0, const Vec2& c1, const Vec2& p1,
                    double tol, std::uint32_t depth, std::uint32_t max_depth,
                    std::uint32_t& budget, std::vector<Vec2>& out, bool& exceeded) {
  if (depth >= max_depth) { out.push_back(p1); return; }
  if (dist_point_to_line(c1, p0, p1) <= tol) { out.push_back(p1); return; }
  if (budget == 0) { exceeded = true; out.push_back(p1); return; }
  --budget;
  const Vec2 m01 = (p0 + c1) * 0.5;
  const Vec2 m12 = (c1 + p1) * 0.5;
  const Vec2 m = (m01 + m12) * 0.5;
  subdivide_quad(p0, m01, m, tol, depth + 1, max_depth, budget, out, exceeded);
  subdivide_quad(m, m12, p1, tol, depth + 1, max_depth, budget, out, exceeded);
}

void subdivide_cubic(const Vec2& p0, const Vec2& c1, const Vec2& c2, const Vec2& p1,
                     double tol, std::uint32_t depth, std::uint32_t max_depth,
                     std::uint32_t& budget, std::vector<Vec2>& out, bool& exceeded) {
  if (depth >= max_depth) { out.push_back(p1); return; }
  const double d1 = dist_point_to_line(c1, p0, p1);
  const double d2 = dist_point_to_line(c2, p0, p1);
  if (d1 <= tol && d2 <= tol) { out.push_back(p1); return; }
  if (budget == 0) { exceeded = true; out.push_back(p1); return; }
  --budget;
  const Vec2 m01 = (p0 + c1) * 0.5;
  const Vec2 m12 = (c1 + c2) * 0.5;
  const Vec2 m23 = (c2 + p1) * 0.5;
  const Vec2 m012 = (m01 + m12) * 0.5;
  const Vec2 m123 = (m12 + m23) * 0.5;
  const Vec2 m = (m012 + m123) * 0.5;
  subdivide_cubic(p0, m01, m012, m, tol, depth + 1, max_depth, budget, out, exceeded);
  subdivide_cubic(m, m123, m23, p1, tol, depth + 1, max_depth, budget, out, exceeded);
}

} // namespace

AffineTransform AffineTransform::translation(double tx, double ty) {
  return AffineTransform{1.0, 0.0, 0.0, 1.0, tx, ty};
}

AffineTransform AffineTransform::rotation_degrees(double degrees) {
  const double rad = degrees * kDegToRad;
  const double cs = std::cos(rad);
  const double sn = std::sin(rad);
  return AffineTransform{cs, sn, -sn, cs, 0.0, 0.0};
}

AffineTransform AffineTransform::scaling(double sx, double sy) {
  return AffineTransform{sx, 0.0, 0.0, sy, 0.0, 0.0};
}

AffineTransform AffineTransform::composed_with(const AffineTransform& o) const {
  return AffineTransform{
      a * o.a + c * o.b,
      b * o.a + d * o.b,
      a * o.c + c * o.d,
      b * o.c + d * o.d,
      a * o.e + c * o.f + e,
      b * o.e + d * o.f + f};
}

std::optional<AffineTransform> AffineTransform::inverse() const {
  const double det = a * d - b * c;
  if (!(std::abs(det) > kEps)) return std::nullopt;
  const double ia = d / det;
  const double ib = -b / det;
  const double ic = -c / det;
  const double id = a / det;
  const double ie = -(ia * e + ic * f);
  const double iff = -(ib * e + id * f);
  return AffineTransform{ia, ib, ic, id, ie, iff};
}

bool Path::ends_with_close() const {
  return !segments.empty() && segments.back().kind == SegmentKind::close;
}

FlattenResult flatten_path(const Path& path, const FlattenOptions& options) {
  FlattenResult result;
  std::uint32_t budget = options.max_segments;
  bool exceeded = false;
  Vec2 current;
  bool have_current = false;
  bool subpath_open = false;

  auto begin_subpath = [&](const Vec2& p) {
    result.subpath_starts.push_back(static_cast<std::uint32_t>(result.points.size()));
    result.points.push_back(p);
    current = p;
    have_current = true;
    subpath_open = true;
  };

  auto finite_p = [](const Vec2& v) { return std::isfinite(v.x) && std::isfinite(v.y); };
  for (const PathSegment& seg : path.segments) {
    if (!finite_p(seg.p) || !finite_p(seg.c1) || !finite_p(seg.c2)) {
      result.ok = false;
      result.error = "flatten path contains non-finite geometry";
      return result;
    }
    switch (seg.kind) {
      case SegmentKind::move_to: {
        begin_subpath(seg.p);
        break;
      }
      case SegmentKind::line_to: {
        if (!have_current) begin_subpath(seg.p);
        else {
          if (budget == 0) exceeded = true;
          else --budget;
          result.points.push_back(seg.p);
          current = seg.p;
        }
        break;
      }
      case SegmentKind::quad_to: {
        if (!have_current) { begin_subpath(seg.p); break; }
        subdivide_quad(current, seg.c1, seg.p, options.tolerance, 0, options.max_subdivisions,
                       budget, result.points, exceeded);
        current = seg.p;
        break;
      }
      case SegmentKind::cubic_to: {
        if (!have_current) { begin_subpath(seg.p); break; }
        subdivide_cubic(current, seg.c1, seg.c2, seg.p, options.tolerance, 0, options.max_subdivisions,
                        budget, result.points, exceeded);
        current = seg.p;
        break;
      }
      case SegmentKind::close: {
        if (have_current && subpath_open && !result.points.empty()) {
          const Vec2 start = result.points[result.subpath_starts.back()];
          if ((current - start).length_sq() > kEps) result.points.push_back(start);
        }
        subpath_open = false;
        have_current = false;
        break;
      }
    }
  }
  if (exceeded) {
    result.ok = false;
    result.error = "flatten budget exceeded (max segments " + std::to_string(options.max_segments) + ")";
  }
  return result;
}

double polygon_signed_area(std::span<const Vec2> polygon) {
  double area = 0.0;
  const std::size_t n = polygon.size();
  for (std::size_t i = 0; i < n; ++i) {
    const std::size_t j = (i + 1) % n;
    area += polygon[i].x * polygon[j].y - polygon[j].x * polygon[i].y;
  }
  return area * 0.5;
}

bool point_in_polygon_even_odd(const Vec2& p, std::span<const Vec2> polygon) {
  const std::size_t n = polygon.size();
  bool inside = false;
  for (std::size_t i = 0, j = n - 1; i < n; j = i++) {
    const Vec2& a = polygon[i];
    const Vec2& b = polygon[j];
    const bool a_below = a.y > p.y;
    const bool b_below = b.y > p.y;
    if (a_below != b_below) {
      const double x_cross = (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x;
      if (p.x < x_cross) inside = !inside;
    }
  }
  return inside;
}

double distance_point_segment(const Vec2& p, const Vec2& a, const Vec2& b,
                              Vec2* out_closest, double* out_param) {
  const Vec2 ab = b - a;
  const double len2 = ab.length_sq();
  double t = 0.0;
  if (len2 > kEps) t = std::clamp(((p - a).dot(ab)) / len2, 0.0, 1.0);
  const Vec2 closest = a + ab * t;
  if (out_closest) *out_closest = closest;
  if (out_param) *out_param = t;
  return (p - closest).length();
}

ClosestPolylineResult closest_on_polyline(const Vec2& p, std::span<const Vec2> polyline) {
  ClosestPolylineResult best;
  best.distance = std::numeric_limits<double>::infinity();
  const std::size_t n = polyline.size();
  for (std::size_t i = 0; i + 1 < n; ++i) {
    Vec2 closest;
    double param = 0.0;
    const double d = distance_point_segment(p, polyline[i], polyline[i + 1], &closest, &param);
    if (d < best.distance) {
      best.distance = d;
      best.closest = closest;
      best.segment_index = static_cast<std::uint32_t>(i);
      best.t = static_cast<double>(i) + param;
    }
  }
  return best;
}

ClosestPolylineResult closest_on_closed_polyline(const Vec2& p, std::span<const Vec2> polygon) {
  ClosestPolylineResult best;
  best.distance = std::numeric_limits<double>::infinity();
  const std::size_t n = polygon.size();
  for (std::size_t i = 0; i < n; ++i) {
    const std::size_t j = (i + 1) % n;
    Vec2 closest;
    double param = 0.0;
    const double d = distance_point_segment(p, polygon[i], polygon[j], &closest, &param);
    if (d < best.distance) {
      best.distance = d;
      best.closest = closest;
      best.segment_index = static_cast<std::uint32_t>(i);
      best.t = static_cast<double>(i) + param;
    }
  }
  return best;
}

double interpolate_width(std::span<const double> widths, double t) {
  const std::size_t n = widths.size();
  if (n == 0) return 0.0;
  if (n == 1) return widths[0];
  if (t <= 0.0) return widths[0];
  if (t >= static_cast<double>(n - 1)) return widths[n - 1];
  const std::size_t i = static_cast<std::size_t>(t);
  const double frac = t - static_cast<double>(i);
  return widths[i] + (widths[i + 1] - widths[i]) * frac;
}

std::vector<Vec2> ellipse_polyline(double rx, double ry, std::uint32_t segments) {
  std::vector<Vec2> pts;
  pts.reserve(segments);
  const std::uint32_t n = (std::max)(segments, 4u);
  for (std::uint32_t i = 0; i < n; ++i) {
    const double ang = 2.0 * kPi * static_cast<double>(i) / static_cast<double>(n);
    pts.emplace_back(rx * std::cos(ang), ry * std::sin(ang));
  }
  return pts;
}

std::vector<Vec2> capsule_polyline(double half_len, double radius, std::uint32_t arc_segments) {
  std::vector<Vec2> pts;
  const std::uint32_t arcs = (std::max)(arc_segments, 4u);
  pts.reserve(arcs * 2 + 2);
  for (std::uint32_t i = 0; i <= arcs; ++i) {
    const double ang = (90.0 + 180.0 * static_cast<double>(i) / static_cast<double>(arcs)) * kDegToRad;
    pts.emplace_back(-half_len + radius * std::cos(ang), radius * std::sin(ang));
  }
  pts.emplace_back(half_len, -radius);
  for (std::uint32_t i = 1; i <= arcs; ++i) {
    const double ang = (270.0 + 180.0 * static_cast<double>(i) / static_cast<double>(arcs)) * kDegToRad;
    if (ang >= 2.0 * kPi) continue;
    pts.emplace_back(half_len + radius * std::cos(ang), radius * std::sin(ang));
  }
  pts.emplace_back(-half_len, radius);
  return pts;
}

std::vector<Vec2> rounded_rect_polyline(double half_w, double half_h, double radius,
                                        std::uint32_t arc_segments) {
  const double r = std::clamp(radius, 0.0, (std::min)(half_w, half_h));
  const std::uint32_t arcs = (std::max)(arc_segments, 4u);
  std::vector<Vec2> pts;
  if (r <= kEps) {
    pts.emplace_back(-half_w, -half_h);
    pts.emplace_back(half_w, -half_h);
    pts.emplace_back(half_w, half_h);
    pts.emplace_back(-half_w, half_h);
    return pts;
  }
  const double cx = half_w - r;
  const double cy = half_h - r;
  pts.reserve(arcs * 4);
  auto arc = [&](double cx0, double cy0, double a0, double a1) {
    for (std::uint32_t i = 0; i < arcs; ++i) {
      const double ang = (a0 + (a1 - a0) * static_cast<double>(i) / static_cast<double>(arcs)) * kDegToRad;
      pts.emplace_back(cx0 + r * std::cos(ang), cy0 + r * std::sin(ang));
    }
  };
  arc(-cx, -cy, 180.0, 270.0);
  arc(cx, -cy, 270.0, 360.0);
  arc(cx, cy, 0.0, 90.0);
  arc(-cx, cy, 90.0, 180.0);
  return pts;
}

} // namespace gspl::sprites::visual
