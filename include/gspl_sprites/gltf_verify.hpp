#pragma once

#include "gspl_sprites/common.hpp"
#include "gspl_sprites/target_contract.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace gspl::sprites {

struct GltfVerification {
  ValidationResult validation;
  std::string glb_identity;
  std::vector<TargetRequirement> requirements;
  [[nodiscard]] bool ok() const noexcept { return validation.ok(); }
};

[[nodiscard]] GltfVerification verify_projection3d_glb(
    std::span<const std::byte> glb,
    std::uint64_t maximum_bytes = 512ULL * 1024ULL * 1024ULL);

} // namespace gspl::sprites
