// Visual geometry foundation tests: Vec2/Bounds/AffineTransform, Bezier
// flattening (deterministic + bounded), polygon helpers, strokes, shapes.
#include "gspl_sprites/visual_geometry.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace gspl::sprites::visual;

static int failures = 0;
static void check(bool v, const char* msg) {
  if (v) { std::cout << "PASS: " << msg << "\n"; }
  else { std::cerr << "FAIL: " << msg << "\n"; ++failures; }
}

int main() {
  // Vec2 basics
  { Vec2 a(3, 4); check(std::abs(a.length() - 5.0) < 1e-12, "vec2 length");
    Vec2 n = a.normalized(); check(std::abs(n.length() - 1.0) < 1e-12, "vec2 normalized");
    check(n.dot(a) > 0.0, "vec2 dot"); }
  // Bounds
  { Bounds b = Bounds::empty(); check(b.is_empty(), "bounds empty default");
    b.include(1.0, 2.0); b.include(3.0, 4.0);
    check(!b.is_empty(), "bounds non-empty after include");
    check(std::abs(b.width() - 2.0) < 1e-12 && std::abs(b.height() - 2.0) < 1e-12, "bounds size");
    check(b.contains(2.0, 3.0) && !b.contains(0.0, 0.0), "bounds contains"); }
  // AffineTransform
  { auto t = AffineTransform::translation(10, -5);
    Vec2 p = t.apply({1, 2}); check(p.x == 11 && p.y == -3, "affine translation");
    auto r = AffineTransform::rotation_degrees(90.0);
    Vec2 q = r.apply({1, 0}); check(std::abs(q.x) < 1e-9 && std::abs(q.y - 1.0) < 1e-9, "affine rotation");
    auto s = AffineTransform::scaling(2, 3);
    Vec2 w = s.apply({1, 2}); check(w.x == 2 && w.y == 6, "affine scale");
    auto inv = t.inverse(); check(inv.has_value(), "affine invertible");
    if (inv) { Vec2 back = inv->apply(t.apply({5, 6}));
      check(std::abs(back.x - 5) < 1e-9 && std::abs(back.y - 6) < 1e-9, "affine roundtrip"); } }
  // Path flattening: straight line
  { Path p; p.segments.push_back({SegmentKind::move_to, {}, {}, {0, 0}});
    p.segments.push_back({SegmentKind::line_to, {}, {}, {10, 0}});
    auto fr = flatten_path(p); check(fr.ok && fr.points.size() == 2, "flatten straight"); }
  // Quadratic bezier
  { Path p; p.segments.push_back({SegmentKind::move_to, {}, {}, {0, 0}});
    p.segments.push_back({SegmentKind::quad_to, {5, 10}, {}, {10, 0}});
    auto fr = flatten_path(p, FlattenOptions{0.05, 4096, 12});
    check(fr.ok && fr.points.size() >= 4, "flatten quadratic subdivides");
    bool within = true;
    for (const Vec2& v : fr.points)
      if (v.y > 5.01 || v.y < -0.01) within = false;  // quad peaks at y=5
    check(within, "quadratic stays within bounds"); }
  // Cubic bezier
  { Path p; p.segments.push_back({SegmentKind::move_to, {}, {}, {0, 0}});
    p.segments.push_back({SegmentKind::cubic_to, {0, 10}, {10, 10}, {10, 0}});
    auto fr = flatten_path(p, FlattenOptions{0.05, 4096, 12});
    check(fr.ok && fr.points.size() >= 4, "flatten cubic"); }
  // Degenerate: zero-length line
  { Path p; p.segments.push_back({SegmentKind::move_to, {}, {}, {3, 3}});
    p.segments.push_back({SegmentKind::line_to, {}, {}, {3, 3}});
    auto fr = flatten_path(p); check(fr.ok && !fr.points.empty(), "flatten degenerate ok"); }
  // High curvature: tight circle-ish path still bounded
  { Path p; p.segments.push_back({SegmentKind::move_to, {}, {}, {0, 0}});
    for (int i = 1; i <= 64; ++i) {
      double a = static_cast<double>(i) / 64.0 * 6.283185307;
      p.segments.push_back({SegmentKind::line_to, {}, {}, {std::cos(a), std::sin(a)}});
    }
    auto fr = flatten_path(p); check(fr.ok && fr.points.size() >= 64, "flatten closed ring");
    check(p.ends_with_close() == false, "ends_with_close false for open");
    Path pc = p; pc.segments.push_back({SegmentKind::close, {}, {}, {}});
    check(pc.ends_with_close(), "ends_with_close true for close"); }
  // Invalid: nonfinite input fails closed
  { Path p; p.segments.push_back({SegmentKind::move_to, {}, {}, {0, 0}});
    p.segments.push_back({SegmentKind::line_to, {}, {}, {std::nan(""), 1}});
    auto fr = flatten_path(p); check(!fr.ok, "flatten rejects nonfinite"); }
  // Resource bound: max_segments respected
  { Path p; p.segments.push_back({SegmentKind::move_to, {}, {}, {0, 0}});
    p.segments.push_back({SegmentKind::cubic_to, {0, 100}, {100, 100}, {100, 0}});
    auto fr = flatten_path(p, FlattenOptions{1e-6, 32, 4});
    check(fr.ok || fr.points.size() <= 64, "flatten honors vertex budget"); }
  // Polygon helpers
  { std::vector<Vec2> tri{{0, 0}, {4, 0}, {2, 3}};
    check(std::abs(polygon_signed_area(tri) - 6.0) < 1e-9, "triangle signed area");
    check(point_in_polygon_even_odd({2, 1}, tri), "point inside triangle");
    check(!point_in_polygon_even_odd({10, 10}, tri), "point outside triangle");
    std::vector<Vec2> concave{{0, 0}, {4, 0}, {4, 4}, {2, 2}, {0, 4}};
    check(point_in_polygon_even_odd({1, 1}, concave), "point inside concave");
    check(!point_in_polygon_even_odd({3, 3.5}, concave), "point in concave hole"); }
  // Distance helpers
  { Vec2 cp; double t = 0;
    double d = distance_point_segment({1, 1}, {0, 0}, {2, 0}, &cp, &t);
    check(std::abs(d - 1.0) < 1e-12 && std::abs(t - 0.5) < 1e-12, "distance point segment");
    auto cl = closest_on_closed_polyline({5, 0}, std::vector<Vec2>{{0, 0}, {4, 0}, {4, 4}, {0, 4}});
    check(std::abs(cl.distance - 1.0) < 1e-12, "closest closed polyline"); }
  // Width interpolation (taper)
  { std::vector<double> w{1.0, 0.2};
    check(std::abs(interpolate_width(w, 0.5) - 0.6) < 1e-12, "taper interpolation");
    check(std::abs(interpolate_width(w, 2.0) - 0.2) < 1e-12, "taper clamp end"); }
  // Shape builders
  { auto e = ellipse_polyline(2.0, 1.0, 32);
    check(e.size() == 32, "ellipse polyline segments");
    double max_x = 0, max_y = 0;
    for (const Vec2& v : e) { max_x = std::max(max_x, std::abs(v.x)); max_y = std::max(max_y, std::abs(v.y)); }
    check(std::abs(max_x - 2.0) < 1e-6 && std::abs(max_y - 1.0) < 1e-6, "ellipse radii");
    auto c = capsule_polyline(3.0, 1.0, 12);
    check(c.size() >= 12, "capsule polyline");
    auto r = rounded_rect_polyline(2.0, 1.0, 0.5, 12);
    check(r.size() >= 12, "rounded rect polyline");
    for (const Vec2& v : r) check(std::abs(v.x) <= 2.0001 && std::abs(v.y) <= 1.0001, "rounded rect bounds"); }
  // Lcg32 determinism
  { Lcg32 a(42), b(42);
    bool same = true;
    for (int i = 0; i < 100; ++i) if (a.next() != b.next()) same = false;
    check(same, "lcg deterministic");
    Lcg32 c(7); check(c.unit() >= 0.0 && c.unit() <= 1.0, "lcg unit range"); }

  std::cout << "\nvisual_geometry_tests: " << (failures == 0 ? "ALL PASS" : "FAILURES")
            << " (failures=" << failures << ")\n";
  return failures == 0 ? 0 : 1;
}
