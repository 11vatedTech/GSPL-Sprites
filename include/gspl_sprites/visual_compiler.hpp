#pragma once

#include "gspl_sprites/visual_ir.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gspl::sprites::visual {

struct VisualCanon;
struct PerformanceIntent;
struct KeyPose;
struct StyleProgram;

/* ── Visual Semantic Compiler ──
 * Production SpriteIr + form + style context + performance state
 * -> Canonical Visual IR. This is where high-level semantics become
 * explicit graphics semantics. The renderer consumes only the result.
 *
 * No Voltfox-specific behavior: forms, palettes and morphologies come
 * exclusively from the input SpriteIr and style context. */

struct VisualCompileOptions {
  std::string form_id;                       // "" = base (first form)
  std::uint32_t canvas_width{128};
  std::uint32_t canvas_height{128};
  std::string style_preset;                  // optional named preset
  std::vector<StylePatch> style_patches;     // deterministic precedence order
  const PerformanceState* performance{nullptr};
  // Visual Intelligence inputs (all optional; compose through the same
  // sealed pipeline when present):
  const VisualCanon* canon{nullptr};         // canon-driven morphology + enforcement
  const PerformanceIntent* intent{nullptr};  // semantic intent -> performance state
  const KeyPose* key_pose{nullptr};          // key pose (requires intent)
  const StyleProgram* style_program{nullptr};// factorized style program
  std::string projection_kind{"2d"};
  std::span<const ChannelRequest> channel_requests{};
  // Typed FX requests: explicit phenomenon+energy+source semantics. The
  // compiler validates each request against the resolved morphology and
  // resolves them into FxState. No magnitude heuristic invents a phenomenon.
  std::span<const FxRequest> fx_requests{};
  // Semantic LOD: authored resolution rules are executed for this target
  // resolution (see VisualFeature rules; identity-critical features survive).
  std::uint32_t target_resolution{0};
};

struct VisualIrResult {
  std::optional<VisualIr> value;
  ValidationResult diagnostics;
  [[nodiscard]] bool ok() const { return value.has_value() && diagnostics.ok(); }
};

/* Compile a canonical Visual IR for the given form of a production SpriteIr. */
[[nodiscard]] VisualIrResult compile_visual_ir(const SpriteIr& ir, const VisualCompileOptions& options = {});

} // namespace gspl::sprites::visual
