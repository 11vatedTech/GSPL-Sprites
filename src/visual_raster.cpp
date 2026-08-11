#include "gspl_sprites/material.hpp"
#include "gspl_sprites/morphology.hpp"
#include "gspl_sprites/palette.hpp"
#include "gspl_sprites/visual_raster.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <sstream>

namespace gspl::sprites::visual {
namespace {

constexpr double kPi = 3.14159265358979323846264338327950288;
constexpr double kPosScale = 3.0;   // v1 position unit -> pixels (legacy-compatible)
constexpr double kSizeScale = 1.5;  // v1 size unit -> pixel radius (legacy-compatible)

[[nodiscard]] bool finite(double v) { return std::isfinite(v); }

[[nodiscard]] bool finite_part(const VisualPart& p) {
  return finite(p.x) && finite(p.y) && finite(p.z) && finite(p.size_x) &&
         finite(p.size_y) && finite(p.rotation_degrees) && finite(p.opacity);
}

/* Packed 0xRRGGBBAA -> float channels. */
struct RgbaF { double r, g, b, a; };
[[nodiscard]] RgbaF unpack(std::uint32_t rgba) {
  return {static_cast<double>((rgba >> 24) & 0xFF) / 255.0,
          static_cast<double>((rgba >> 16) & 0xFF) / 255.0,
          static_cast<double>((rgba >> 8) & 0xFF) / 255.0,
          static_cast<double>(rgba & 0xFF) / 255.0};
}
[[nodiscard]] std::uint32_t pack(RgbaF c) {
  auto b = [](double v) -> std::uint8_t {
    return static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::lround(v * 255.0)), 0, 255));
  };
  return (static_cast<std::uint32_t>(b(c.r)) << 24) | (static_cast<std::uint32_t>(b(c.g)) << 16) |
         (static_cast<std::uint32_t>(b(c.b)) << 8) | static_cast<std::uint32_t>(b(c.a));
}

/* Deterministic screen-space hash for texture detail. */
[[nodiscard]] double hash2(std::int32_t x, std::int32_t y, std::uint32_t seed) {
  std::uint32_t h = seed;
  h ^= static_cast<std::uint32_t>(x) * 0x9E3779B1u;
  h ^= static_cast<std::uint32_t>(y) * 0x85EBCA77u;
  h ^= h >> 16; h *= 0x7FEB352Du; h ^= h >> 15;
  h *= 0x846CA68Bu; h ^= h >> 16;
  return static_cast<double>(h) / 4294967295.0;
}

/* World state of a part after hierarchy + performance accumulation. */
struct WorldPart {
  const VisualPart* part{nullptr};
  double cx{}, cy{};      // pixel-space center
  double rot_rad{};       // accumulated rotation
  double rx{}, ry{};      // pixel-space radii (size * scale)
  double glow{};          // 0..1 emission intensity
  std::int32_t layer_idx{};
  std::int32_t z_order{};
};

[[nodiscard]] double resolve_emission(const VisualPart& p, const MaterialSemantics& mat) {
  if (p.emissive) return 1.0;
  return std::clamp(mat.emission, 0.0, 1.0);
}

/* Local-space shape geometry (semantic units, y-up). */
struct ShapeGeom {
  std::vector<Vec2> fill;          // closed fill polygon (unit space)
  std::vector<Vec2> stroke;        // open/closed stroke polyline (unit space)
  std::vector<double> widths;      // per-vertex stroke widths (unit space)
  bool has_fill{true};
  bool has_stroke{false};
  bool closed{false};
};

[[nodiscard]] ShapeGeom build_shape(const VisualPart& p, const FlattenOptions& flat) {
  ShapeGeom g;
  const double hw = (p.size_x > 0.0) ? p.size_x : 1.0;
  const double hh = (p.size_y > 0.0) ? p.size_y : 1.0;
  switch (p.shape) {
    case VisualShapeKind::ellipse: {
      g.fill = ellipse_polyline(1.0, 1.0, 48);
      g.has_fill = true;
      break;
    }
    case VisualShapeKind::capsule: {
      // Extends along local Y: half-length hh, radius hw.
      g.fill = capsule_polyline(hh, hw, 24);
      g.has_fill = true;
      break;
    }
    case VisualShapeKind::rounded_rect: {
      const double radius = std::min(hw, hh) * 0.35;
      g.fill = rounded_rect_polyline(hw, hh, radius, 16);
      g.has_fill = true;
      break;
    }
    case VisualShapeKind::polygon: {
      if (p.polygon.size() >= 3) {
        g.fill = p.polygon;
        g.has_fill = true;
      } else {
        g.fill = ellipse_polyline(1.0, 1.0, 48);
        g.has_fill = true;
      }
      break;
    }
    case VisualShapeKind::ring: {
      g.fill = ellipse_polyline(1.0, 1.0, 64);
      g.has_fill = false;  // rendered as stroked ellipse
      g.stroke = ellipse_polyline(1.0, 1.0, 64);
      g.widths.assign(g.stroke.size(), 0.5);
      g.has_stroke = true;
      g.closed = true;
      break;
    }
    case VisualShapeKind::open_path:
    case VisualShapeKind::closed_path: {
      Path path = p.path;
      const bool is_closed = (p.shape == VisualShapeKind::closed_path) || path.ends_with_close();
      const FlattenResult fr = flatten_path(path, flat);
      if (fr.ok) {
        g.stroke = fr.points;
        g.has_stroke = true;
        g.closed = is_closed;
        g.widths = (p.stroke_widths.size() >= 2) ? p.stroke_widths
                    : std::vector<double>(fr.points.size(), 0.35);
        if (p.shape == VisualShapeKind::closed_path && g.closed && fr.points.size() >= 3) {
          g.fill = fr.points;
          g.has_fill = true;
        }
      }
      break;
    }
  }
  return g;
}

