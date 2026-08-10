#include "gspl_sprites/palette.hpp"
#include "gspl_sprites/visual_metrics.hpp"

#include <algorithm>
#include <cmath>
#include <queue>

namespace gspl::sprites::visual {
namespace {

[[nodiscard]] bool covered(const ImageRgba8& img, std::uint32_t x, std::uint32_t y) {
  const std::size_t i = (static_cast<std::size_t>(y) * img.width + x) * 4;
  return img.pixels[i + 3] > 127;
}

[[nodiscard]] bool partial(const ImageRgba8& img, std::uint32_t x, std::uint32_t y) {
  const std::size_t i = (static_cast<std::size_t>(y) * img.width + x) * 4;
  const std::uint8_t a = img.pixels[i + 3];
  return a > 0 && a < 255;
}

} // namespace

SilhouetteStats analyze_silhouette(const ImageRgba8& image) {
  SilhouetteStats st;
  if (image.width == 0 || image.height == 0 || image.pixels.size() <
      static_cast<std::size_t>(image.width) * image.height * 4) {
    st.bounds = Bounds::empty();
    return st;
  }
  const std::uint32_t w = image.width, h = image.height;
  std::vector<std::uint8_t> mask(static_cast<std::size_t>(w) * h, 0);
  double sum_x = 0.0, sum_y = 0.0;
  std::uint32_t count = 0;
  for (std::uint32_t y = 0; y < h; ++y) {
    for (std::uint32_t x = 0; x < w; ++x) {
      if (!covered(image, x, y)) continue;
      mask[static_cast<std::size_t>(y) * w + x] = 1;
      ++count;
      sum_x += static_cast<double>(x);
      sum_y += static_cast<double>(y);
      if (st.bounds.is_empty()) {
        st.bounds = Bounds{static_cast<double>(x), static_cast<double>(y),
                           static_cast<double>(x), static_cast<double>(y)};
      } else {
        st.bounds.min_x = std::min(st.bounds.min_x, static_cast<double>(x));
        st.bounds.min_y = std::min(st.bounds.min_y, static_cast<double>(y));
        st.bounds.max_x = std::max(st.bounds.max_x, static_cast<double>(x));
        st.bounds.max_y = std::max(st.bounds.max_y, static_cast<double>(y));
      }
    }
  }
  st.covered_pixels = count;
  st.area_ratio = static_cast<double>(count) / (static_cast<double>(w) * h);
  if (count > 0) {
    st.centroid_dx = (sum_x / count - (w - 1) * 0.5) / w;
    st.centroid_dy = (sum_y / count - (h - 1) * 0.5) / h;
    const double bw = st.bounds.width(), bh = st.bounds.height();
    st.aspect_ratio = (bh > 0.0) ? bw / bh : 0.0;
  }
  // Perimeter + AA edge quality.
  for (std::uint32_t y = 0; y < h; ++y) {
    for (std::uint32_t x = 0; x < w; ++x) {
      if (!covered(image, x, y)) continue;
      const bool edge = (x == 0 || y == 0 || x + 1 == w || y + 1 == h ||
                         !covered(image, x - 1, y) || !covered(image, x + 1, y) ||
                         !covered(image, x, y - 1) || !covered(image, x, y + 1));
      if (edge) ++st.perimeter_pixels;
    }
  }
  // Connected components (4-neighborhood flood fill, bounded).
  std::vector<std::uint8_t> seen(static_cast<std::size_t>(w) * h, 0);
  for (std::uint32_t y = 0; y < h; ++y) {
    for (std::uint32_t x = 0; x < w; ++x) {
      const std::size_t m = static_cast<std::size_t>(y) * w + x;
      if (!mask[m] || seen[m]) continue;
      ++st.connected_components;
      std::queue<std::size_t> q;
      q.push(m);
      seen[m] = 1;
      std::uint32_t guard = 0;
      while (!q.empty() && guard++ < count + 1) {
        const std::size_t cur = q.front();
        q.pop();
        const std::uint32_t cx = static_cast<std::uint32_t>(cur % w);
        const std::uint32_t cy = static_cast<std::uint32_t>(cur / w);
        if (cx + 1 < w) { const std::size_t n = cur + 1; if (mask[n] && !seen[n]) { seen[n] = 1; q.push(n); } }
        if (cx > 0) { const std::size_t n = cur - 1; if (mask[n] && !seen[n]) { seen[n] = 1; q.push(n); } }
        if (cy + 1 < h) { const std::size_t n = cur + w; if (mask[n] && !seen[n]) { seen[n] = 1; q.push(n); } }
        if (cy > 0) { const std::size_t n = cur - w; if (mask[n] && !seen[n]) { seen[n] = 1; q.push(n); } }
      }
    }
  }
  return st;
}


double frame_similarity(const ImageRgba8& a, const ImageRgba8& b) {
  if (a.width != b.width || a.height != b.height || a.pixels.size() != b.pixels.size()) return 0.0;
  const std::size_t npx = a.pixels.size() / 4;
  if (npx == 0) return 1.0;
  // Background-invariant: only pixels where either frame has content count.
  // Style/rendering differences on the entity must not be diluted by the
  // transparent canvas around it.
  double acc = 0.0;
  std::size_t relevant = 0;
  for (std::size_t i = 0; i < a.pixels.size(); i += 4) {
    if (a.pixels[i + 3] == 0 && b.pixels[i + 3] == 0) continue;
    ++relevant;
    const double da = static_cast<double>(std::abs(static_cast<int>(a.pixels[i]) - static_cast<int>(b.pixels[i])));
    const double dg = static_cast<double>(std::abs(static_cast<int>(a.pixels[i + 1]) - static_cast<int>(b.pixels[i + 1])));
    const double db = static_cast<double>(std::abs(static_cast<int>(a.pixels[i + 2]) - static_cast<int>(b.pixels[i + 2])));
    const double dd = static_cast<double>(std::abs(static_cast<int>(a.pixels[i + 3]) - static_cast<int>(b.pixels[i + 3])));
    acc += (da + dg + db + dd) / (4.0 * 255.0);
  }
  if (relevant == 0) return 1.0;
  return 1.0 - acc / static_cast<double>(relevant);
}

double frame_distinction(const ImageRgba8& a, const ImageRgba8& b) {
  return 1.0 - frame_similarity(a, b);
}

FidelityReport measure_frame_fidelity(const VisualIr& ir, const RasterOutput& out,
                                      const ImageRgba8* reference) {
  FidelityReport rep;
  const ImageRgba8& img = out.image;
  rep.silhouette = analyze_silhouette(img);

  // Alpha-edge quality: among boundary pixels (covered or adjacent to
  // covered), the fraction that carry partial alpha (analytic AA).
  if (img.width > 0 && img.height > 0) {
    std::uint32_t edge_total = 0, edge_partial = 0;
    for (std::uint32_t y = 0; y < img.height; ++y) {
      for (std::uint32_t x = 0; x < img.width; ++x) {
        const bool c = covered(img, x, y);
        const bool border = (x == 0 || y == 0 || x + 1 == img.width || y + 1 == img.height);
        const bool adjacent = border || !covered(img, x - 1, y) || !covered(img, x + 1, y) ||
                              !covered(img, x, y - 1) || !covered(img, x, y + 1);
        if (c != adjacent || (c && !adjacent)) continue;
        if (adjacent) {
          ++edge_total;
          if (partial(img, x, y) || (c && border)) ++edge_partial;
        }
      }
    }
    if (edge_total > 0) rep.alpha_edge_quality =
        static_cast<double>(edge_partial) / static_cast<double>(edge_total);
  }

  // Palette adherence: covered pixels near the resolved palette colors
  // (shaded variants allowed within luminance distance 0.28).
  std::vector<std::uint32_t> palette_rgba;
  for (const auto& [role, color] : ir.palette.colors) {
    (void)role;
    palette_rgba.push_back(color);
  }
  if (!palette_rgba.empty()) {
    std::uint32_t hit = 0, covered_n = 0;
    for (std::uint32_t y = 0; y < img.height; ++y) {
      for (std::uint32_t x = 0; x < img.width; ++x) {
        const std::size_t i = (static_cast<std::size_t>(y) * img.width + x) * 4;
        if (img.pixels[i + 3] <= 127) continue;
        ++covered_n;
        const double lum = color_luminance((static_cast<std::uint32_t>(img.pixels[i]) << 24) |
            (static_cast<std::uint32_t>(img.pixels[i + 1]) << 16) |
            (static_cast<std::uint32_t>(img.pixels[i + 2]) << 8) | 0xFFu);
        bool near = false;
        for (const std::uint32_t pc : palette_rgba) {
          const double pl = color_luminance(pc);
          if (std::abs(lum - pl) < 0.28) { near = true; break; }
        }
        if (near) ++hit;
      }
    }
    if (covered_n > 0) rep.palette_adherence = static_cast<double>(hit) / covered_n;
    rep.palette_valid = true;
  }

  // Shape overlap: fraction of covered area also covered by >1 part,
  // measured from the material_id channel when present.
  for (const ChannelRaster& ch : out.channels) {
    if (ch.kind != ChannelMapKind::material_id) continue;
    const ImageRgba8& mc = ch.image;
    std::uint32_t multi = 0, cov2 = 0;
    for (std::uint32_t y = 0; y < mc.height; ++y) {
      for (std::uint32_t x = 0; x < mc.width; ++x) {
        const std::size_t i = (static_cast<std::size_t>(y) * mc.width + x) * 4;
        if (mc.pixels[i + 3] <= 127) continue;
        ++cov2;
        // Distinct material stamps accumulate via source-over; a pixel
        // covered by 2+ parts shows a blended value different from the
        // last stamp alone - approximate overlap via alpha < 255 depth.
        if (mc.pixels[i + 3] < 255) ++multi;
      }
    }
    if (cov2 > 0) rep.shape_overlap_ratio = static_cast<double>(multi) / cov2;
    break;
  }

  if (reference) {
    rep.frame_distinction = frame_distinction(img, *reference);
    rep.loop_closure = frame_similarity(img, *reference);
  }

  rep.notes.push_back("silhouette components=" + std::to_string(rep.silhouette.connected_components));
  rep.notes.push_back("silhouette area=" + std::to_string(rep.silhouette.area_ratio));
  rep.notes.push_back("alpha edge quality=" + std::to_string(rep.alpha_edge_quality));
  rep.notes.push_back("palette adherence=" + std::to_string(rep.palette_adherence));
  return rep;
}

} // namespace gspl::sprites::visual
