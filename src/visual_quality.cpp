#include "gspl_sprites/visual_quality.hpp"

#include <algorithm>
#include <cmath>
#include <queue>
#include <sstream>

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

/* Outline pixel: covered with at least one uncovered 4-neighbor (or on
 * the canvas border). Returns false for interior/empty pixels. */
[[nodiscard]] bool is_outline(const ImageRgba8& img, std::uint32_t x, std::uint32_t y) {
  if (!covered(img, x, y)) return false;
  if (x == 0 || y == 0 || x + 1 == img.width || y + 1 == img.height) return true;
  return !covered(img, x - 1, y) || !covered(img, x + 1, y) ||
         !covered(img, x, y - 1) || !covered(img, x, y + 1);
}

/* Weight class 0..2 derived from 8-neighbor coverage (thin/normal/thick). */
[[nodiscard]] int weight_class(const ImageRgba8& img, std::uint32_t x, std::uint32_t y) {
  int n = 0;
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      if (dx == 0 && dy == 0) continue;
      const std::int64_t nx = static_cast<std::int64_t>(x) + dx;
      const std::int64_t ny = static_cast<std::int64_t>(y) + dy;
      if (nx < 0 || ny < 0 || nx >= img.width || ny >= img.height) continue;
      if (covered(img, static_cast<std::uint32_t>(nx), static_cast<std::uint32_t>(ny))) ++n;
    }
  }
  if (n < 3) return 0;
  if (n < 6) return 1;
  return 2;
}

/* Deterministic luminance-class quantization for pixel-cluster analysis. */
[[nodiscard]] std::uint8_t pixel_class(const ImageRgba8& img, std::uint32_t x, std::uint32_t y) {
  const std::size_t i = (static_cast<std::size_t>(y) * img.width + x) * 4;
  if (img.pixels[i + 3] <= 127) return 0xFF;  // background sentinel
  const std::uint32_t r = img.pixels[i], g = img.pixels[i + 1], b = img.pixels[i + 2];
  const std::uint32_t lum = (r * 299u + g * 587u + b * 114u) / 1000u;
  // 8 luminance buckets + 4 chroma-dominant buckets -> 32 classes.
  const std::uint32_t bucket = std::min(lum / 32u, 7u);
  const std::uint32_t chroma = (r > g && r > b) ? 0u : ((g > b) ? 1u : 2u);
  return static_cast<std::uint8_t>(bucket * 4u + chroma);
}

} // namespace

std::uint32_t silhouette_disconnected_regions(const ImageRgba8& image, double tolerance_ratio) {
  if (image.width == 0 || image.height == 0) return 0;
  const std::uint32_t w = image.width, h = image.height;
  std::vector<std::uint8_t> mask(static_cast<std::size_t>(w) * h, 0);
  std::uint32_t total = 0;
  for (std::uint32_t y = 0; y < h; ++y) {
    for (std::uint32_t x = 0; x < w; ++x) {
      if (!covered(image, x, y)) continue;
      mask[static_cast<std::size_t>(y) * w + x] = 1;
      ++total;
    }
  }
  if (total == 0) return 0;
  const std::uint32_t min_area = static_cast<std::uint32_t>(
      std::max(1.0, static_cast<double>(total) * std::clamp(tolerance_ratio, 0.0, 1.0)));
  std::vector<std::uint8_t> seen(static_cast<std::size_t>(w) * h, 0);
  std::uint32_t accidental = 0;
  for (std::uint32_t y = 0; y < h; ++y) {
    for (std::uint32_t x = 0; x < w; ++x) {
      const std::size_t m = static_cast<std::size_t>(y) * w + x;
      if (!mask[m] || seen[m]) continue;
      std::queue<std::size_t> q;
      q.push(m);
      seen[m] = 1;
      std::uint32_t area = 0;
      while (!q.empty()) {
        const std::size_t cur = q.front();
        q.pop();
        ++area;
        const std::uint32_t cx = static_cast<std::uint32_t>(cur % w);
        const std::uint32_t cy = static_cast<std::uint32_t>(cur / w);
        if (cx + 1 < w) { const std::size_t n = cur + 1; if (mask[n] && !seen[n]) { seen[n] = 1; q.push(n); } }
        if (cx > 0) { const std::size_t n = cur - 1; if (mask[n] && !seen[n]) { seen[n] = 1; q.push(n); } }
        if (cy + 1 < h) { const std::size_t n = cur + w; if (mask[n] && !seen[n]) { seen[n] = 1; q.push(n); } }
        if (cy > 0) { const std::size_t n = cur - w; if (mask[n] && !seen[n]) { seen[n] = 1; q.push(n); } }
      }
      if (area < min_area) ++accidental;
    }
  }
  return accidental;
}

