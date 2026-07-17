#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace gspl::sprites {

enum class RightsClass { original_user_creation, user_owned, licensed, public_domain, permissive, research_only, restricted, unknown, prohibited };
enum class RuntimeState { idle, observing, charging, attacking, recovering };

struct AbilitySeed {
  std::string id;
  std::string effect;
  std::uint32_t cost{};
  std::uint32_t cooldown_ticks{};
  std::uint32_t active_ticks{};
};

struct SpriteSeed {
  std::string schema;
  std::string stable_id;
  std::string name;
  std::string classification;
  RightsClass rights{RightsClass::unknown};
  std::uint64_t entropy_root{};
  std::string primary_color;
  std::string accent_color;
  std::vector<AbilitySeed> abilities;
};

struct Diagnostic { std::string code; std::string message; };
struct ValidationResult { std::vector<Diagnostic> diagnostics; [[nodiscard]] bool ok() const noexcept { return diagnostics.empty(); } };

struct SpriteIr {
  std::string seed_identity;
  std::string entity_id;
  std::vector<AbilitySeed> abilities;
  std::string primary_color;
  std::string accent_color;
};

struct RuntimeEntity {
  RuntimeState state{RuntimeState::idle};
  std::uint32_t energy{100};
  std::uint32_t remaining_ticks{};
  std::uint32_t cooldown_ticks{};
};

[[nodiscard]] SpriteSeed parse_seed(std::string_view source);
[[nodiscard]] ValidationResult validate(const SpriteSeed& seed);
[[nodiscard]] std::string canonicalize(const SpriteSeed& seed);
[[nodiscard]] std::string sha256(std::string_view input);
[[nodiscard]] SpriteIr compile(const SpriteSeed& seed);
[[nodiscard]] bool activate(RuntimeEntity& entity, const AbilitySeed& ability);
void tick(RuntimeEntity& entity) noexcept;
[[nodiscard]] std::string render_svg(const SpriteIr& ir);
void build_package(const SpriteSeed& seed, const std::filesystem::path& output);

} // namespace gspl::sprites