[[nodiscard]] bool segment_intersects_bbox(
    const Vec2& a, const Vec2& b, double min_x, double min_y, double max_x, double max_y) {
  auto out_code = [&](double x, double y) -> std::uint8_t {
    std::uint8_t c = 0;
    if (x < min_x) c |= 1; else if (x > max_x) c |= 2;
    if (y < min_y) c |= 4; else if (y > max_y) c |= 8;
    return c;
  };
  std::uint8_t ca = out_code(a.x, a.y), cb = out_code(b.x, b.y);
  if ((ca | cb) == 0) return true;
  if ((ca & cb) != 0) return false;
  return true;
}

[[nodiscard]] double clamp01(double v) { return std::clamp(v, 0.0, 1.0); }


/* Deterministic 2D lighting model: light direction, ambient/key/fill/rim
 * plus material response (roughness, metallicity, subsurface, texture).
 * Pure function of (pixel, part) - no state. */
struct LightingModel {
  double lx{}, ly{};   // unit light direction (screen space)
  double ambient{0.42}, key{0.55}, fill{0.18}, rim{0.15};
  std::uint32_t band_count{1};  // cel band quantization (1 = smooth)
};

[[nodiscard]] RgbaF shade_pixel(const LightingModel& L, const WorldPart& wp, const RgbaF& base,
                                const MaterialSemantics& mat, double coverage,
                                double sdf, double grad, std::int32_t px, std::int32_t py) {
  if (L.ambient == 0.0 && L.key == 0.0 && L.fill == 0.0 && L.rim == 0.0) return base;
  (void)sdf;
  double h = clamp01(grad);                      // light-axis gradient proxy
  // Cel band quantization: when band_count > 1, quantize the gradient
  // into discrete steps for a toon/cel look. Each band is a step function
  // of the continuous gradient.
  if (L.band_count > 1) {
    const double step = 1.0 / static_cast<double>(L.band_count);
    h = std::floor(h / step) * step + step * 0.5;
    h = clamp01(h);
  }
  const double lit = L.ambient + L.key * h + L.fill * (1.0 - h);
  double r = base.r * lit, g = base.g * lit, b = base.b * lit;
  // Roughness flattens shading toward the base.
  const double flat = clamp01(1.0 - mat.roughness * 0.55);
  r = r * flat + base.r * (1.0 - flat);
  g = g * flat + base.g * (1.0 - flat);
  b = b * flat + base.b * (1.0 - flat);
  // Specular (metal/glass/reflective).
  if (mat.reflectivity > 0.0) {
    const double spec = mat.reflectivity * std::pow(h, 10.0);
    const RgbaF tint = (mat.metallicity > 0.5) ? base : RgbaF{1, 1, 1, 1};
    r += spec * tint.r; g += spec * tint.g; b += spec * tint.b;
  }
  // Subsurface intent: warm fill glow at mid-gradient.
  if (mat.subsurface_intent > 0.0) {
    const double sub = mat.subsurface_intent * h * (1.0 - h) * 0.35;
    r += sub; g += sub * 0.7; b += sub * 0.4;
  }
  // Deterministic texture detail (fur/cloth): small value modulation.
  if (mat.surface_texture_scale > 0.0 && mat.roughness > 0.5) {
    const double n = hash2(px, py, 0x51u) * 2.0 - 1.0;
    const double amt = 0.035 * std::min(mat.surface_texture_scale, 4.0) * mat.roughness;
    const double mod = 1.0 + n * amt;
    r *= mod; g *= mod; b *= mod;
  }
  // Rim response on silhouette edges (coverage gradient near boundary).
  const double rim_amt = L.rim * mat.rim_response * coverage * (1.0 - coverage) * 4.0;
  r += rim_amt; g += rim_amt; b += rim_amt;
  (void)wp;
  return {std::max(0.0, r), std::max(0.0, g), std::max(0.0, b), base.a};
}

/* Renderer context: framebuffer, budgets, coverage buffer, layer table. */
struct RasterCtx {
  std::uint32_t w{}, h{};
  std::uint8_t* fb{nullptr};         // RGBA framebuffer (buffer owned by caller image)
  std::vector<double> cov;            // per-pixel accumulated coverage (for outlines)
  std::uint64_t work{0};
  std::uint64_t max_work{0};
  bool overrun{false};
  std::vector<std::int32_t> layer_index;  // layer name -> order
  LightingModel light;
  StyleSemantics style;
  PaletteDefinition palette;
  double aa_width{1.0};
  bool additive_layers{false};
  bool flat_color{false};  // channel stamping: no lighting

  [[nodiscard]] std::size_t idx(std::int32_t x, std::int32_t y) const {
    return (static_cast<std::size_t>(y) * w + static_cast<std::size_t>(x)) * 4;
  }