double landmark_jitter_between(const VisualCanon& canon,
                               const ImageRgba8& a, const ImageRgba8& b,
                               std::string_view landmark_id) {
  const auto lit = canon.landmarks.find(std::string(landmark_id));
  if (lit == canon.landmarks.end()) return 1.0;
  const VisualLandmark& lm = lit->second;
  const auto sit = canon.structures.find(lm.owner);
  if (sit == canon.structures.end()) return 1.0;
  const CanonStructure& s = sit->second;

  // Canon-space bounds over all structures (deterministic normalization).
  Bounds bounds = Bounds::empty();
  for (const auto& [id, st] : canon.structures) {
    (void)id;
    bounds.include(st.x - st.size_x * 0.5, st.y - st.size_y * 0.5);
    bounds.include(st.x + st.size_x * 0.5, st.y + st.size_y * 0.5);
  }
  if (bounds.is_empty() || bounds.width() <= 0.0 || bounds.height() <= 0.0) return 1.0;

  const double px = s.x + lm.ox * s.size_x * 0.5;
  const double py = s.y + lm.oy * s.size_y * 0.5;
  const double nx = (px - bounds.min_x) / bounds.width();
  const double ny = (py - bounds.min_y) / bounds.height();
  if (a.width != b.width || a.height != b.height || a.width == 0 || a.height == 0) return 1.0;

  const std::uint32_t w = a.width, h = a.height;
  const std::uint32_t half = std::max<std::uint32_t>(2, std::min(w, h) / 16);
  const std::int64_t cx = static_cast<std::int64_t>(nx * (w - 1));
  const std::int64_t cy = static_cast<std::int64_t>(ny * (h - 1));

  // Windowed content similarity around the landmark (background-invariant).
  double acc = 0.0;
  std::size_t relevant = 0;
  for (std::int64_t dy = -static_cast<std::int64_t>(half); dy <= static_cast<std::int64_t>(half); ++dy) {
    for (std::int64_t dx = -static_cast<std::int64_t>(half); dx <= static_cast<std::int64_t>(half); ++dx) {
      const std::int64_t wx = cx + dx;
      const std::int64_t wy = cy + dy;
      if (wx < 0 || wy < 0 || wx >= w || wy >= h) continue;
      const std::uint32_t ux = static_cast<std::uint32_t>(wx);
      const std::uint32_t uy = static_cast<std::uint32_t>(wy);
      const std::size_t ia = (static_cast<std::size_t>(uy) * a.width + ux) * 4;
      const std::size_t ib = (static_cast<std::size_t>(uy) * b.width + ux) * 4;
      if (a.pixels[ia + 3] == 0 && b.pixels[ib + 3] == 0) continue;
      ++relevant;
      double d = 0.0;
      for (int c = 0; c < 4; ++c) {
        d += std::abs(static_cast<int>(a.pixels[ia + c]) - static_cast<int>(b.pixels[ib + c])) / 255.0;
      }
      acc += d / 4.0;
    }
  }
  if (relevant == 0) return 0.0;
  return acc / static_cast<double>(relevant);
}

