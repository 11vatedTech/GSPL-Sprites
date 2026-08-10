#pragma once

#include "gspl_sprites/image.hpp"
#include "gspl_sprites/visual_ir.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace gspl::sprites::visual {

/* ── Native Layered Raster Compiler ──
 * The first GSPL-native rendering backend. Input: canonical Visual IR
 * (validated). Output: ImageRgba8 plus semantic channels. The compiler
 * executes resolved visual instructions; all semantic decisions
 * (style, materials, markings, palette) were made upstream in the
 * Visual Semantic Compiler. No entity-specific behavior lives here.
 *
 * Rendering is deterministic (no randomness, no globals, explicit
 * raster-work budget) and analytic-AA capable (no external raster lib
 * is semantic authority). */

struct RasterLimits {
  std::uint32_t max_width{1024};
  std::uint32_t max_height{1024};
  std::uint64_t max_raster_work{32ULL << 20};  // deterministic pixel-work budget
  std::uint32_t max_flattened_vertices{(1u << 16)};
  std::uint32_t max_markings{128};
};

struct ChannelRaster {
  std::string id;
  ChannelMapKind kind{ChannelMapKind::emissive};
  ImageRgba8 image;
};

struct RasterOutput {
  ImageRgba8 image;
  std::vector<ChannelRaster> channels;
  std::uint64_t raster_work{};  // pixels processed (deterministic)
};

struct RasterResult {
  std::optional<RasterOutput> value;
  ValidationResult diagnostics;
  [[nodiscard]] bool ok() const { return value.has_value() && diagnostics.ok(); }
};

/* Render a validated Visual IR to RGBA + channels. Fails closed on
 * invalid input, resource overrun, or any nonfinite geometry. */
[[nodiscard]] RasterResult render_visual_ir(const VisualIr& ir, const RasterLimits& limits = {});

} // namespace gspl::sprites::visual
