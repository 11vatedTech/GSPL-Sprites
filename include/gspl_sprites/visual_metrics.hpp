#pragma once

#include "gspl_sprites/common.hpp"
#include "gspl_sprites/image.hpp"
#include "gspl_sprites/visual_ir.hpp"
#include "gspl_sprites/visual_raster.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace gspl::sprites::visual {

/* ── Deterministic visual fidelity metrics ──
 * Machine validation of rendered output. These metrics produce
 * structured diagnostics; they never replace human inspection but they
 * make fidelity measurable and regression-testable. All metrics are
 * pure functions of image/IR data - no learned critic here yet. */

struct SilhouetteStats {
  double area_ratio{0.0};          // covered fraction of canvas
  std::uint32_t covered_pixels{0};
  std::uint32_t perimeter_pixels{0};
  std::uint32_t connected_components{0};
  double aspect_ratio{0.0};
  double centroid_dx{0.0};         // normalized centroid offset from canvas center
  double centroid_dy{0.0};
  Bounds bounds;
};

struct FidelityReport {
  ValidationResult diagnostics;
  SilhouetteStats silhouette;
  double alpha_edge_quality{0.0};   // 1 = all boundary pixels are AA-partial
  double palette_adherence{0.0};    // 0..1 fraction of covered pixels near palette colors
  double shape_overlap_ratio{0.0};  // fraction of covered area with >1 part (via material channel)
  double frame_distinction{0.0};    // vs reference (0 identical .. 1 fully different)
  double loop_closure{0.0};         // self-similarity 0..1 (1 = identical)
  bool palette_valid{false};
  std::vector<std::string> notes;
};

[[nodiscard]] SilhouetteStats analyze_silhouette(const ImageRgba8& image);
[[nodiscard]] double frame_similarity(const ImageRgba8& a, const ImageRgba8& b);
[[nodiscard]] double frame_distinction(const ImageRgba8& a, const ImageRgba8& b);

/* Measure a rendered frame against the IR that produced it. `reference`
 * is optional (used for distinction/loop metrics). */
[[nodiscard]] FidelityReport measure_frame_fidelity(const VisualIr& ir, const RasterOutput& out,
                                                     const ImageRgba8* reference = nullptr);

} // namespace gspl::sprites::visual
