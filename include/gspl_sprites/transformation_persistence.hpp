#pragma once

#include "gspl_sprites/transformation.hpp"

#include <cstdint>
#include <string>

namespace gspl::sprites {

[[nodiscard]] std::string canonicalize_transformation_program(
    const TransformationProgram &program, const CombatProgram &combat_program);
[[nodiscard]] std::string transformation_program_identity(
    const TransformationProgram &program, const CombatProgram &combat_program);
[[nodiscard]] std::string serialize_transformation_state(
    const TransformationProgram &program, const CombatProgram &combat_program,
    const TransformationState &state);
[[nodiscard]] TransformationState deserialize_transformation_state(
    const TransformationProgram &program, const CombatProgram &combat_program,
    std::string_view source,
    std::uint64_t maximum_bytes = 4ULL * 1024ULL * 1024ULL);
[[nodiscard]] std::string transformation_state_identity(
    const TransformationProgram &program, const CombatProgram &combat_program,
    const TransformationState &state);

} // namespace gspl::sprites