double palette_violation_ratio(const FidelityReport& report) {
  return std::clamp(1.0 - report.palette_adherence, 0.0, 1.0);
}

double line_consistency_between(const ImageRgba8& a, const ImageRgba8& b) {
  if (a.width != b.width || a.height != b.height || a.width == 0 || a.height == 0) return 0.0;
  double matched = 0.0;
  std::size_t outline_n = 0;
  for (std::uint32_t y = 0; y < a.height; ++y) {
    for (std::uint32_t x = 0; x < a.width; ++x) {
      const bool oa = is_outline(a, x, y);
      const bool ob = is_outline(b, x, y);
      if (!oa && !ob) continue;
      ++outline_n;
      if (oa && ob && weight_class(a, x, y) == weight_class(b, x, y)) matched += 1.0;
      else if (oa && ob) matched += 0.5;
    }
  }
  if (outline_n == 0) return 1.0;
  return matched / static_cast<double>(outline_n);
}

double pixel_cluster_instability(std::span<const ImageRgba8> frames) {
  if (frames.size() < 2) return 0.0;
  double total_instability = 0.0;
  std::size_t pairs = 0;
  for (std::size_t f = 1; f < frames.size(); ++f) {
    const ImageRgba8& a = frames[f - 1];
    const ImageRgba8& b = frames[f];
    if (a.width != b.width || a.height != b.height || a.width == 0) continue;
    std::size_t changed = 0, relevant = 0;
    for (std::uint32_t y = 0; y < a.height; ++y) {
      for (std::uint32_t x = 0; x < a.width; ++x) {
        const std::uint8_t ca = pixel_class(a, x, y);
        const std::uint8_t cb = pixel_class(b, x, y);
        if (ca == 0xFF && cb == 0xFF) continue;
        ++relevant;
        if (ca != cb) ++changed;
      }
    }
    if (relevant > 0) {
      total_instability += static_cast<double>(changed) / static_cast<double>(relevant);
      ++pairs;
    }
  }
  if (pairs == 0) return 0.0;
  return total_instability / static_cast<double>(pairs);
}

QualityReport evaluate_frame_pair(const VisualCanon* canon,
                                  const ImageRgba8& a, const ImageRgba8& b,
                                  std::string_view landmark_id) {
  QualityReport rep;
  rep.silhouette_disconnection =
      static_cast<double>(silhouette_disconnected_regions(b));
  rep.landmark_jitter = (canon) ? landmark_jitter_between(*canon, a, b, landmark_id) : 0.0;
  rep.line_inconsistency = 1.0 - line_consistency_between(a, b);
  rep.frame_duplication = 1.0 - frame_similarity(a, b);
  rep.contour_correspondence = line_consistency_between(a, b);

  auto diag = [&](std::string code, const std::string& msg) {
    rep.diagnostics.diagnostics.push_back({std::move(code), msg});
  };
  if (rep.silhouette_disconnection > 0.0) {
    diag("QUALITY_SILHOUETTE_DISCONNECTION",
        "frame b has " + std::to_string(static_cast<std::uint32_t>(rep.silhouette_disconnection)) +
        " accidental silhouette region(s)");
  }
  if (rep.landmark_jitter > 0.35) {
    diag("QUALITY_LANDMARK_JITTER",
        "landmark '" + std::string(landmark_id) + "' jitter " + std::to_string(rep.landmark_jitter));
  }
  if (rep.line_inconsistency > 0.35) {
    diag("QUALITY_LINE_INCONSISTENCY",
        "line weight inconsistency " + std::to_string(rep.line_inconsistency));
  }
  if (rep.frame_duplication < 0.02) {
    diag("QUALITY_FRAME_DUPLICATION", "frames are near-identical (duplication risk)");
  }
  return rep;
}

} // namespace gspl::sprites::visual
