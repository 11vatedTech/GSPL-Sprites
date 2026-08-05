#pragma once

#include "gspl_sprites/core.hpp"

#include <map>
#include <span>
#include <string>
#include <string_view>

namespace gspl::sprites {

/* MorphologyMap canonical alias. EffectiveMorphology (synthesis.hpp) and
 * MorphologyMap are the same map type; this is the semantic serialization
 * authority for both. */
using MorphologyMap = std::map<std::string, MorphologyPart, std::less<>>;
using FormMorphologyOverrides =
    std::map<std::string, MorphologyMap, std::less<>>;

/* ── Morphology semantic serialization authority ──
 *
 * Two canonical forms exist and must remain byte-stable (package identities
 * are computed from them):
 *   - JSON form: used by seed canonicalization (core.cpp canonicalize())
 *     and effective-morphology JSON artifacts.
 *   - Preimage form: used by Living Visual Package provenance identity
 *     (package.cpp canonicalize_morphology_preimage) via domain separation.
 *
 * These are the ONLY morphology semantic serializers. Consumers must not
 * independently reinterpret morphology fields. New visual semantics added
 * in future versions must flow through this authority with versioned schemas.
 */

[[nodiscard]] std::string canonicalize_morphology_part(const MorphologyPart& part);
[[nodiscard]] std::string canonicalize_morphology_map(const MorphologyMap& map);
[[nodiscard]] std::string canonicalize_form_morphology_overrides(const FormMorphologyOverrides& overrides);
[[nodiscard]] std::string canonicalize_effective_morphology_preimage(const MorphologyMap& morph);
[[nodiscard]] std::string canonicalize_transformations_preimage(std::span<const MorphologyMap> transformation);

} // namespace gspl::sprites
