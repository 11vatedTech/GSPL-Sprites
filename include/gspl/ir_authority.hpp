#pragma once

#include <string_view>

namespace gspl {

/* ═══ IR AUTHORITY ═══
 *
 * The repository contains two IR types. Their roles are now explicit:
 *
 *   gspl::SpriteIr          — COMPILER INTERCHANGE IR (compatibility).
 *                             Produced by IrGenPhase from the canonical entity
 *                             gene graph; serialized by IrSerializer; consumed
 *                             by CLI diagnostics, headless evidence, and IR
 *                             persistence. It is a view over the canonical
 *                             entity, NOT the production authority.
 *
 *   gspl::sprites::SpriteIr — CANONICAL PRODUCTION IR.
 *                             Produced by SpriteIrLowering::lower(CanonicalEntity)
 *                             (authoritative) or compile(SpriteSeed) (seed path).
 *                             Consumed by synthesis, package building, runtime
 *                             wiring, and every downstream production system.
 *
 * Strategy: C (designate canonical + compatibility adapter) + D (document).
 * No rename was performed because the compiler interchange IR is part of the
 * public compiler surface and its serialization format is stable. Both IRs
 * derive from CanonicalEntity; CanonicalEntity is the single semantic
 * authority. See docs/IR_AUTHORITY.md.
 */

/* Classification of every public pipeline path. */
enum class IrPathRole {
  canonical,        // authoritative production path
  compatibility,    // adapter for legacy callers
  fixture           // acceptance fixture only
};

[[nodiscard]] inline std::string_view ir_path_role_name(IrPathRole role) noexcept {
  switch (role) {
    case IrPathRole::canonical:     return "canonical";
    case IrPathRole::compatibility: return "compatibility";
    case IrPathRole::fixture:       return "fixture";
  }
  return "unknown";
}

} // namespace gspl