  void source_over(std::int32_t x, std::int32_t y, RgbaF c) {
    if (x < 0 || y < 0 || x >= static_cast<std::int32_t>(w) || y >= static_cast<std::int32_t>(h)) return;
    std::size_t i = idx(x, y);
    const double sa = std::clamp(c.a, 0.0, 1.0);
    const double da = static_cast<double>(fb[i + 3]) / 255.0;
    const double oa = sa + da * (1.0 - sa);
    if (oa <= 0.0) return;
    double r = (c.r * sa + static_cast<double>(fb[i]) / 255.0 * da * (1.0 - sa)) / oa;
    double g = (c.g * sa + static_cast<double>(fb[i + 1]) / 255.0 * da * (1.0 - sa)) / oa;
    double b = (c.b * sa + static_cast<double>(fb[i + 2]) / 255.0 * da * (1.0 - sa)) / oa;
    fb[i] = static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::lround(r * 255.0)), 0, 255));
    fb[i + 1] = static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::lround(g * 255.0)), 0, 255));
    fb[i + 2] = static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::lround(b * 255.0)), 0, 255));
    fb[i + 3] = static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::lround(oa * 255.0)), 0, 255));
    cov[static_cast<std::size_t>(y) * w + x] = std::max(cov[static_cast<std::size_t>(y) * w + x], oa);
  }

  void additive(std::int32_t x, std::int32_t y, RgbaF c, double intensity) {
    if (x < 0 || y < 0 || x >= static_cast<std::int32_t>(w) || y >= static_cast<std::int32_t>(h)) return;
    std::size_t i = idx(x, y);
    const double m = intensity * c.a;
    double r = static_cast<double>(fb[i]) / 255.0 + c.r * m;
    double g = static_cast<double>(fb[i + 1]) / 255.0 + c.g * m;
    double b = static_cast<double>(fb[i + 2]) / 255.0 + c.b * m;
    fb[i] = static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::lround(r * 255.0)), 0, 255));
    fb[i + 1] = static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::lround(g * 255.0)), 0, 255));
    fb[i + 2] = static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::lround(b * 255.0)), 0, 255));
    fb[i + 3] = 255;
  }
};


/* Map a screen pixel back into a part's local unit space (y-up). */
struct PartToScreen {
  double cx{}, cy{}, cos_r{1.0}, sin_r{0.0}, rx{1.0}, ry{1.0};
  [[nodiscard]] Vec2 to_local(double px, double py) const {
    const double dx = px - cx;
    const double dy = cy - py;   // screen y-down -> world y-up
    const double lx = (dx * cos_r - dy * sin_r) / rx;
    const double ly = (dx * sin_r + dy * cos_r) / ry;
    return {lx, ly};
  }
  [[nodiscard]] Vec2 to_screen(const Vec2& l) const {
    const double dx = l.x * rx, dy = l.y * ry;
    return {cx + dx * cos_r - dy * sin_r, cy - (dx * sin_r + dy * cos_r)};
  }
};

/* Rasterize a closed fill polygon with analytic AA. */
void raster_fill(RasterCtx& ctx, const PartToScreen& xf, std::span<const Vec2> unit_poly,
                 const RgbaF& base, double opacity, const MaterialSemantics& mat,
                 const LightingModel& L, double glow) {
  if (unit_poly.size() < 3) return;
  std::vector<Vec2> s(unit_poly.size());
  double min_x = std::numeric_limits<double>::infinity(), min_y = min_x;
  double max_x = -min_x, max_y = -min_x;
  for (std::size_t i = 0; i < unit_poly.size(); ++i) {
    s[i] = xf.to_screen(unit_poly[i]);
    min_x = std::min(min_x, s[i].x); min_y = std::min(min_y, s[i].y);
    max_x = std::max(max_x, s[i].x); max_y = std::max(max_y, s[i].y);
  }
  const std::int32_t x0 = std::max(0, static_cast<std::int32_t>(std::floor(min_x)) - 1);
  const std::int32_t y0 = std::max(0, static_cast<std::int32_t>(std::floor(min_y)) - 1);
  const std::int32_t x1 = std::min(static_cast<std::int32_t>(ctx.w) - 1, static_cast<std::int32_t>(std::ceil(max_x)) + 1);
  const std::int32_t y1 = std::min(static_cast<std::int32_t>(ctx.h) - 1, static_cast<std::int32_t>(std::ceil(max_y)) + 1);
  if (x0 > x1 || y0 > y1) return;
  const std::uint64_t area = static_cast<std::uint64_t>(x1 - x0 + 1) * static_cast<std::uint64_t>(y1 - y0 + 1);
  ctx.work += area;
  if (ctx.work > ctx.max_work) { ctx.overrun = true; return; }
  const double aa = std::max(ctx.aa_width, 0.25);
  const double glow_r = std::max(xf.rx, xf.ry);
  for (std::int32_t py = y0; py <= y1; ++py) {
    for (std::int32_t px = x0; px <= x1; ++px) {
      const Vec2 lp = xf.to_local(static_cast<double>(px) + 0.5, static_cast<double>(py) + 0.5);
      if (!point_in_polygon_even_odd(lp, unit_poly)) continue;
      const ClosestPolylineResult cr = closest_on_closed_polyline(lp, unit_poly);
      const double sdf = cr.distance;   // positive distance to boundary (inside)
      double cov = std::clamp(0.5 + sdf / aa, 0.0, 1.0);
      if (ctx.style.aa_policy == AntiAliasPolicy::none) cov = 1.0;
      cov *= opacity;
      if (cov <= 0.0) continue;
      // Light-axis gradient proxy: dot(pixel-local, light dir in part frame).
      const double gx = lp.x * xf.rx, gy = lp.y * xf.ry;
      const double grad = 0.5 + 0.5 * (gx * L.lx + gy * L.ly) / (std::max(xf.rx, xf.ry) + 1.0);
      RgbaF col = (glow > 0.0)
          ? RgbaF{base.r * 1.6, base.g * 1.6, base.b * 1.6, base.a}
          : shade_pixel(L, {}, base, mat, cov, -sdf, grad, px, py);
      if (glow <= 0.0) {
        const double hl = (ctx.style.highlight_model == HighlightModel::none) ? 0.0
            : (ctx.style.highlight_model == HighlightModel::strong) ? 0.16 : 0.06;
        if (hl > 0.0) {
          col.r = std::min(1.0, col.r + hl * grad * base.r);
          col.g = std::min(1.0, col.g + hl * grad * base.g);
          col.b = std::min(1.0, col.b + hl * grad * base.b);
        }
        // Style saturation policy (deterministic, luminance-preserving).
        if (ctx.style.saturation_behavior != 1.0) {
          const double lum = 0.299 * col.r + 0.587 * col.g + 0.114 * col.b;
          const double sat = ctx.style.saturation_behavior;
          col.r = std::clamp(lum + (col.r - lum) * sat, 0.0, 1.0);
          col.g = std::clamp(lum + (col.g - lum) * sat, 0.0, 1.0);
          col.b = std::clamp(lum + (col.b - lum) * sat, 0.0, 1.0);
        }
        // Style value grouping: posterize to N levels per channel.
        if (ctx.style.value_grouping > 0.0) {
          const double levels = static_cast<double>(1 + static_cast<int>(std::lround(ctx.style.value_grouping * 8.0)));
          const double scale = levels - 1.0;
          if (scale > 0.0) {
            col.r = std::lround(col.r * scale) / scale;
            col.g = std::lround(col.g * scale) / scale;
            col.b = std::lround(col.b * scale) / scale;
          }
        }
        // Style pixel quantization: posterize final RGB (post-shading).
        if (ctx.style.palette_policy == PalettePolicy::quantize) {
          const double keep_a = col.a;
          col = unpack(quantize_color(pack(RgbaF{col.r, col.g, col.b, 1.0}), std::max(2.0, ctx.style.pixel_quantization)));
          col.a = keep_a;
        }
      }
      col.a *= cov;
      if (glow > 0.0 && ctx.additive_layers) ctx.additive(px, py, col, 0.55 * glow);
      else ctx.source_over(px, py, col);
    }
  }
  (void)glow_r;
}

