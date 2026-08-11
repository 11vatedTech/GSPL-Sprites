#pragma once

#include "gspl_sprites/common.hpp"
#include "gspl_sprites/image.hpp"
#include "gspl_sprites/performance_intent.hpp"
#include "gspl_sprites/temporal_identity.hpp"
#include "gspl_sprites/visual_canon.hpp"
#include "gspl_sprites/visual_geometry.hpp"
#include "gspl_sprites/visual_metrics.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gspl::sprites::visual {

/* ── Visual Quality Diagnostics ──
 * Objective lower-bound fidelity constraints that are enforceable and
 * regression-testable. These metrics never claim artistic excellence;
 * they detect structural defects: silhouette disconnection, landmark
 * jitter, marking drift, palette violations, line inconsistency,
 * prohibited anti-aliasing, pixel-cluster instability. See
 * docs/architecture/VISUAL_FIDELITY_VALIDATION.md. */

struct QualityReport {
  ValidationResult diagnostics;   // violations are diagnostics; ok() = clean
  double silhouette_disconnection{0.0};   // 1 = accidental regions found
  double landmark_jitter{0.0};            // per-landmark position drift 0..1
  double contour_correspondence{0.0};     // 0..1 (1 = fully stable)
  double marking_drift{0.0};              // 0..1
  double frame_duplication{0.0};          // 1 = frames are identical
  double palette_violation_ratio{0.0};    // 0..1
  double line_inconsistency{0.0};         // 0..1
  double prohibited_aa_ratio{0.0};        // AA pixels under no-AA policy
  double pixel_cluster_instability{0.0};  // 0..1 across frames
};

/* Silhouette health: disconnected accidental regions in a rendered frame
 * (beyond a tolerance) are defects. */
[[nodiscard]] std::uint32_t silhouette_disconnected_regions(const ImageRgba8& image,
                                                            double tolerance_ratio = 0.02);

/* Landmark jitter between two rendered frames of the same pose schedule:
 * normalized drift of the landmark graph. */
[[nodiscard]] double landmark_jitter_between(const VisualCanon& canon,
                                             const ImageRgba8& a, const ImageRgba8& b,
                                             std::string_view landmark_id);

/* Palette violation: fraction of covered pixels not near the IR palette
 * (reuses visual_metrics machinery). */
[[nodiscard]] double palette_violation_ratio(const FidelityReport& report);

/* Line consistency: fraction of outline pixels whose weight class
 * (thin/normal/thick by run length) changed between frames. */
[[nodiscard]] double line_consistency_between(const ImageRgba8& a, const ImageRgba8& b);

/* Pixel-cluster instability: average per-pixel class change across
 * frames for the pixel-constrained policy (deterministic). */
[[nodiscard]] double pixel_cluster_instability(std::span<const ImageRgba8> frames);

/* Full two-frame quality evaluation. `canon` is optional (landmark
 * checks skipped when absent). */
[[nodiscard]] QualityReport evaluate_frame_pair(const VisualCanon* canon,
                                                const ImageRgba8& a, const ImageRgba8& b,
                                                std::string_view landmark_id = {});

} // namespace gspl::sprites::visual
