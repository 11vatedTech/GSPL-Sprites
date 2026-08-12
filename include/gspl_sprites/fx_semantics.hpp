#pragma once

#include "gspl_sprites/common.hpp"
#include "gspl_sprites/visual_common.hpp"
#include "gspl_sprites/visual_geometry.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gspl::sprites::visual {

/* ── FX Semantics ──
 * FX is represented as physical/semantic phenomenon (energy, impact,
 * wind, electricity, aura...) with typed properties, never as texture
 * decals. A StyleProgram decides how the phenomenon is drawn (cel
 * bands, speed lines, halftones, pixel ramps). The renderer consumes
 * only style-interpreted results. See STYLE_PROGRAM_ARCHITECTURE.md. */

enum class FxEnergyKind {
  neutral, kinetic, heat, electricity, light, force, void_, biological, water, wind
};
[[nodiscard]] std::string_view fx_energy_kind_name(FxEnergyKind kind) noexcept;
[[nodiscard]] std::optional<FxEnergyKind> fx_energy_kind_from_name(std::string_view name) noexcept;

enum class FxPhenomenon {
  aura, impact_flash, speed_lines, trail, electric_arc, energy_burst,
  ripple, wind_current, glow_pulse, debris, smoke, shockwave
};
[[nodiscard]] std::string_view fx_phenomenon_name(FxPhenomenon kind) noexcept;
[[nodiscard]] std::optional<FxPhenomenon> fx_phenomenon_from_name(std::string_view name) noexcept;

struct FxEffect {
  std::string id;
  FxPhenomenon phenomenon{FxPhenomenon::aura};
  FxEnergyKind energy{FxEnergyKind::neutral};
  std::string source_part;          // semantic emitter (part id)
  std::string target_part;          // semantic receiver (part id or "")
  double intensity{0.0};            // 0..1
  double temperature{0.0};          // -1..1 cold..hot (semantic)
  double charge{0.0};               // 0..1 electrical charge
  Vec2 velocity;                    // world-relative direction
  double branching{0.0};            // 0..1 (electric arcs, roots)
  double persistence{1.0};          // lifetime multiplier
  double surface_attachment{0.0};   // 0 = free, 1 = pinned to surface
  double environment_influence{0.0};// 0 = self-contained, 1 = environment-driven
  double emission{0.0};             // 0..1 emissive contribution
  double impact_response{0.0};      // 0..1 impulse-response coupling
  std::string color_role{"effect"}; // semantic role (style interprets)
};

struct FxState {
  std::string schema{"gspl.fx/0.1"};
  std::string entity_id;
  std::uint64_t frame_index{0};
  std::vector<FxEffect> effects;
};

struct FxLimits {
  std::uint32_t max_effects{64};
};

/* ── Typed FX request (semantic authority) ──
 * FX phenomenon and energy NEVER originate from magnitude heuristics
 * (force > 0.5 does not invent electricity). An effect is requested with an
 * explicit phenomenon + energy + semantic source; force/commitment may only
 * scale intensity. Requests are validated against the resolved morphology
 * (source/target part ids must exist) before becoming FxState. */
struct FxRequest {
  std::string id;
  FxPhenomenon phenomenon{FxPhenomenon::impact_flash};
  FxEnergyKind energy{FxEnergyKind::neutral};
  std::string source_part;          // semantic emitter (structure/part id)
  std::string target_part;          // semantic receiver ("" allowed)
  double intensity{1.0};            // 0..1 (semantic magnitude, not invented)
  double temperature{0.0};
  double charge{0.0};
  Vec2 velocity;
  double branching{0.0};
  double persistence{1.0};
  double emission{0.0};
  std::string color_role{"effect"};
};

[[nodiscard]] ValidationResult validate_fx_request(const FxRequest& request);
[[nodiscard]] ValidationResult validate_fx_state(const FxState& state, const FxLimits& limits = {});
[[nodiscard]] std::string canonicalize_fx_state(const FxState& state);
[[nodiscard]] std::string fx_state_identity(const FxState& state);

/* Style-interpretation contract: given an effect and the current style
 * program output level, produce deterministic draw parameters. Kept
 * small and explicit; the raster backend consumes these. */
struct FxDrawParams {
  double band_strength{0.0};     // style band interpretation
  double line_strength{0.0};     // speed-line/arc interpretation
  double halftone_dots{0.0};
  double emission_alpha{0.0};
  double pixel_ramp_scale{0.0};
};
[[nodiscard]] FxDrawParams interpret_fx_effect(const FxEffect& effect, double style_level);

} // namespace gspl::sprites::visual
