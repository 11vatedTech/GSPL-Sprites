#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace gspl::sprites {

class TargetSourceEvidence final {
public:
  [[nodiscard]] const std::string &package_identity() const noexcept;
  [[nodiscard]] const std::string &seed_identity() const noexcept;
  [[nodiscard]] const std::string &authoring_provenance_sha256() const noexcept;
  [[nodiscard]] const std::string &target_compatibility_sha256() const noexcept;

private:
  TargetSourceEvidence(std::string package_identity, std::string seed_identity,
                       std::string authoring_provenance_sha256,
                       std::string target_compatibility_sha256);
  std::string package_identity_;
  std::string seed_identity_;
  std::string authoring_provenance_sha256_;
  std::string target_compatibility_sha256_;
  friend TargetSourceEvidence target_source_evidence_from_package(
      const std::filesystem::path &package_root);
};

[[nodiscard]] TargetSourceEvidence
target_source_evidence_from_package(const std::filesystem::path &package_root);
[[nodiscard]] std::string canonicalize_target_source_evidence(
    const std::optional<TargetSourceEvidence> &evidence);
[[nodiscard]] bool
is_canonical_target_source_evidence(std::string_view value) noexcept;

} // namespace gspl::sprites
