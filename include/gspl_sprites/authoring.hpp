#pragma once

#include "gspl_sprites/core.hpp"

#include <optional>
#include <span>

namespace gspl::sprites {

struct AuthoringField {
  std::string path;
  std::vector<std::string> alternatives;
  std::optional<std::uint32_t> selected;
  bool locked{};
};

struct AuthoringAbility {
  AbilitySeed value;
  bool enabled{true};
  bool locked{};
};

struct AuthoringVariant {
  std::string id;
  std::vector<std::pair<std::string, std::string>> field_overrides;
  std::vector<std::string> disabled_abilities;
};

struct AuthoringProject {
  std::string schema{"gspl.sprite-authoring/0.1"};
  std::string id;
  std::string intent;
  std::uint64_t revision{};
  std::optional<std::string> parent_revision_identity;
  std::vector<AuthoringField> fields;
  std::vector<AuthoringAbility> abilities;
  std::vector<AuthoringVariant> variants;
};

struct AuthoringEdit {
  std::string field_path;
  std::optional<std::string> selected_value;
  std::optional<bool> locked;
};

struct AuthoringLoweringResult {
  std::optional<SpriteSeed> seed;
  ValidationResult validation;
  [[nodiscard]] bool ok() const noexcept {
    return seed.has_value() && validation.ok();
  }
};

[[nodiscard]] ValidationResult
validate_authoring_project(const AuthoringProject &project);
[[nodiscard]] std::string
canonicalize_authoring_project(const AuthoringProject &project);
[[nodiscard]] std::string
authoring_revision_identity(const AuthoringProject &project);
[[nodiscard]] AuthoringProject
revise_authoring_project(const AuthoringProject &project,
                         std::string_view expected_revision_identity,
                         std::span<const AuthoringEdit> edits);
[[nodiscard]] AuthoringLoweringResult
lower_authoring_project(const AuthoringProject &project,
                        std::optional<std::string_view> variant_id = {});

} // namespace gspl::sprites