/* Rasterize a stroked polyline (tapered widths) with analytic AA. */
void raster_stroke(RasterCtx& ctx, const PartToScreen& xf, std::span<const Vec2> unit_pts,
                   std::span<const double> unit_widths, bool closed, const RgbaF& base,
                   double opacity, const MaterialSemantics& mat, const LightingModel& L,
                   double glow, double width_scale) {
  if (unit_pts.size() < 2) return;
  std::vector<Vec2> s(unit_pts.size());
  double min_x = std::numeric_limits<double>::infinity(), min_y = min_x;
  double max_x = -min_x, max_y = -min_x;
  for (std::size_t i = 0; i < unit_pts.size(); ++i) {
    s[i] = xf.to_screen(unit_pts[i]);
    min_x = std::min(min_x, s[i].x); min_y = std::min(min_y, s[i].y);
    max_x = std::max(max_x, s[i].x); max_y = std::max(max_y, s[i].y);
  }
  const double wscale = width_scale * (xf.rx + xf.ry) * 0.5;
  const std::int32_t x0 = std::max(0, static_cast<std::int32_t>(std::floor(min_x)) - 3);
  const std::int32_t y0 = std::max(0, static_cast<std::int32_t>(std::floor(min_y)) - 3);
  const std::int32_t x1 = std::min(static_cast<std::int32_t>(ctx.w) - 1, static_cast<std::int32_t>(std::ceil(max_x)) + 3);
  const std::int32_t y1 = std::min(static_cast<std::int32_t>(ctx.h) - 1, static_cast<std::int32_t>(std::ceil(max_y)) + 3);
  if (x0 > x1 || y0 > y1) return;
  const std::uint64_t area = static_cast<std::uint64_t>(x1 - x0 + 1) * static_cast<std::uint64_t>(y1 - y0 + 1);
  ctx.work += area;
  if (ctx.work > ctx.max_work) { ctx.overrun = true; return; }
  const double aa = std::max(ctx.aa_width, 0.25);
  for (std::int32_t py = y0; py <= y1; ++py) {
    for (std::int32_t px = x0; px <= x1; ++px) {
      const Vec2 p{static_cast<double>(px) + 0.5, static_cast<double>(py) + 0.5};
      double best = std::numeric_limits<double>::infinity();
      const std::size_t n = closed ? s.size() : s.size() - 1;
      for (std::size_t i = 0; i < n; ++i) {
        const Vec2 a = s[i], b = s[(i + 1) % s.size()];
        Vec2 cp; double t = 0.0;
        const double d = distance_point_segment(p, a, b, &cp, &t);
        double w = interpolate_width(unit_widths, static_cast<double>(i) + t) * wscale;
        w = std::max(w, 0.5);
        const double eff = std::abs(d) - w * 0.5;
        if (eff < best) best = eff;
      }
      const double cov = std::clamp(0.5 - best / aa, 0.0, 1.0) * opacity;
      if (cov <= 0.0) continue;
      const Vec2 lp = xf.to_local(p.x, p.y);
      const double gx = lp.x * xf.rx, gy = lp.y * xf.ry;
      const double grad = 0.5 + 0.5 * (gx * L.lx + gy * L.ly) / (std::max(xf.rx, xf.ry) + 1.0);
      RgbaF col = (glow > 0.0)
          ? RgbaF{base.r * 1.8, base.g * 1.8, base.b * 1.8, base.a}
          : shade_pixel(L, {}, base, mat, cov, -best, grad, px, py);
      col.a *= cov;
      if (glow > 0.0 && ctx.additive_layers) ctx.additive(px, py, col, 0.7 * glow);
      else ctx.source_over(px, py, col);
    }
  }
}


