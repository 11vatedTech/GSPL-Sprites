#pragma once

#include "gspl_sprites/visual_canon.hpp"

namespace gspl::sprites::visual {

/* ── Visual Intelligence canonical fixtures ──
 * Pure canon DATA builders. These are fixtures/reference content, not
 * compiler behavior: every entity flows through the same generic
 * VisualCanon -> morphology -> Visual IR -> raster pipeline. See
 * docs/architecture/VISUAL_CANON_ARCHITECTURE.md.
 *
 * Canon coordinates are RELATIONAL units (y-up, forward = +y). The
 * compiler maps them to canvas units via a deterministic body scale. */

/* Voltfox — first owned quality reference: quadruped fox-like creature
 * with layered construction, markings, expressive face and an electric
 * storm form. */
[[nodiscard]] VisualCanon make_voltfox_canon();

/* Generalization fixtures (prove the architecture is not fox-specific). */
[[nodiscard]] VisualCanon make_humanoid_canon();  // biped humanoid
[[nodiscard]] VisualCanon make_mech_canon();      // mechanical / inanimate
[[nodiscard]] VisualCanon make_flyer_canon();     // non-quadruped creature

} // namespace gspl::sprites::visual
