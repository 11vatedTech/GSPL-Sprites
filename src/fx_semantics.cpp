#include "gspl_sprites/core.hpp"
#include "gspl_sprites/fx_semantics.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace gspl::sprites::visual {
namespace {

std::string escape_json(std::string_view value) {
  std::string out;
  for (const unsigned char c : value) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"':  out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c < 0x20) {
          out += "\\u00";
          const char* hex = "0123456789abcdef";
          out += hex[(c >> 4) & 0xF];
          out += hex[c & 0xF];
        } else {
          out += static_cast<char>(c);
        }
    }
  }
  return out;
}

[[nodiscard]] bool finite(const Vec2& v) { return std::isfinite(v.x) && std::isfinite(v.y); }

} // namespace

std::string_view fx_energy_kind_name(FxEnergyKind kind) noexcept {
  switch (kind) {
    case FxEnergyKind::neutral: return "neutral";
    case FxEnergyKind::kinetic: return "kinetic";
    case FxEnergyKind::heat: return "heat";
    case FxEnergyKind::electricity: return "electricity";
    case FxEnergyKind::light: return "light";
    case FxEnergyKind::force: return "force";
    case FxEnergyKind::void_: return "void";
    case FxEnergyKind::biological: return "biological";
    case FxEnergyKind::water: return "water";
    case FxEnergyKind::wind: return "wind";
  }
  return "unknown";
}

std::optional<FxEnergyKind> fx_energy_kind_from_name(std::string_view name) noexcept {
  for (int i = 0; i <= static_cast<int>(FxEnergyKind::wind); ++i) {
    const auto kind = static_cast<FxEnergyKind>(i);
    if (fx_energy_kind_name(kind) == name) return kind;
  }
  return std::nullopt;
}

std::string_view fx_phenomenon_name(FxPhenomenon kind) noexcept {
  switch (kind) {
    case FxPhenomenon::aura: return "aura";
    case FxPhenomenon::impact_flash: return "impact_flash";
    case FxPhenomenon::speed_lines: return "speed_lines";
    case FxPhenomenon::trail: return "trail";
    case FxPhenomenon::electric_arc: return "electric_arc";
    case FxPhenomenon::energy_burst: return "energy_burst";
    case FxPhenomenon::ripple: return "ripple";
    case FxPhenomenon::wind_current: return "wind_current";
    case FxPhenomenon::glow_pulse: return "glow_pulse";
    case FxPhenomenon::debris: return "debris";
    case FxPhenomenon::smoke: return "smoke";
    case FxPhenomenon::shockwave: return "shockwave";
  }
  return "unknown";
}

std::optional<FxPhenomenon> fx_phenomenon_from_name(std::string_view name) noexcept {
  for (int i = 0; i <= static_cast<int>(FxPhenomenon::shockwave); ++i) {
    const auto kind = static_cast<FxPhenomenon>(i);
    if (fx_phenomenon_name(kind) == name) return kind;
  }
  return std::nullopt;
}

ValidationResult validate_fx_state(const FxState& state, const FxLimits& limits) {
  ValidationResult result;
  auto add = [&](std::string code, const std::string& msg) {
    result.diagnostics.push_back({std::move(code), msg});
  };
  if (state.schema != "gspl.fx/0.1") {
    add("FX_SCHEMA", "unexpected schema '" + state.schema + "'");
  }
  if (state.entity_id.empty()) {
    add("FX_NO_ENTITY", "FX state has no entity id");
  }
  if (state.effects.size() > limits.max_effects) {
    add("FX_LIMIT_EXCEEDED", "effect count " + std::to_string(state.effects.size()) +
        " exceeds max " + std::to_string(limits.max_effects));
  }
  std::vector<std::string> ids;
  for (const FxEffect& e : state.effects) {
    if (e.id.empty()) add("FX_EMPTY_ID", "an effect has an empty id");
    if (e.source_part.empty()) add("FX_NO_SOURCE", "effect '" + e.id + "' has no source part");
    if (std::find(ids.begin(), ids.end(), e.id) != ids.end()) {
      add("FX_DUPLICATE_ID", "duplicate effect id '" + e.id + "'");
    }
    ids.push_back(e.id);
    if (!finite(e.velocity)) add("FX_NONFINITE", "effect '" + e.id + "' velocity is non-finite");
    const auto in01 = [&](double v, const char* name) {
      if (!(v >= 0.0 && v <= 1.0)) add("FX_RANGE", "effect '" + e.id + "' " + name + " outside [0,1]");
    };
    in01(e.intensity, "intensity");
    in01(e.charge, "charge");
    in01(e.branching, "branching");
    in01(e.persistence, "persistence");
    in01(e.surface_attachment, "surface_attachment");
    in01(e.environment_influence, "environment_influence");
    in01(e.emission, "emission");
    in01(e.impact_response, "impact_response");
    if (!(e.temperature >= -1.0 && e.temperature <= 1.0)) {
      add("FX_TEMPERATURE", "effect '" + e.id + "' temperature outside [-1,1]");
    }
  }
  return result;
}

std::string canonicalize_fx_state(const FxState& state) {
  std::ostringstream out;
  out << "{\"schema\":\"" << escape_json(state.schema)
      << "\",\"entity\":\"" << escape_json(state.entity_id)
      << "\",\"frame\":" << state.frame_index
      << ",\"effects\":[";
  for (std::size_t i = 0; i < state.effects.size(); ++i) {
    if (i) out << ",";
    const FxEffect& e = state.effects[i];
    out << "{\"id\":\"" << escape_json(e.id)
        << "\",\"phenomenon\":\"" << escape_json(fx_phenomenon_name(e.phenomenon))
        << "\",\"energy\":\"" << escape_json(fx_energy_kind_name(e.energy))
        << "\",\"source\":\"" << escape_json(e.source_part)
        << "\",\"target\":\"" << escape_json(e.target_part)
        << "\",\"intensity\":" << e.intensity
        << ",\"temperature\":" << e.temperature
        << ",\"charge\":" << e.charge
        << ",\"vx\":" << e.velocity.x << ",\"vy\":" << e.velocity.y
        << ",\"branching\":" << e.branching
        << ",\"persistence\":" << e.persistence
        << ",\"attachment\":" << e.surface_attachment
        << ",\"environment\":" << e.environment_influence
        << ",\"emission\":" << e.emission
        << ",\"impact\":" << e.impact_response
        << ",\"color_role\":\"" << escape_json(e.color_role) << "\"}";
  }
  out << "]}";
  return out.str();
}

std::string fx_state_identity(const FxState& state) {
  return gspl::sprites::sha256(canonicalize_fx_state(state));
}

FxDrawParams interpret_fx_effect(const FxEffect& effect, double style_level) {
  FxDrawParams p;
  p.band_strength = std::clamp(effect.intensity * style_level, 0.0, 1.0);
  p.line_strength = std::clamp((effect.branching * 0.5 + effect.charge * 0.5) * style_level, 0.0, 1.0);
  p.halftone_dots = std::clamp(effect.persistence * 0.4 * style_level, 0.0, 1.0);
  p.emission_alpha = std::clamp(effect.emission, 0.0, 1.0);
  p.pixel_ramp_scale = std::clamp((effect.temperature * 0.5 + 0.5) * effect.intensity, 0.0, 1.0);
  return p;
}

} // namespace gspl::sprites::visual
