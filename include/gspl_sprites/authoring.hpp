#pragma once

#include "gspl_sprites/core.hpp"
#include "gspl_sprites/target_contract.hpp"

#include <filesystem>
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

enum class AuthoringReferenceUse {
  semantic_structure,
  visual_asset,
  motion_asset,
  audio_asset
};

struct AuthoringReference {
  std::string id;
  std::string uri;
  std::string content_sha256;
  RightsClass rights{RightsClass::unknown};
  AuthoringReferenceUse use{AuthoringReferenceUse::semantic_structure};
  bool required{true};
};

struct AuthoringTargetRequest {
  std::string adapter_id;
  std::vector<TargetRequirement> features;
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
  std::vector<AuthoringReference> references;
  std::vector<AuthoringTargetRequest> targets;
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
canonicalize_authoring_provenance(const AuthoringProject &project);
[[nodiscard]] std::string
canonicalize_authoring_target_reports(const AuthoringProject &project);
[[nodiscard]] std::string
authoring_revision_identity(const AuthoringProject &project);
[[nodiscard]] AuthoringProject
revise_authoring_project(const AuthoringProject &project,
                         std::string_view expected_revision_identity,
                         std::span<const AuthoringEdit> edits);
[[nodiscard]] AuthoringLoweringResult
lower_authoring_project(const AuthoringProject &project,
                        std::optional<std::string_view> variant_id = {});
[[nodiscard]] AuthoringProject parse_authoring_project(std::string_view source);
[[nodiscard]] std::string
serialize_authoring_project(const AuthoringProject &project);
void save_authoring_project(const AuthoringProject &project,
                            const std::filesystem::path &path);
[[nodiscard]] AuthoringProject
load_authoring_project(const std::filesystem::path &path);
[[nodiscard]] AuthoringProject
authoring_project_from_seed(const SpriteSeed &seed, std::string project_id,
                            std::string intent);

} // namespace gspl::sprites