/* Draw a marking in its bound part's local frame, masked by part
 * coverage. All geometry comes from package/IR semantics. */
void raster_marking(RasterCtx& ctx, const Marking& m, const PartToScreen& xf,
                    const RgbaF& color, double glow) {
  const double wscale = (xf.rx + xf.ry) * 0.5;
  switch (m.kind) {
    case MarkingKind::spot: {
      // Spots: small filled ellipses at local centers.
      for (const Vec2& c : m.spots) {
        std::vector<Vec2> poly = ellipse_polyline(m.spot_radius, m.spot_radius, 16);
        for (Vec2& v : poly) v = Vec2{v.x + c.x, v.y + c.y};
        const PartToScreen sx{xf.cx, xf.cy, xf.cos_r, xf.sin_r,
                              xf.rx * m.scale, xf.ry * m.scale};
        raster_fill(ctx, sx, poly, color, m.opacity, make_material("energy"), ctx.light, glow);
      }
      break;
    }
    case MarkingKind::gradient_region: {
      if (m.path.size() >= 3) {
        raster_fill(ctx, xf, m.path, color, m.opacity, make_material("energy"), ctx.light, glow);
      }
      break;
    }
    default: {
      // Polyline markings: stripe/band/vein/circuit/electrical/scar/tattoo/symbol/procedural.
      if (m.path.size() >= 2) {
        std::vector<double> widths = (m.widths.size() == m.path.size())
            ? m.widths : std::vector<double>(m.path.size(), m.band_width);
        raster_stroke(ctx, xf, m.path, widths, false, color, m.opacity,
                      make_material("energy"), ctx.light, glow, wscale);
      }
      break;
    }
  }
}

/* Drop shadow: soft dark ellipse offset below the entity bbox. */
void raster_drop_shadow(RasterCtx& ctx, const PartToScreen& xf, double rx, double ry,
                        const LightingSpec& light) {
  if (!light.drop_shadow) return;
  std::vector<Vec2> poly = ellipse_polyline(1.0, 1.0, 48);
  const PartToScreen sx{xf.cx + light.shadow_offset_x, xf.cy + light.shadow_offset_y,
                        1.0, 0.0, rx, ry};
  raster_fill(ctx, sx, poly, RgbaF{0, 0, 0, light.shadow_alpha}, 1.0,
              make_material("skin"), ctx.light, 0.0);
}

/* Outline pass: dark stroke along the accumulated silhouette. */
void raster_outline(RasterCtx& ctx, const std::vector<WorldPart>& parts,
                    const std::vector<PartToScreen>& xfs, const std::vector<ShapeGeom>& geoms,
                    const std::vector<std::uint32_t>& colors, double weight_px) {
  const RgbaF ink{0.06, 0.05, 0.12, 1.0};
  std::vector<double> unit_widths;  // reused scratch: capacity is preserved across parts
  for (std::size_t i = 0; i < parts.size(); ++i) {
    const VisualPart& p = *parts[i].part;
    if (!p.silhouette_contribution) continue;
    if (p.shape == VisualShapeKind::open_path) continue;
    if (geoms[i].fill.size() < 3) continue;
    const RgbaF c = unpack(colors[i]);
    // Stroke the boundary at ink color with the part's base alpha.
    unit_widths.assign(geoms[i].fill.size(), 1.0);
    raster_stroke(ctx, xfs[i], geoms[i].fill, unit_widths,
                  true, ink, std::clamp(c.a, 0.0, 1.0), make_material("skin"),
                  LightingModel{}, 0.0, weight_px);
  }
  (void)parts;
}


/* Build world transforms through the part hierarchy, applying
 * performance-state motion offsets. Deterministic and cycle-free
 * (validation rejects cycles earlier). */
/* Returns false if any part could not be resolved (cycle or missing parent
 * that slipped past validation). Callers must fail closed; unresolved parts
 * must never silently render at the origin. */
bool resolve_world_parts(const VisualIr& ir, std::vector<WorldPart>& out) {
  const auto& parts = ir.morphology.parts;
  struct Node { const VisualPart* part; std::string parent; };
  std::map<std::string, Node, std::less<>> nodes;
  for (const auto& [id, p] : parts) nodes.emplace(id, Node{&p, p.parent});

  // Forward kinematics: child offsets live in the parent frame.
  // world_pos = parent_pos + R(parent_rot) * offset
  // world_rot = parent_rot + own_rot
  auto motion_for = [&](const std::string& id) -> const PartMotion* {
    for (const auto& m : ir.performance.motions)
      if (m.part_id == id) return &m;
    return nullptr;
  };

  std::map<std::string, double, std::less<>> wpos_x, wpos_y, wrot;
  std::vector<std::pair<std::string, const VisualPart*>> order;
  for (const auto& [id, n] : nodes) order.emplace_back(id, n.part);
  // Deterministic topological order: process parents before children.
  bool changed = true;
  std::size_t guard = 0;
  while (changed && guard++ < order.size() + 1) {
    changed = false;
    for (auto& [id, p] : order) {
      if (wpos_x.count(id)) continue;
      const std::string& par = p->parent;
      if (!par.empty()) {
        if (!wpos_x.count(par)) continue;  // parent not ready
        const double pr = wrot[par] * kPi / 180.0;
        wpos_x[id] = wpos_x[par] + (p->x * std::cos(pr) - p->y * std::sin(pr));
        wpos_y[id] = wpos_y[par] + (p->x * std::sin(pr) + p->y * std::cos(pr));
        wrot[id] = wrot[par] + p->rotation_degrees;
      } else {
        wpos_x[id] = p->x; wpos_y[id] = p->y;
        wrot[id] = p->rotation_degrees;
      }
      changed = true;
    }
  }

  // Fail closed: any part still missing from the resolved tables could not be
  // placed (cycle/missing parent). Do not fall back to local coordinates.
  for (const auto& [id, n] : nodes) {
    if (!wpos_x.count(id) || !wpos_y.count(id) || !wrot.count(id)) return false;
  }

  for (const auto& [id, n] : nodes) {
    const VisualPart& p = *n.part;
    WorldPart wp;
    wp.part = n.part;
    double mx = wpos_x[id];
    double my = wpos_y[id];
    double mrot = wrot[id];
    double msx = 1.0, msy = 1.0;
    if (const PartMotion* m = motion_for(id)) {
      mx += m->dx; my += m->dy; mrot += m->rotation_degrees;
      msx = m->scale_x; msy = m->scale_y;
    }
    if (ir.projection.mirror_x) mx = -mx;
    wp.cx = static_cast<double>(ir.projection.width) * 0.5 + mx * kPosScale;
    wp.cy = static_cast<double>(ir.projection.height) * 0.5 - my * kPosScale;
    wp.rot_rad = mrot * kPi / 180.0;
    wp.rx = p.size_x * kSizeScale * msx;
    wp.ry = p.size_y * kSizeScale * msy;
    if (wp.rx <= 0.0) wp.rx = 0.5;
    if (wp.ry <= 0.0) wp.ry = 0.5;
    wp.z_order = p.z_order;
    out.push_back(wp);
  }
  return true;
}


} // anonymous namespace (helpers)
RasterResult render_visual_ir(const VisualIr& ir, const RasterLimits& limits) {
  RasterResult result;
  ValidationResult& diag = result.diagnostics;
  auto add = [&](std::string code, const std::string& msg) {
    diag.diagnostics.push_back({std::move(code), msg});
  };

  // Fail closed: validate IR against combined limits. The channel budget is
  // bound to the raster marking budget so validation and the render loop can
  // never disagree (a request count between the two budgets would otherwise be
  // silently truncated below).
  VisualLimits vl;
  vl.max_canvas_width = limits.max_width;
  vl.max_canvas_height = limits.max_height;
  vl.max_channel_requests = limits.max_markings;
  const ValidationResult vcheck = validate_visual_ir(ir, vl);
  for (const auto& d : vcheck.diagnostics) diag.diagnostics.push_back(d);
  if (!vcheck.ok() || ir.projection.width == 0 || ir.projection.height == 0) return result;
  if (ir.projection.width > limits.max_width || ir.projection.height > limits.max_height) {
    add("RASTER_CANVAS_OVER_LIMIT", "canvas exceeds raster limits");
    return result;
  }

  const std::uint32_t w = ir.projection.width, h = ir.projection.height;
  RasterOutput out;
  out.image.width = w; out.image.height = h;
  out.image.color_space = ColorSpace::srgb;
  out.image.alpha_mode = AlphaMode::straight;
  out.image.pixels.assign(static_cast<std::size_t>(w) * h * 4, 0);

  RasterCtx ctx{w, h, out.image.pixels.data(), std::vector<double>(static_cast<std::size_t>(w) * h, 0.0),
                0, limits.max_raster_work, false, {}, {}, {}, {}, 1.0, false, false};
  ctx.style = ir.style;
  ctx.palette = ir.palette;
  // Layer index from IR layer_order (authoritative compositing order).
  for (std::size_t i = 0; i < ir.layer_order.size(); ++i)
    ctx.layer_index.push_back(static_cast<std::int32_t>(i));
  auto layer_idx_of = [&](std::string_view name) -> std::int32_t {
    for (std::size_t i = 0; i < ir.layer_order.size(); ++i)
      if (ir.layer_order[i] == name) return static_cast<std::int32_t>(i);
    return static_cast<std::int32_t>(ir.layer_order.size());
  };
  // Lighting from IR.
  const double lrad = ir.lighting.light_degrees * kPi / 180.0;
  ctx.light = LightingModel{std::cos(lrad), std::sin(lrad), ir.lighting.ambient,
                            ir.lighting.key, ir.lighting.fill, ir.lighting.rim,
                            ir.style.shadow_model == ShadowModel::cell ? ir.style.band_count : 1u};
  ctx.aa_width = (ir.style.aa_policy == AntiAliasPolicy::none) ? 0.0
      : 0.75 + ir.style.edge_softness * 1.25;
  ctx.additive_layers = (ir.style.layer_compositing == LayerCompositing::additive);

  // Resolve parts: world transforms + geometry + colors + materials.
  std::vector<WorldPart> world;
  if (!resolve_world_parts(ir, world)) {
    add("RASTER_UNRESOLVED_PARTS", "part hierarchy could not be fully resolved");
    return result;
  }
  const std::size_t np = world.size();
  std::vector<PartToScreen> xfs(np);
  std::vector<ShapeGeom> geoms(np);
  std::vector<std::uint32_t> colors(np, 0xFFFFFFFF);
  std::vector<MaterialSemantics> mats(np);
  std::vector<double> glows(np, 0.0);
  std::vector<std::int32_t> layers(np, 0);
  const FlattenOptions flat{0.25, 4096, 12};
  for (std::size_t i = 0; i < np; ++i) {
    const VisualPart& p = *world[i].part;
    xfs[i] = PartToScreen{world[i].cx, world[i].cy, std::cos(world[i].rot_rad),
                          std::sin(world[i].rot_rad), world[i].rx, world[i].ry};
    geoms[i] = build_shape(p, flat);
    if (geoms[i].stroke.size() > limits.max_flattened_vertices) {
      add("RASTER_VERTICES_OVER_LIMIT", "flattened path exceeds vertex budget");
      return result;
    }
    colors[i] = resolve_part_color(p, ir.palette);
    if (ir.style.palette_policy == PalettePolicy::quantize) {
      colors[i] = quantize_color(colors[i], std::max(2.0, ir.style.pixel_quantization));
    }
    mats[i] = resolve_material(make_material(p.material_class, p.material_id), ir.style);
    glows[i] = resolve_emission(p, mats[i]);
    layers[i] = layer_idx_of(layer_name(p.layer));
  }

  // StyleProgram max_colors: when non-zero, clamp the palette to the
  // requested maximum by collapsing the least-frequent colors to their
  // nearest neighbor (deterministic Euclidean distance in RGB).
  if (ir.style.max_colors > 0 && np > ir.style.max_colors) {
    std::map<std::uint32_t, std::size_t, std::less<>> freq;
    for (std::size_t i = 0; i < np; ++i) ++freq[colors[i]];
    const std::uint32_t limit = ir.style.max_colors;
    while (freq.size() > limit) {
      // Find rarest color and nearest neighbor to merge into.
      std::uint32_t rarest = 0, best_target = 0;
      std::size_t rarest_cnt = std::numeric_limits<std::size_t>::max();
      double best_dist = std::numeric_limits<double>::max();
      for (const auto& [c, n] : freq) {
        if (n < rarest_cnt) { rarest = c; rarest_cnt = n; }
      }
      // Find the nearest color to merge into (exclude self).
      const double rr = static_cast<double>((rarest >> 16) & 0xFF) / 255.0;
      const double rg = static_cast<double>((rarest >> 8) & 0xFF) / 255.0;
      const double rb = static_cast<double>(rarest & 0xFF) / 255.0;
      for (const auto& [c, n] : freq) {
        if (c == rarest) continue;
        const double dr = rr - static_cast<double>((c >> 16) & 0xFF) / 255.0;
        const double dg = rg - static_cast<double>((c >> 8) & 0xFF) / 255.0;
        const double db = rb - static_cast<double>(c & 0xFF) / 255.0;
        const double d = dr * dr + dg * dg + db * db;
        if (d < best_dist) { best_dist = d; best_target = c; }
      }
      freq[best_target] += rarest_cnt;
      freq.erase(rarest);
    }
    // Remap: each color maps to its nearest palette color.
    for (std::size_t i = 0; i < np; ++i) {
      std::uint32_t best = 0;
      double best_dist = std::numeric_limits<double>::max();
      const double cr = static_cast<double>((colors[i] >> 16) & 0xFF) / 255.0;
      const double cg = static_cast<double>((colors[i] >> 8) & 0xFF) / 255.0;
      const double cb = static_cast<double>(colors[i] & 0xFF) / 255.0;
      for (const auto& [c, n] : freq) {
        const double dr = cr - static_cast<double>((c >> 16) & 0xFF) / 255.0;
        const double dg = cg - static_cast<double>((c >> 8) & 0xFF) / 255.0;
        const double db = cb - static_cast<double>(c & 0xFF) / 255.0;
        const double d = dr * dr + dg * dg + db * db;
        if (d < best_dist) { best_dist = d; best = c; }
      }
      colors[i] = best;
    }
  }

  // Deterministic draw order: layer, then z_order, then part id (stable).
  std::vector<std::size_t> order(np);
  for (std::size_t i = 0; i < np; ++i) order[i] = i;
  std::vector<std::string> ids(np);
  for (std::size_t i = 0; i < np; ++i) ids[i] = world[i].part->id;
  std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
    if (layers[a] != layers[b]) return layers[a] < layers[b];
    if (world[a].z_order != world[b].z_order) return world[a].z_order < world[b].z_order;
    return ids[a] < ids[b];
  });


  // Drop shadow first (behind everything).
  {
    double min_rx = 1.0, min_ry = 1.0;
    std::size_t n_shadow = 0;
    for (std::size_t i = 0; i < np; ++i) {
      const VisualPart& p = *world[i].part;
      if (!p.silhouette_contribution) continue;
      min_rx = std::max(min_rx, world[i].rx);
      min_ry = std::max(min_ry, world[i].ry);
      ++n_shadow;
    }
    if (n_shadow > 0 && world.size() > 0) {
      LightingSpec lm = ir.lighting;
      if (ir.style.shadow_model == ShadowModel::flat) lm.shadow_alpha = 0.55;
      else if (ir.style.shadow_model == ShadowModel::gradient) lm.shadow_alpha = 0.32;
      else if (ir.style.shadow_model == ShadowModel::cell) lm.shadow_alpha = 0.45;
      raster_drop_shadow(ctx, xfs[0], min_rx, min_ry, lm);
    }
  }

  // Render each part in deterministic order (fill, stroke, markings).
  for (const std::size_t idx : order) {
    const VisualPart& p = *world[idx].part;
    if (!p.visible) continue;
    const RgbaF base = unpack(colors[idx]);
    const double opacity = std::clamp(p.opacity, 0.0, 1.0);
    if (geoms[idx].has_fill) {
      raster_fill(ctx, xfs[idx], geoms[idx].fill, base, opacity, mats[idx], ctx.light,
                  glows[idx]);
    }
    if (geoms[idx].has_stroke) {
      raster_stroke(ctx, xfs[idx], geoms[idx].stroke, geoms[idx].widths, geoms[idx].closed,
                    base, opacity, mats[idx], ctx.light, glows[idx], 1.0);
    }
    // Markings bound to this semantic surface (masked by part geometry).
    for (const Marking& m : ir.morphology.markings) {
      if (m.part_id != p.id) continue;
      const bool bound = std::find(p.marking_ids.begin(), p.marking_ids.end(), m.id) !=
                         p.marking_ids.end();
      if (!bound) continue;
      std::uint32_t mcol = (m.color_role != ColorRole::custom)
          ? palette_color(ir.palette, m.color_role, m.color) : m.color;
      ctx.flat_color = (m.color_role == ColorRole::emission);
      raster_marking(ctx, m, xfs[idx], unpack(mcol),
                     (m.color_role == ColorRole::emission) ? 1.0 : 0.0);
      ctx.flat_color = false;
    }
  }

  // Outline pass (deterministic, after compositing).
  if (ir.style.line_weight > 0.0 && ir.style.outline_selection != OutlineSelection::none) {
    const double wpx = ir.style.line_weight * (w + h) / 256.0;
    raster_outline(ctx, world, xfs, geoms, colors, wpx);
  }

  if (ctx.overrun) {
    add("RASTER_WORK_OVER_LIMIT", "raster work budget exceeded");
    return result;
  }
  out.raster_work = ctx.work;


  // ── Semantic channels derived from the Visual IR (not from RGBA). ──
  auto make_channel = [&](std::string id, ChannelMapKind kind) -> ImageRgba8 {
    (void)id; (void)kind;
    ImageRgba8 img;
    img.width = w; img.height = h;
    img.color_space = ColorSpace::data;
    img.alpha_mode = AlphaMode::straight;
    img.pixels.assign(static_cast<std::size_t>(w) * h * 4, 0);
    return img;
  };
  struct ChanBuf { ChannelRaster cr; RasterCtx c; };
  // NOTE: under MSVC Debug STL, vector/string/map moves are not noexcept, so a
  // vector<ChanBuf> reallocation would COPY ChanBuf and the copied RasterCtx.fb
  // (raw pointer into cr.image.pixels) would dangle. Reserve capacity up front so
  // no reallocation can ever happen; fb is rebound from the live image below.
  // Fail closed: never silently truncate channel requests. Validation above
  // already bound max_channel_requests to this budget, so this check is a
  // defensive double guard; reserving the exact request count makes vector
  // reallocation (and the MSVC-Debug copy hazard this buffer used to have)
  // structurally impossible.
  if (ir.channel_requests.size() > limits.max_markings) {
    add("RASTER_CHANNEL_OVER_LIMIT", "channel requests exceed raster budget");
    return result;
  }
  std::vector<ChanBuf> chan;
  chan.reserve(ir.channel_requests.size());
  for (const ChannelRequest& req : ir.channel_requests) {
    chan.emplace_back();
    ChannelRaster& cr = chan.back().cr;
    cr.id = req.id;
    cr.kind = req.kind;
    cr.image = make_channel(req.id, req.kind);
    RasterCtx& c = chan.back().c;
    c = RasterCtx{w, h, cr.image.pixels.data(),
                  std::vector<double>(static_cast<std::size_t>(w) * h, 0.0),
                  0, limits.max_raster_work, false, {}, {}, {}, {}, 1.0, true, true};
    c.flat_color = true;
  }

  for (std::size_t i = 0; i < np; ++i) {
    const VisualPart& p = *world[i].part;
    const std::uint32_t cidx = static_cast<std::uint32_t>(i + 1);
    for (ChanBuf& cb : chan) {
      cb.c.fb = cb.cr.image.pixels.data();  // stable even if ChanBuf was copied
      switch (cb.cr.kind) {
        case ChannelMapKind::emissive: {
          if (glows[i] > 0.0) {
            const RgbaF base = unpack(colors[i]);
            RgbaF e{base.r, base.g, base.b, glows[i]};
            raster_fill(cb.c, xfs[i], geoms[i].fill, e, 1.0, mats[i], cb.c.light,
                        0.0);
          }
          break;
        }
        case ChannelMapKind::depth: {
          const double z01 = std::clamp(0.5 + p.z / 64.0, 0.0, 1.0);
          const std::uint8_t gz = static_cast<std::uint8_t>(z01 * 255.0);
          const std::uint32_t zc = (static_cast<std::uint32_t>(gz) << 24) |
              (static_cast<std::uint32_t>(gz) << 16) | (static_cast<std::uint32_t>(gz) << 8) | 0xFFu;
          raster_fill(cb.c, xfs[i], geoms[i].fill, unpack(zc), 1.0, mats[i],
                      cb.c.light, 0.0);
          break;
        }
        case ChannelMapKind::material_id: {
          const std::uint8_t mi = static_cast<std::uint8_t>((cidx * 40) & 0xFF);
          const std::uint32_t mc = (static_cast<std::uint32_t>(mi) << 24) |
              (static_cast<std::uint32_t>(mi) << 16) | (static_cast<std::uint32_t>(mi) << 8) | 0xFFu;
          raster_fill(cb.c, xfs[i], geoms[i].fill, unpack(mc), 1.0, mats[i],
                      cb.c.light, 0.0);
          break;
        }
        default: {  // effects: coverage mask (white)
          raster_fill(cb.c, xfs[i], geoms[i].fill, RgbaF{1, 1, 1, 1}, 1.0, mats[i],
                      cb.c.light, 0.0);
          break;
        }
      }
    }
  }

  for (ChanBuf& cb : chan) {
    if (cb.c.overrun) {
      add("RASTER_CHANNEL_OVER_LIMIT", "channel raster work exceeded");
      return result;
    }
    out.channels.push_back(std::move(cb.cr));
  }

  result.value = std::move(out);
  return result;
}

} // namespace gspl::sprites::visual
